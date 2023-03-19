#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

// readString: Reads a string from file_path and dumps it into fd.
// Uses cyclic reading-writing to avoid buffer overflow.
int readString(const char* file_path, int fd)
{
    printf("[Reader] Started with file '%s'\n", file_path);

    const int input_fd = open(file_path, O_RDONLY);
    if (input_fd == -1) {
        printf("[Reader Error] Failed to open file '%s': %s\n", file_path, strerror(errno));
        return 1;
    }

    int exit_code = 0;

    static char buffer[BUFFER_SIZE];

    ssize_t read_bytes = 0;
    size_t written_bytes = 0;

    do {
        read_bytes = read(input_fd, buffer, BUFFER_SIZE);
        if (read_bytes == -1) {
            printf("[Reader Error] Failed to read another chunk of file '%s': %s\n",
                file_path, strerror(errno));
            exit_code = 1;
            goto cleanup;
        }

        if (write(fd, buffer, read_bytes) < 0) {
            printf("[Reader Error] Failed to write another chunk of file '%s' to pipe: '%s'\n", file_path, strerror(errno));
            exit_code = 1;
            goto cleanup;
        }

        written_bytes += read_bytes;
    } while (read_bytes == BUFFER_SIZE);

cleanup:
    // Close no longer needed input_fd.
    if (close(input_fd) < 0) {
        printf("[Reader Error] Failed to close input file '%s': %s\n", file_path, strerror(errno));
        return 1;
    }

    if (exit_code == 0) {
        printf("[Reader] Passed a string of length %zu from file '%s' to fd %d\n",
            written_bytes, file_path, fd);
    }

    return exit_code;
}

// writeString: Reads a string from fd and dumps it into file_path.
// Uses cyclic reading-writing to avoid buffer overflow.
int writeString(const char* file_path, int fd)
{
    printf("[Writer] Started with file '%s'\n", file_path);

    const int output_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_fd == -1) {
        printf("[Writer Error] Failed to open file '%s': %s\n", file_path, strerror(errno));
        return 1;
    }

    int exit_code = 0;

    static char buffer[BUFFER_SIZE];
    ssize_t read_bytes = 0;

    do {
        read_bytes = read(fd, buffer, BUFFER_SIZE);
        if (read_bytes == -1) {
            printf("[Writer Error] Failed to read another chunk from fd %d: %s\n", fd, strerror(errno));
            exit_code = 1;
            goto cleanup;
        }

        if (write(output_fd, buffer, read_bytes) < 0) {
            printf("[Writer Error] Failed to write result to file '%s': %s\n", file_path, strerror(errno));
            exit_code = 1;
            goto cleanup;
        }
    } while (read_bytes == BUFFER_SIZE);

cleanup:
    // Close no longer needed output_fd
    if (close(output_fd) < 0) {
        printf("[Reader Error] Failed to close output file '%s': %s\n", file_path, strerror(errno));
        return 1;
    }

    if (exit_code == 0) {
        printf("[Writer] Passed result to file '%s' from input fd %d\n", file_path, fd);
    }

    return exit_code;
}

void checkArgumentCount(bool arg_condition, const char* arg_name)
{
    if (arg_condition) {
        printf("Usage: ./prog <input_file_1> <input_file_2> <output_file_1> <output_file_2>\n");
        printf("[Error] Missing required argument %s\n", arg_name);
        exit(1);
    }
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
    checkArgumentCount(argc < 2, "<input_file_1>");
    checkArgumentCount(argc < 3, "<input_file_2>");
    checkArgumentCount(argc < 4, "<output_file_1>");
    checkArgumentCount(argc < 5, "<output_file_2>");

    // Create all FIFOs here.
    if (mkfifo(INPUT_FIFO_NAME_1, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            INPUT_FIFO_NAME_1, strerror(errno));
        return 1;
    }

    if (mkfifo(INPUT_FIFO_NAME_2, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            INPUT_FIFO_NAME_2, strerror(errno));
        return 1;
    }

    if (mkfifo(OUTPUT_FIFO_NAME_1, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            OUTPUT_FIFO_NAME_1, strerror(errno));
        return 1;
    }

    if (mkfifo(OUTPUT_FIFO_NAME_2, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            OUTPUT_FIFO_NAME_2, strerror(errno));
        return 1;
    }

    int input_fd_1 = -1;
    int input_fd_2 = -1;
    int output_fd_1 = -1;
    int output_fd_2 = -1;

    if ((input_fd_1 = open(INPUT_FIFO_NAME_1, O_WRONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open input pipe '%s': %s\n",
            INPUT_FIFO_NAME_1, strerror(errno));
        return 1;
    }

    int exit_code = 0;
    printf("[Reader-Writer] Opened (reader-writer -> data handler) pipe '%s' with fd: '%d'\n",
        INPUT_FIFO_NAME_1, input_fd_1);

    if ((input_fd_2 = open(INPUT_FIFO_NAME_2, O_WRONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open input pipe '%s': %s\n",
            INPUT_FIFO_NAME_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Reader-Writer] Opened (reader-writer -> data handler) pipe '%s' with fd: '%d'\n",
        INPUT_FIFO_NAME_2, input_fd_2);

    const char* input_file_1 = argv[1];
    const char* input_file_2 = argv[2];

    exit_code = readString(input_file_1, input_fd_1)
        || readString(input_file_2, input_fd_2);

    closeFile(&input_fd_1);
    closeFile(&input_fd_2);

    if (exit_code != 0) {
        printf("[Reader-Writer Error] Failed to read strings, exiting...\n");
        goto cleanup;
    }

    if ((output_fd_1 = open(OUTPUT_FIFO_NAME_1, O_RDONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open output pipe '%s': %s\n",
            OUTPUT_FIFO_NAME_1, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Reader-Writer] Opened (data handler -> reader-writer) pipe '%s' with fd: '%d'\n",
        OUTPUT_FIFO_NAME_1, input_fd_1);

    if ((output_fd_2 = open(OUTPUT_FIFO_NAME_2, O_RDONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open output pipe '%s': %s\n",
            OUTPUT_FIFO_NAME_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Reader-Writer] Opened (data handler -> reader-writer) pipe '%s' with fd: '%d'\n",
        OUTPUT_FIFO_NAME_2, input_fd_2);

    const char* output_file_1 = argv[3];
    const char* output_file_2 = argv[4];

    exit_code = writeString(output_file_1, output_fd_1)
        || writeString(output_file_2, output_fd_2);

    if (exit_code != 0) {
        printf("[Reader-Writer Error] Failed to write strings, exiting...\n");
        goto cleanup;
    }

cleanup:
    closeFile(&input_fd_1);
    closeFile(&input_fd_2);
    closeFile(&output_fd_1);
    closeFile(&output_fd_2);

    if (exit_code == 0) {
        printf("[Reader-Writer] Done!\n");
    }

    return exit_code;
}
