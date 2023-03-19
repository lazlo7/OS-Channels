#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Buffer size to be used for cycling reading-writing.
#define BUFFER_SIZE 8192

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

typedef enum {
    STR_DIFF_UNKNOWN,
    STR_DIFF_INCLUDED,
    STR_DIFF_EXCLUDED
} string_difference_t;

// Computes a string difference between including and excluding strings.
// Puts the result in result buffer, which should be of at least size 256.
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

int handleStings(int input_fd_1, int input_fd_2, int output_fd_1, int output_fd_2)
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

// Data Handler: Computes string difference from input_fd_1 and input_fd_2
// and dumps the result into output_fd_1 and output_fd_2.
// Uses cyclic reading to avoid buffer overflow.
int dataHandler(
    const char* input_pipe_name_1, const char* input_pipe_name_2,
    const char* output_pipe_name_1, const char* output_pipe_name_2)
{
    int input_fd_1 = -1;
    if ((input_fd_1 = open(input_pipe_name_1, O_RDONLY)) < 0) {
        printf("[Data Handler Error] Failed to open input pipe '%s': %s\n",
            input_pipe_name_1, strerror(errno));
        return 1;
    }

    printf("[Data Handler] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        input_pipe_name_1, input_fd_1);

    int exit_code = 0;

    int input_fd_2 = -1;
    if ((input_fd_2 = open(input_pipe_name_2, O_RDONLY)) < 0) {
        printf("[Data Handler Error] Failed to open input pipe '%s': %s\n",
            input_pipe_name_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Data Handler] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        input_pipe_name_2, input_fd_2);

    int output_fd_1 = -1;
    if ((output_fd_1 = open(output_pipe_name_1, O_WRONLY)) < 0) {
        printf("[Data Handler Error] Failed to open output pipe '%s': %s\n",
            output_pipe_name_1, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Data Handler] Opened (data handler -> reader-writer) pipe '%s' with fd: %d\n",
        output_pipe_name_1, output_fd_1);

    int output_fd_2 = -1;
    if ((output_fd_2 = open(output_pipe_name_2, O_WRONLY)) < 0) {
        printf("[Data Handler Error] Failed to open output pipe '%s': %s\n", output_pipe_name_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Data Handler] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        output_pipe_name_2, output_fd_2);

    exit_code = handleStings(input_fd_1, input_fd_2, output_fd_1, output_fd_2);

cleanup:
    closeFile(&input_fd_1);
    closeFile(&input_fd_2);
    closeFile(&output_fd_1);
    closeFile(&output_fd_2);

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

// ReaderWriter: combines reader and writer process.
// Creates data handler process inside itself.
int readerWriter(
    const char* input_file_1, const char* input_file_2,
    const char* output_file_1, const char* output_file_2)
{
    static const char* unhandled_data_pipe_name_1 = "unhandled_1.fifo";
    static const char* unhandled_data_pipe_name_2 = "unhandled_2.fifo";
    static const char* handled_data_pipe_name_1 = "handled_1.fifo";
    static const char* handled_data_pipe_name_2 = "handled_2.fifo";

    // Create all FIFOs here.
    if (mkfifo(unhandled_data_pipe_name_1, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            unhandled_data_pipe_name_1, strerror(errno));
        return 1;
    }

    if (mkfifo(unhandled_data_pipe_name_2, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            unhandled_data_pipe_name_2, strerror(errno));
        return 1;
    }

    if (mkfifo(handled_data_pipe_name_1, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            handled_data_pipe_name_1, strerror(errno));
        return 1;
    }

    if (mkfifo(handled_data_pipe_name_2, 0666) < 0 && errno != EEXIST) {
        printf("[Reader-Writer Error] Failed to create pipe '%s': %s\n",
            handled_data_pipe_name_2, strerror(errno));
        return 1;
    }

    // Fork for data handler process inside reader-writer.
    int fork_result = fork();
    if (fork_result == -1) {
        return 1;
    }

    // Pass input strings to data handler.
    if (fork_result == 0) {
        return dataHandler(unhandled_data_pipe_name_1, unhandled_data_pipe_name_2,
            handled_data_pipe_name_1, handled_data_pipe_name_2);
    }

    int input_fd_1 = -1;
    int input_fd_2 = -1;
    int output_fd_1 = -1;
    int output_fd_2 = -1;

    if ((input_fd_1 = open(unhandled_data_pipe_name_1, O_WRONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open input pipe '%s': %s\n",
            unhandled_data_pipe_name_1, strerror(errno));
        return 1;
    }

    int exit_code = 0;
    printf("[Reader-Writer] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        unhandled_data_pipe_name_1, input_fd_1);

    if ((input_fd_2 = open(unhandled_data_pipe_name_2, O_WRONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open input pipe '%s': %s\n",
            unhandled_data_pipe_name_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Reader-Writer] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        unhandled_data_pipe_name_2, input_fd_2);

    // Read input strings.
    exit_code = readString(input_file_1, input_fd_1)
        || readString(input_file_2, input_fd_2);

    if (exit_code != 0) {
        printf("[Reader-Writer Error] Failed to read strings, exiting...");
        goto cleanup;
    }

    if ((output_fd_1 = open(handled_data_pipe_name_1, O_RDONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open output pipe '%s': %s\n",
            handled_data_pipe_name_1, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Reader-Writer] Opened (data handler -> reader-writer) pipe '%s' with fd: %d\n",
        handled_data_pipe_name_1, output_fd_1);

    if ((output_fd_2 = open(handled_data_pipe_name_2, O_RDONLY)) < 0) {
        printf("[Reader-Writer Error] Failed to open output pipe '%s': %s\n",
            handled_data_pipe_name_2, strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    printf("[Reader-Writer] Opened (reader-writer -> data handler) pipe '%s' with fd: %d\n",
        handled_data_pipe_name_2, output_fd_2);

    // Wait for data handler process to exit.
    int child_exit_status = 0;
    if (wait(&child_exit_status) == -1) {
        printf("[Reader-Writer Error] Failed to wait for data handler process to finish: %s\n", strerror(errno));
        exit_code = 1;
        goto cleanup;
    }

    // Check that data handler process exited normally.
    if (WEXITSTATUS(child_exit_status) != 0) {
        printf("[Reader-Writer Error] Data handler process returned with exit code %d, exiting...\n",
            WEXITSTATUS(child_exit_status));
        exit_code = 1;
        goto cleanup;
    }

    closeFile(&input_fd_1);
    closeFile(&input_fd_2);

    // Only after that reader-writer can start writing results.
    exit_code = writeString(output_file_1, output_fd_1)
        || writeString(output_file_2, output_fd_2);

cleanup:
    closeFile(&input_fd_1);
    closeFile(&input_fd_2);
    closeFile(&output_fd_1);
    closeFile(&output_fd_2);

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

int main(int argc, char** argv)
{
    checkArgumentCount(argc < 2, "<input_file_1>");
    checkArgumentCount(argc < 3, "<input_file_2>");
    checkArgumentCount(argc < 4, "<output_file_1>");
    checkArgumentCount(argc < 5, "<output_file_1>");

    int fork_result = fork();
    if (fork_result == -1) {
        printf("[Error] Failed to fork for reader-writer process: %s\n", strerror(errno));
        return 1;
    }

    if (fork_result == 0) {
        return readerWriter(argv[1], argv[2], argv[3], argv[4]);
    }

    int reader_writer_exit_status;
    if (wait(&reader_writer_exit_status) == -1) {
        printf("[Error] Failed to wait for reader-writer process to finish: %s\n", strerror(errno));
        return 1;
    }

    if (WEXITSTATUS(reader_writer_exit_status) != 0) {
        printf("[Error] Reader-writer process returned with exit code %d, exiting...\n",
            WEXITSTATUS(reader_writer_exit_status));
        return 1;
    }

    printf("Done!\n");
}
