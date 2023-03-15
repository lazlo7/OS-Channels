#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Buffer size to be used for cycling reading-writing.
#define BUFFER_SIZE 8192

// Reader: Reads a string from file_path and dumps it into fd.
// Uses cyclic reading-writing to avoid buffer overflow.
void reader(const char* file_path, int fd)
{
    printf("[Reader] Started with file '%s'\n", file_path);

    const int input_fd = open(file_path, O_RDONLY);
    if (input_fd == -1) {
        printf("[Reader Error] Failed to open file '%s': %s\n", file_path, strerror(errno));
        exit(1);
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
        exit(1);
    }

    if (exit_code != 0) {
        exit(exit_code);
    }

    printf("[Reader] Passed a string of length %zu from file '%s' to fd %d\n",
        written_bytes, file_path, fd);
}

// Computes a string difference between including and excluding strings.
// Puts the result in result buffer, which should be of at least size 256.
void updateStringDifference(
    const char* including, size_t including_length,
    const char* excluding, size_t excluding_length,
    bool* result)
{
    for (size_t i = 0; i < including_length; ++i) {
        result[(unsigned char)including[i]] = true;
    }

    for (size_t i = 0; i < excluding_length; ++i) {
        result[(unsigned char)excluding[i]] = false;
    }
}

// Data Handler: Computes string difference from input_fd_1 and input_fd_2
// and dumps the result into output_fd_1 and output_fd_2.
// Uses cyclic reading to avoid buffer overflow.
void dataHandler(int input_fd_1, int input_fd_2, int output_fd_1, int output_fd_2)
{
    printf("[Handler] Started with input fds %d and %d\n", input_fd_1, input_fd_2);

    static char buffer_1[BUFFER_SIZE];
    static char buffer_2[BUFFER_SIZE];

    // Just to be sure, we'll be allocating an array of size 256.
    static bool string_difference_1[256];
    static bool string_difference_2[256];

    // Clearing potential leftover data.
    memset(string_difference_1, false, sizeof(string_difference_1));
    memset(string_difference_2, false, sizeof(string_difference_2));

    ssize_t read_result_1 = 0;
    ssize_t read_result_2 = 0;

    // Computing string differences.
    do {
        read_result_1 = read(input_fd_1, buffer_1, BUFFER_SIZE);
        if (read_result_1 == -1) {
            printf("[Handler Error] Failed to read another chunk from pipe 1: %s\n", strerror(errno));
            exit(1);
        }

        read_result_2 = read(input_fd_2, buffer_2, BUFFER_SIZE);
        if (read_result_2 == -1) {
            printf("[Handler Error] Failed to read another chunk from pipe 2: %s\n", strerror(errno));
            exit(1);
        }

        updateStringDifference(buffer_1, read_result_1, buffer_2, read_result_2, string_difference_1);
        updateStringDifference(buffer_2, read_result_2, buffer_1, read_result_1, string_difference_2);
    } while (read_result_1 == BUFFER_SIZE || read_result_2 == BUFFER_SIZE);

    // Compiling string results.
    static char result_1[128];
    static char result_2[128];

    size_t result_1_length = 0;
    for (int i = 0; i < 128; ++i) {
        if (string_difference_1[i]) {
            result_1[result_1_length++] = (char)i;
        }
    }

    size_t result_2_length = 0;
    for (int i = 0; i < 128; ++i) {
        if (string_difference_2[i]) {
            result_2[result_2_length++] = (char)i;
        }
    }

    // Writing results.
    if (write(output_fd_1, result_1, result_1_length) < 0) {
        printf("[Handler Error] Failed to write result to pipe 1: %s\n", strerror(errno));
        exit(1);
    }

    if (write(output_fd_2, result_2, result_2_length) < 0) {
        printf("[Handler Error] Failed to write result to pipe 2: %s\n", strerror(errno));
        exit(1);
    }

    printf("[Handler] Passed results to output fds %d and %d\n", output_fd_1, output_fd_2);
}

