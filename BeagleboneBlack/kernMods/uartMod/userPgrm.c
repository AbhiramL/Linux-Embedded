#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_FILE "/dev/AL_UART"
#define SEND_MESSAGE 0x01  // Same as defined in the kernel module

int main(int argc, char *argv[])
{
    int fd;
    char message[256];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    snprintf(message, sizeof(message), "%s", argv[1]);

    // Open the device file
    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device file");
        return EXIT_FAILURE;
    }

    // Send message to the kernel module
    if (ioctl(fd, SEND_MESSAGE, message) < 0) {
        perror("Failed to send message via IOCTL");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Message sent: %s\n", message);

    close(fd);
    return EXIT_SUCCESS;
}

