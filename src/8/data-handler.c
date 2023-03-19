#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

typedef enum {
    STR_DIFF_UNKNOWN,
    STR_DIFF_INCLUDED,
    STR_DIFF_EXCLUDED
} string_difference_t;

void updateStringDifference(
    const char* including, size_t including_length,
    const char* excluding, size_t excluding_length,
    string_difference_t* result)
{
    for (size_t i = 0; i < including_length; ++i) {
        if (result[(unsigned char)including[i]] != STR_DIFF_EXCLUDED) {
            result[(unsigned char)including[i]] = STR_DIFF_INCLUDED;
        }
    }

    for (size_t i = 0; i < excluding_length; ++i) {
        result[(unsigned char)excluding[i]] = STR_DIFF_EXCLUDED;
    }
}

int handleStrings(int input_fd_1, int input_fd_2, int output_fd_1, int output_fd_2)
{
    printf("[Handler] Started with input fds %d and %d\n", input_fd_1, input_fd_2);

    static char buffer_1[BUFFER_SIZE];
    static char buffer_2[BUFFER_SIZE];

    // Just to be sure, we'll be allocating an array of size 256.
    static string_difference_t string_difference_1[256];
    static string_difference_t string_difference_2[256];

    // Clearing potential leftover data.
    memset(string_difference_1, STR_DIFF_UNKNOWN, sizeof(string_difference_1));
    memset(string_difference_2, STR_DIFF_UNKNOWN, sizeof(string_difference_2));

    ssize_t read_result_1 = 0;
    ssize_t read_result_2 = 0;

    // Computing string differences.
    do {
        read_result_1 = read(input_fd_1, buffer_1, BUFFER_SIZE);
        if (read_result_1 == -1) {
            printf("[Handler Error] Failed to read another chunk from pipe 1: %s\n", strerror(errno));
            return 1;
        }

        read_result_2 = read(input_fd_2, buffer_2, BUFFER_SIZE);
        if (read_result_2 == -1) {
            printf("[Handler Error] Failed to read another chunk from pipe 2: %s\n", strerror(errno));
            return 1;
        }

        updateStringDifference(buffer_1, read_result_1, buffer_2, read_result_2, string_difference_1);
        updateStringDifference(buffer_2, read_result_2, buffer_1, read_result_1, string_difference_2);
    } while (read_result_1 == BUFFER_SIZE || read_result_2 == BUFFER_SIZE);

    // Compiling string results.
    static char result_1[128];
    static char result_2[128];

    size_t result_1_length = 0;
    for (int i = 0; i < 128; ++i) {
        if (string_difference_1[i] == STR_DIFF_INCLUDED) {
            result_1[result_1_length++] = (char)i;
        }
    }

    size_t result_2_length = 0;
    for (int i = 0; i < 128; ++i) {
        if (string_difference_2[i] == STR_DIFF_INCLUDED) {
            result_2[result_2_length++] = (char)i;
        }
    }

    // Writing results.
    if (write(output_fd_1, result_1, result_1_length) < 0) {
        printf("[Handler Error] Failed to write result to pipe 1: %s\n", strerror(errno));
        return 1;
    }

    if (write(output_fd_2, result_2, result_2_length) < 0) {
        printf("[Handler Error] Failed to write result to pipe 2: %s\n", strerror(errno));
        return 1;
    }

    printf("[Handler] Passed results to output fds %d and %d\n", output_fd_1, output_fd_2);

    return 0;
}

// Helper function that closes a file descriptor and sets it to -1 only if it's not equal to -1.
static void closeFile(int* fd)
{
    if (*fd != -1) {
        close(*fd);
        *fd = -1;
    }
}

int main(int argc, char** argv)
{
    // Data handler doesn't need argc, argv.
    (void)argc;
    (void)argv;

    int input_fd_1 = -1;
    if ((input_fd_1 = open(INPUT_FIFO_NAME_1, O_RDONLY)) < 0) {
        printf("[Data Handler Error] Failed to open input pipe '%s': %s\n",
            INPUT_FIFO_NAME_1, strerror(errno));
        return 1;
    }

    printf("[Data Handler] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        INPUT_FIFO_NAME_1, input_fd_1);

    int exit_code = 0;

    int input_fd_2 = -1;
    if ((input_fd_2 = open(INPUT_FIFO_NAME_2, O_RDONLY)) < 0) {
        printf("[Data Handler Error] Failed to open input pipe '%s': %s\n",
            INPUT_FIFO_NAME_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Data Handler] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        INPUT_FIFO_NAME_2, input_fd_2);

    int output_fd_1 = -1;
    if ((output_fd_1 = open(OUTPUT_FIFO_NAME_1, O_WRONLY)) < 0) {
        printf("[Data Handler Error] Failed to open output pipe '%s': %s\n",
            OUTPUT_FIFO_NAME_1, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Data Handler] Opened (data handler -> reader-writer) pipe '%s' with fd: %d\n",
        OUTPUT_FIFO_NAME_1, output_fd_1);

    int output_fd_2 = -1;
    if ((output_fd_2 = open(OUTPUT_FIFO_NAME_2, O_WRONLY)) < 0) {
        printf("[Data Handler Error] Failed to open output pipe '%s': %s\n",
            OUTPUT_FIFO_NAME_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Data Handler] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        OUTPUT_FIFO_NAME_2, output_fd_2);

    exit_code = handleStrings(input_fd_1, input_fd_2, output_fd_1, output_fd_2);

    if (exit_code != 0) {
        printf("[Data Handler Error] Failed to handle strings, exiting...");
        goto cleanup;
    }

cleanup:
    closeFile(&input_fd_1);
    closeFile(&input_fd_2);
    closeFile(&output_fd_1);
    closeFile(&output_fd_2);

    if (exit_code == 0) {
        printf("[Data Handler] Done!\n");
    }

    return exit_code;
}
