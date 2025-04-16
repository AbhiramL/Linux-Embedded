#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/file.h>
#include <linux/io.h>
#include <linux/delay.h>


#define DEVICE_NAME "AL_UART"             // Name of the device
#define CLASS_NAME  "my_uart_module"      // Name of the class for the device
#define UART_BUFFER_SIZE 256              // Size of the buffer for UART communication
#define SEND_MESSAGE_IOCTL 0x01           // Custom IOCTL command

static char uart_buffer[UART_BUFFER_SIZE];    // Buffer to store data
static int major_number = -1;                 // Device major number
static struct class *uart_class = NULL;       // Device class
static struct device *uart_device = NULL;     // Device instance
static struct cdev uart_cdev;                 // Character device structure

// Function prototypes
static void send_uart_message(const char *message);   // Function to send a message via UART
static void uart_write_string(const char *str);
static int uart_open(struct inode *inode, struct file *file); // Open function
static int uart_release(struct inode *inode, struct file *file); // Release function
static ssize_t uart_write(struct file *file, unsigned int cmd, unsigned long arg); // Write function
static long uart_ioctl(struct file *file, unsigned int cmd, unsigned long arg); // IOCTL function

// Define file operations structure
static const struct file_operations uart_fops = {
    .owner = THIS_MODULE,
    .open = uart_open,
    .release = uart_release,
    .write = uart_write,
    .unlocked_ioctl = uart_ioctl,  // Updated to modern unlocked_ioctl
};

static void send_uart_message(const char *message)
{
    struct file *uart_file;      // File structure for the serial port
    ssize_t written;             // Number of bytes written
    loff_t position = 0;         // File position
    //char formatted_message[UART_BUFFER_SIZE + 6]; // Buffer for "(Ser)" + message (+1 for null terminator)

    // Prepend "(Ser)" to the message
    //snprintf(formatted_message, sizeof(formatted_message), "(Ser)%s", message);

    // Open the serial port
    uart_file = filp_open("/dev/ttyS1", O_WRONLY | O_NOCTTY, 0); // Change to your UART port

    if (IS_ERR(uart_file)) {
        printk(KERN_ERR "(KernMod)UART Module: Failed to open serial port\n");
        return;
    }

    // Write the message to the serial port
    written = kernel_write(uart_file, message, strlen(message), &position); 

    if (written < 0) {
        printk(KERN_ERR "(KernMod)UART Module: Failed to write to serial port\n");
    }

    // Close the file
    filp_close(uart_file, NULL);
}

// Initialization function for the kernel module
static int __init uart_init(void)
{
    printk(KERN_INFO "(KernMod)UART Module is being loaded...\n");

    // Register character device
    major_number = register_chrdev(0, DEVICE_NAME, &uart_fops);
    if (major_number < 0) 
    {
        printk(KERN_ALERT "(KernMod)Failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "(KernMod)Registered UART device with major number %d\n", major_number);

    // Create device class
    uart_class = class_create(CLASS_NAME);
    if (IS_ERR(uart_class)) 
    {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "(KernMod)Failed to register device class\n");
        return PTR_ERR(uart_class);
    }

    // Create device
    uart_device = device_create(uart_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(uart_device)) 
    {
        class_destroy(uart_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "(KernMod)Failed to create the device\n");
        return PTR_ERR(uart_device);
    }

    // Initialize character device
    cdev_init(&uart_cdev, &uart_fops);
    if (cdev_add(&uart_cdev, MKDEV(major_number, 0), 1) < 0) 
    {
        device_destroy(uart_class, MKDEV(major_number, 0));
        class_destroy(uart_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "(KernMod)Failed to add character device\n");
        return -1;
    }

    printk(KERN_INFO "(KernMod)UART Module Loaded.\n");
    send_uart_message("Hello Serial World...waiting 5 seconds...\n");
    msleep(5000);

    return 0;
}

// Cleanup function for the kernel module
static void __exit uart_exit(void)
{
    printk(KERN_INFO "(KernMod)UART Module is being unloaded...\n");
    uart_write_string("Bye Serial World.\n");

    // Clean up character device
    cdev_del(&uart_cdev);
    device_destroy(uart_class, MKDEV(major_number, 0));
    class_destroy(uart_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    // Unmap the uart 1 from virtual address
    iounmap(uart_virtual_addr);

    printk(KERN_INFO "(KernMod)UART Module Unloaded.\n");   
}

// Open function
static int uart_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "(KernMod)UART device opened\n");
    return 0;
}

// Release function
static int uart_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "(KernMod)UART device closed\n");
    return 0;
}

// Write function
static ssize_t uart_write(struct file *file, unsigned int cmd, unsigned long arg)
{
    char message[UART_BUFFER_SIZE];

    switch (cmd) 
    {
    case SEND_MESSAGE_IOCTL:
        if (copy_from_user(message, (char __user *)arg, UART_BUFFER_SIZE)) 
        {
            printk(KERN_ERR "(KernMod)Failed to copy message from user\n");
            return -EFAULT;
        }
        send_uart_message(message);
        printk(KERN_INFO "(KernMod)Message sent to serial port 1 via WRITE: %s\n", message);
        break;
    default:
        printk(KERN_WARNING "(KernMod)Unknown WRITE command\n");
        return -EINVAL;
    }

    return 0;

}

// IOCTL function
static long uart_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

// Register the init and exit functions
module_init(uart_init);
module_exit(uart_exit);

MODULE_LICENSE("GPL");                 // License for the kernel module
MODULE_AUTHOR("AL");                  // Author of the module
MODULE_DESCRIPTION("A simple UART character driver module"); // Description of the module
MODULE_VERSION("2.0");                // Version of the module