// Writer: Reads a string from fd and dumps it into file_path.
// Uses cyclic reading-writing to avoid buffer overflow.
void writer(const char* file_path, int fd)
{
    printf("[Writer] Started with file '%s'\n", file_path);

    const int output_fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_fd == -1) {
        printf("[Writer Error] Failed to open file '%s': %s\n", file_path, strerror(errno));
        exit(1);
    }

    int exit_code = 0;

    static char buffer[BUFFER_SIZE];
    ssize_t read_bytes = 0;

    do {
        read_bytes = read(fd, buffer, BUFFER_SIZE);
        if (read_bytes == -1) {
            printf("[Writer Error] Failed to read another chunk of file '%s': %s\n", file_path, strerror(errno));
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
        exit(1);
    }

    if (exit_code != 0) {
        exit(exit_code);
    }

    printf("[Writer] Passed result to file '%s' from input fd %d\n", file_path, fd);
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

    int exit_code = 0;

    int unhandled_data_fds_1[2];
    if (pipe(unhandled_data_fds_1) < 0) {
        printf("[Error] Failed to create unhandled data pipe 1: %s\n", strerror(errno));
        return 1;
    }

    int unhandled_data_fds_2[2];
    if (pipe(unhandled_data_fds_2) < 0) {
        printf("[Error] Failed to create unhandled data pipe 2: %s\n", strerror(errno));
        exit_code = 1;
        goto unhandled_data_fds_1_cleanup;
    }

    const char* input_file_1 = argv[1];
    const char* input_file_2 = argv[2];

    int fork_result = fork();
    if (fork_result == -1) {
        printf("[Error] Failed to fork for reader process: %s\n", strerror(errno));
        exit_code = 1;
        goto unhandled_data_fds_2_cleanup;
    }

    if (fork_result == 0) {
        // In the child process -> read strings and pass them to data handler.
        reader(input_file_1, unhandled_data_fds_1[1]);
        reader(input_file_2, unhandled_data_fds_2[1]);
        return 0;
    }

    // Wait until the reader process is done.
    if (wait(NULL) == -1) {
        printf("[Error] Failed to wait for reader process to finish: %s\n", strerror(errno));
        exit_code = 1;
        goto unhandled_data_fds_2_cleanup;
    }

    int handled_data_fds_1[2];
    if (pipe(handled_data_fds_1) < 0) {
        printf("[Error] Failed to create handled data pipe 1: %s\n", strerror(errno));
        exit_code = 1;
        goto unhandled_data_fds_2_cleanup;
    }

    int handled_data_fds_2[2];
    if (pipe(handled_data_fds_2) < 0) {
        printf("[Error] Failed to create handled data pipe 2: %s\n", strerror(errno));
        exit_code = 1;
        goto handled_data_fds_1_cleanup;
    }

    fork_result = fork();
    if (fork_result == -1) {
        printf("[Error] Failed to fork for data handler process: %s\n", strerror(errno));
        exit_code = 1;
        goto handled_data_fds_2_cleanup;
    }

    if (fork_result == 0) {
        // In the child process -> handle data and pass the results to writer.
        dataHandler(unhandled_data_fds_1[0], unhandled_data_fds_2[0],
            handled_data_fds_1[1], handled_data_fds_2[1]);
        return 0;
    }

    // Wait until the data handler process is done.
    if (wait(NULL) == -1) {
        printf("[Error] Failed to wait for data handler process to finish: %s\n", strerror(errno));
        exit_code = 1;
        goto handled_data_fds_2_cleanup;
    }

    const char* output_file_1 = argv[3];
    const char* output_file_2 = argv[4];

    fork_result = fork();
    if (fork_result == -1) {
        printf("[Error] Failed to fork for writer process: %s\n", strerror(errno));
        exit_code = 1;
        goto handled_data_fds_2_cleanup;
    }

    if (fork_result == 0) {
        // In the child process -> read results and write them to the files.
        writer(output_file_1, handled_data_fds_1[0]);
        writer(output_file_2, handled_data_fds_2[0]);
        return 0;
    }

    // Wait until the writer process is done.
    if (wait(NULL) == -1) {
        printf("[Error] Failed to wait for writer process to finish: %s\n", strerror(errno));
        exit_code = 1;
        goto handled_data_fds_2_cleanup;
    }

    // Closing all fds.
handled_data_fds_2_cleanup:
    close(handled_data_fds_1[0]);
    close(handled_data_fds_1[1]);

handled_data_fds_1_cleanup:
    close(handled_data_fds_2[0]);
    close(handled_data_fds_2[1]);

unhandled_data_fds_2_cleanup:
    close(unhandled_data_fds_2[0]);
    close(unhandled_data_fds_2[1]);

unhandled_data_fds_1_cleanup:
    close(unhandled_data_fds_1[0]);
    close(unhandled_data_fds_1[1]);

    return exit_code;
}
