#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BAD_SIZE 5000

int main() {
    int handle = open("/dev/fakefile", O_RDWR);
    if(handle == 0) {
        printf("Failed to open `fakefile` device!\n");
        return 0;
    }

    const char input[] = "This a test.";
    size_t msg_size = strlen(input);
    printf("Input is: %s\n", input);

    size_t wcount = write(handle, input, msg_size + 1);
    printf("Wrote %lu bytes.\n", wcount);

    char output[15] = { 0 };
    lseek(handle, 0, SEEK_SET);

    size_t rcount = read(handle, output, 15);
    printf("Read %lu bytes.\n", rcount);

    printf("Output is: %s\n", output);

    char bad_input[BAD_SIZE];
    memset(bad_input, 0xff, BAD_SIZE);
    lseek(handle, 0, SEEK_SET);

    printf("Attempting to write %d bytes...\n", BAD_SIZE);
    size_t bad_wcount = write(handle, bad_input, BAD_SIZE);
    printf("Wrote %lu bytes.\n", bad_wcount);

    close(handle);

    return 0;
}