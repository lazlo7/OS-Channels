#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void printUsage()
{
    printf("Usage: ./prog <input_file_1> <input_file_2> <output_file_1> <output_file_2>\n");
}

// Returns the number of bytes read.
size_t readFile(const char* file_path, char* buffer, size_t buffer_size)
{
    const int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("[Error] Failed to open file '%s': %s\n", file_path, strerror(errno));
        exit(1);
    }

    const ssize_t read_result = read(fd, buffer, buffer_size);
    if (read_result == -1) {
        printf("[Error] Failed to read from file '%s': %s\n", file_path, strerror(errno));
        exit(1);
    }

    return read_result;
}

// Returns the length of result.
size_t stringDifference(
    const char* including, size_t including_length,
    const char* excluding, size_t excluding_length,
    char* result, size_t result_size)
{
    // Just to be sure, we'll allocate an array of size 256.
    static bool isIncluded[256];
    memset(isIncluded, false, sizeof(isIncluded));

    for (size_t i = 0; i < including_length; ++i) {
        isIncluded[including[i]] = true;
    }

    for (size_t i = 0; i < excluding_length; ++i) {
        isIncluded[excluding[i]] = false;
    }

    size_t result_length = 0;
    for (size_t i = 0; i < 128; ++i) {
        if (isIncluded[i]) {
            result[result_length++] = i;
            if (result_length >= result_size) {
                break;
            }
        }
    }

    return result_length;
}

// Returns the number of bytes written.
size_t writeFile(const char* file_path, char* buffer, size_t buffer_length)
{
    const int fd = open(file_path, O_WRONLY);
    if (fd == -1) {
        printf("[Error] Failed to open file '%s': %s\n", file_path, strerror(errno));
        exit(1);
    }

    const ssize_t write_result = write(fd, buffer, buffer_length);
    if (write_result == -1) {
        printf("[Error] Failed to write to file '%s': %s\n", file_path, strerror(errno));
        exit(1);
    }

    return write_result;
}

void checkArgumentCount(bool arg_condition, const char* arg_name)
{
    if (arg_condition) {
        printUsage();
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

    char str1[8192], str2[8192];
    int unhandled_data_fds[2];
    int fork_result = fork();

    if (fork_result == -1) {
        printf("[Error] Failed to fork: %s\n", strerror(errno));
        return 1;
    } else if (fork_result == 0) {
        // Child process: read a string from each file and pass strings through pipe.
        if (pipe(unhandled_data_fds) < 0) {
            printf("[Error] Failed to create pipe: %s\n", strerror(errno));
            return 1;
        }

        const size_t str1_length = readFile(argv[1], str1, sizeof(str1));
        const size_t str2_length = readFile(argv[2], str2, sizeof(str2));

        if (write(unhandled_data_fds[1], str1, str1_length) < 0) {
            printf("[Error] Failed to write to pipe: %s\n", strerror(errno));
            return 1;
        }
    }

    return 0;
}