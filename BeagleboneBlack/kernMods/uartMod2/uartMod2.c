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
#include <linux/spinlock.h>  // For spinlock_t and spin_lock()

#define DEVICE_NAME "AL_UART"             // Name of the device
#define CLASS_NAME  "my_uart_module"      // Name of the class for the device
#define UART_BUFFER_SIZE 256              // Size of the buffer for UART communication
#define SEND_MESSAGE_IOCTL 0x01           // Custom IOCTL command

//static char uart_buffer[UART_BUFFER_SIZE];    // Buffer to store data
static int major_number = -1;                 // Device major number
static struct class *uart_class = NULL;       // Device class
static struct device *uart_device = NULL;     // Device instance
static struct cdev uart_cdev;                 // Character device structure

// UART device state structure
struct uart_device {
    char rx_buffer[UART_BUFFER_SIZE]; // Holds recived data
    size_t head;  
    size_t tail;
    spinlock_t lock;  // Protects access to buffer
}

// Function prototypes
static void send_uart_message(const char *message);   // Function to send a message via UART
static int uart_open(struct inode *inode, struct file *file); // Open function
static int uart_release(struct inode *inode, struct file *file); // Release function
static ssize_t uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);  // Write function
static ssize_t uart_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
static long uart_ioctl(struct file *file, unsigned int cmd, unsigned long arg); // IOCTL function

// Define file operations structure
static const struct file_operations uart_fops = {
    .owner = THIS_MODULE,
    .open = uart_open,
    .release = uart_release,
    .write = uart_write,
    .read = uart_read,
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
    send_uart_message("Hello Serial World.\n");
    msleep(5000);

    return 0;
}

// Cleanup function for the kernel module
static void __exit uart_exit(void)
{
    printk(KERN_INFO "(KernMod)UART Module is being unloaded...\n");
    send_uart_message("Bye Serial World.\n");

    // Clean up character device
    cdev_del(&uart_cdev);
    device_destroy(uart_class, MKDEV(major_number, 0));
    class_destroy(uart_class);
    unregister_chrdev(major_number, DEVICE_NAME);

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
static ssize_t uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char message[UART_BUFFER_SIZE];
    
    //if count < UART_BUF_SIZE-1, then len=count, else len=UART_BUF_SIZE-1
    size_t len = (count < UART_BUFFER_SIZE -1) ? count : (UART_BUFFER_SIZE - 1); 
    
    if (copy_from_user(message, buf, len)) 
    {
        printk(KERN_ERR "(KernMod)Failed to copy message from user\n");
        return -EFAULT;
    }
    message[len] = '\0'; // Null-terminate the string
    
    send_uart_message(message);
    printk(KERN_INFO "(KernMod)Message sent to serial port 1 via WRITE: %s\n", message);

    return count;
}

static ssize_t uart_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    struct uart_device *dev = file->private_data;
    ssize_t bytes_read = 0;
    size_t available_bytes, first_part, second_part;

    // Ensure that the device context is available.
    if (!dev)
    {
        return -EFAULT;
    }
    
    // Lock the device state while accessing the ring buffer. 
    spin_lock(&dev->lock);

    // Calculate the number of bytes available in the Rx buffer.
    // This assumes a circular (ring) buffer implementation.
    if (dev->head >= dev->tail)
    {
        available_bytes = dev->head - dev->tail;
    }
    else
    {
        available_bytes = UART_BUFFER_SIZE - dev->tail + dev->head;
    }

    // If no data is available, either block (using wait queues) or return non-blocking.
    // Here we return -EAGAIN if the file was opened with non-blocking I/O.
    if (available_bytes == 0) 
    {
        spin_unlock(&dev->lock);
        return (file->f_flags & O_NONBLOCK) ? -EAGAIN : 0;  // Replace 0 with wait queue logic for blocking I/O.
    }

    // Limit the amount read to what is available.
    if (count > available_bytes)
    {
        count = available_bytes;
    }

    // Handle the possibility that the data wraps around the end of the ring buffer.
    if (dev->tail + count <= UART_BUFFER_SIZE) 
    {
        // Data is contiguous.
        if (copy_to_user(buf, dev->rx_buffer + dev->tail, count))  //copy_to_user will transfer data from kernel space to user space
        {
            spin_unlock(&dev->lock);
            return -EFAULT;
        }
        dev->tail = (dev->tail + count) % UART_BUFFER_SIZE;
        bytes_read = count;
    } 
    else 
    {
        // Data is split in two parts.
        first_part = UART_BUFFER_SIZE - dev->tail;
        second_part = count - first_part;

        if (copy_to_user(buf, dev->rx_buffer + dev->tail, first_part)) //copy_to_user will transfer data from kernel space to user space
        {
            spin_unlock(&dev->lock);
            return -EFAULT;
        }
        if (copy_to_user(buf + first_part, dev->rx_buffer, second_part)) 
        {
            spin_unlock(&dev->lock);
            return -EFAULT;
        }
        dev->tail = second_part;  // Wrap-around completed.
        bytes_read = count;
    }

    spin_unlock(&dev->lock);
    return bytes_read;
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
