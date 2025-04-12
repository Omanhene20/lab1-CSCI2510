#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

// Helper function to exit with perror and negative status
void exit_with_error(const char *msg) {
    perror(msg);
    exit(-1);
}

// Compare two buffers of size K
int compare_buffers(const char *a, const char *b, int k) {
    return (memcmp(a, b, k) == 0);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <input file> <output file> <compression length> <mode>\n", argv[0]);
        exit(-1);
    }

    // Parse arguments
    char *input_file = argv[1];
    char *output_file = argv[2];
    int k = atoi(argv[3]);
    int mode = atoi(argv[4]);

    if (k < 1) {
        fprintf(stderr, "Error: Compression length must be >= 1\n");
        exit(-1);
    }

    if (mode != 0 && mode != 1) {
        fprintf(stderr, "Error: Mode must be 0 (compress) or 1 (decompress)\n");
        exit(-1);
    }

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd < 0) {
        exit_with_error("Error opening input file");
    }

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (out_fd < 0) {
        close(in_fd);
        exit_with_error("Error opening output file");
    }

    if (mode == 0) {
        // Compression mode
        char *curr = malloc(k);
        char *next = malloc(k);
        if (!curr || !next) exit_with_error("Memory allocation failed");

        ssize_t bytes_read = read(in_fd, curr, k);
        while (bytes_read > 0) {
            int count = 1;
            ssize_t next_read;
            while ((next_read = read(in_fd, next, k)) == k && compare_buffers(curr, next, k)) {
                count++;
                if (count == 255) break;
            }

            // Write 1-byte count
            unsigned char count_byte = count;
            if (write(out_fd, &count_byte, 1) != 1) exit_with_error("Error writing count");

            // Write K-byte pattern
            if (write(out_fd, curr, bytes_read) != bytes_read) exit_with_error("Error writing pattern");

            // Prepare for next loop
            if (next_read > 0) {
                memcpy(curr, next, next_read);
                bytes_read = next_read;
            } else {
                break;
            }
        }

        free(curr);
        free(next);
    } else {
        // Decompression mode
        unsigned char count;
        char *pattern = malloc(k);
        if (!pattern) exit_with_error("Memory allocation failed");

        while (read(in_fd, &count, 1) == 1) {
            ssize_t pattern_read = read(in_fd, pattern, k);
            if (pattern_read != k) exit_with_error("Error reading pattern during decompression");

            for (int i = 0; i < count; ++i) {
                if (write(out_fd, pattern, k) != k) exit_with_error("Error writing decompressed data");
            }
        }

        free(pattern);
    }

    if (close(in_fd) < 0) exit_with_error("Error closing input file");
    if (close(out_fd) < 0) exit_with_error("Error closing output file");

    return 0;
}
