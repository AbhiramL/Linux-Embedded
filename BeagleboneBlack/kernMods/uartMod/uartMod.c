#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/file.h>

#define DEVICE_NAME "AL_UART"
#define CLASS_NAME  "my_uart_module"
#define UART_BUFFER_SIZE 256
#define UART_BASE  0x48022000  // UART1 Base Address on BBB
#define UART_THR   0x00        // Transmit Holding Register
#define UART_LSR   0x14        // Line Status Register
#define UART_LSR_THRE  0x20    // Transmit Holding Register Empty flag

static char uart_buffer[UART_BUFFER_SIZE];

// Declare device major and minor numbers
static int major_number = -1;
static struct class *uart_class = NULL;
static struct device *uart_device = NULL;
static struct cdev uart_cdev;

// Function prototypes
static void write_uart_char(char c);
static void write_uart_string(const char *str);
static void send_uart_message(const char *message);
static int uart_open(struct inode *inode, struct file *file);
static int uart_release(struct inode *inode, struct file *file);
static ssize_t uart_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static long uart_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

void __iomem *uart_base;

// Define file operations structure
static const struct file_operations uart_fops = {
    .owner = THIS_MODULE,
    .open = uart_open,
    .release = uart_release,
    .read = uart_read,
    .write = uart_write,
    .unlocked_ioctl = uart_ioctl,  // Updated to modern unlocked_ioctl
};


// Function to write a character to UART instead of using default driver
static void write_uart_char(char c)
{
    // Wait until the UART is ready
    while (!(readl(uart_base + UART_LSR) & UART_LSR_THRE))
        udelay(10);

    // Write the character
    writel(c, uart_base + UART_THR);
}

static void uart_write_string(const char *str)
{
    while (*str)  // Loop until the null terminator is reached
    {
        write_uart_char(*str);  // Write each character
        str++;  // Move to the next character
    }
}

//uart message writing function using default driver
static void send_uart_message(const char *message)
{
    struct file *uart_file;
    ssize_t written;
    loff_t position = 0;    

    uart_file = filp_open("/dev/ttyS0", O_WRONLY | O_NOCTTY, 0); // Change to your UART port

    if (IS_ERR(uart_file)) 
    {
        printk(KERN_ERR "UART Module: Failed to open serial port\n");
        return;
    }

    written = kernel_write(uart_file, message, strlen(message), &position); // Write on Serial port

    if (written < 0)
    {
        printk(KERN_ERR "UART Module: Failed to write to serial port\n");
    }

    filp_close(uart_file, NULL);
}


static int __init uart_init(void)
{
    printk(KERN_INFO "UART Module is being loaded...\n");

    // Register character device
    major_number = register_chrdev(0, DEVICE_NAME, &uart_fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "Failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "Registered UART device with major number %d\n", major_number);

    // Create device class
    uart_class = class_create(CLASS_NAME);
    if (IS_ERR(uart_class))
    {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(uart_class);
    }

    // Create device
    uart_device = device_create(uart_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(uart_device))
    {
        class_destroy(uart_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(uart_device);
    }

    // Initialize character device
    cdev_init(&uart_cdev, &uart_fops);
    if (cdev_add(&uart_cdev, MKDEV(major_number, 0), 1) < 0)
    {
        device_destroy(uart_class, MKDEV(major_number, 0));
        class_destroy(uart_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to add character device\n");
        return -1;
    }

    // Init UART character write by mapping register address
    uart_base = ioremap(UART_BASE, 0x1000);
    if (!uart_base)
    {
        printk(KERN_ERR "Failed to map UART memory\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "(Kern)UART Module Loaded.\n");
    
    send_uart_message("(Ser)UART Module Loaded.\n");

    write_uart_string("Hello Serial World\n");

    return 0;
}

static void __exit uart_exit(void)
{
    printk(KERN_INFO "UART Module is being unloaded...\n");
    
    send_uart_message("(Ser)UART Module is being unloaded.\n");

    write_uart_string("Bye Serial World\n");

    // Clean up character device
    cdev_del(&uart_cdev);
    device_destroy(uart_class, MKDEV(major_number, 0));
    class_destroy(uart_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    if (uart_base)
    {
        iounmap(uart_base);
    }
    
    printk(KERN_INFO "(Kern)UART Module Unloaded.\n");    
}

// Open function
static int uart_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "UART device opened\n");
    return 0;
}

// Release function
static int uart_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "UART device closed\n");
    return 0;
}

// Read function
static ssize_t uart_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    ssize_t bytes_read = 0;

    if (*ppos == 0) {
        if (copy_to_user(buf, uart_buffer, count)) {
            printk(KERN_ERR "Failed to send data to the user\n");
            return -EFAULT;
        }
        bytes_read = count;
        *ppos += bytes_read;
    }

    return bytes_read;
}

// Write function
static ssize_t uart_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    if (count > UART_BUFFER_SIZE) {
        printk(KERN_ERR "Data too large for UART buffer\n");
        return -EINVAL;
    }

    if (copy_from_user(uart_buffer, buf, count)) {
        printk(KERN_ERR "Failed to write data from user\n");
        return -EFAULT;
    }

    printk(KERN_INFO "Data written to UART: %s\n", uart_buffer);
    return count;
}

// IOCTL function
static long uart_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "IOCTL called with cmd: %u, arg: %lu\n", cmd, arg);
    return 0;
}

// Register the init and exit functions
module_init(uart_init);
module_exit(uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AL");
MODULE_DESCRIPTION("A simple UART character driver module");
MODULE_VERSION("1.0");
