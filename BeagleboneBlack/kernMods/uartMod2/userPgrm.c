#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_FILE "/dev/AL_UART"

int main(int argc, char *argv[])
{
    int fd;
    char message[256];
    
    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("(UserProgram) Failed to open device file");
        return EXIT_FAILURE;
    }

    printf("(UserProgram) Type your message below (type 'quit' to exit):\n");

    // Copy into message buffer
    snprintf(message, sizeof(message), "%s", argv[1]);

    while (1) 
    {
        printf("> ");
        
        // Read user input from stdin
        if (fgets(message, sizeof(message), stdin) == NULL) //blocking call
        {
            perror("(UserProgram) Error reading input");
            break;
        }

        // Remove trailing newline from fgets
        message[strcspn(message, "\n")] = '\0';

        // Check for exit condition
        if (strcmp(message, "quit") == 0) 
        {
            printf("(UserProgram) Exiting...\n");
            break;
        }

        // Write the message to the device
        if (write(fd, message, strlen(message)) < 0) 
        {
            perror("(UserProgram) Failed to send message via WRITE");
            break;
        }

        printf("(UserProgram) Message sent successfully!\n");
    }

    close(fd);
    return EXIT_SUCCESS;
}
