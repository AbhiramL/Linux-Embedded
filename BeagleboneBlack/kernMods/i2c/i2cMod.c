#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#define DEVICE_NAME "I2C_COMM"
#define CLASS_NAME  "i2c_module"
#define I2C_BUFFER_SIZE 256
#define I2C_DEV_ADDR 0x50  // Example I²C address

static char i2c_buffer[I2C_BUFFER_SIZE];
static int major_number = -1;
static struct class *i2c_class = NULL;
static struct device *i2c_device = NULL;
static struct cdev i2c_cdev;

// Global pointer to our I²C client – set in our probe function.
static struct i2c_client *i2c_client_handle;

// File operations prototypes
static int i2c_dev_open(struct inode *inode, struct file *file);
static int i2c_dev_release(struct inode *inode, struct file *file);
static ssize_t i2c_dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
static ssize_t i2c_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
static long i2c_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#define SEND_I2C_MESSAGE 0x01  // Our custom ioctl command

// File operations structure
static const struct file_operations i2c_fops = {
    .owner = THIS_MODULE,
    .open = i2c_dev_open,
    .release = i2c_dev_release,
    .read = i2c_dev_read,
    .write = i2c_dev_write,
    .unlocked_ioctl = i2c_dev_ioctl,
};

static int i2c_dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "I2C device opened\n");
    return 0;
}

static int i2c_dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "I2C device closed\n");
    return 0;
}

// For simplicity, the read function isn’t implemented here.
static ssize_t i2c_dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

static ssize_t i2c_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    if (count > I2C_BUFFER_SIZE) {
        printk(KERN_ERR "Data too large for I2C buffer\n");
        return -EINVAL;
    }
    if (copy_from_user(i2c_buffer, buf, count)) {
        printk(KERN_ERR "Failed to copy data from user\n");
        return -EFAULT;
    }
    printk(KERN_INFO "Data written to I2C module (but not sent via I²C yet): %s\n", i2c_buffer);
    return count;
}

// Use IOCTL to trigger an I²C transfer
static long i2c_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    char message[I2C_BUFFER_SIZE];
    int ret;

    switch (cmd) {
    case SEND_I2C_MESSAGE:
        if (copy_from_user(message, (char __user *)arg, I2C_BUFFER_SIZE)) {
            printk(KERN_ERR "Failed to copy message from user space\n");
            return -EFAULT;
        }

        /* Here we use the I²C API to send the message.
         * i2c_master_send() sends the data pointed to by message over the I²C bus.
         * The return value should be the number of bytes transmitted or an error code.
         */
        ret = i2c_master_send(i2c_client_handle, message, strnlen(message, I2C_BUFFER_SIZE));
        if (ret < 0) {
            printk(KERN_ERR "Failed to send I2C message: %d\n", ret);
            return ret;
        }
        printk(KERN_INFO "I2C message sent: %s\n", message);
        break;
    default:
        printk(KERN_WARNING "Unknown IOCTL command\n");
        return -EINVAL;
    }
    return 0;
}

// I²C driver probe function
static int i2c_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    // Save the i2c client pointer for future use (e.g., in IOCTL)
    i2c_client_handle = client;
    printk(KERN_INFO "I2C device probed: %s\n", client->name);
    return 0;
}

// I²C driver remove function
static int i2c_driver_remove(struct i2c_client *client)
{
    printk(KERN_INFO "I2C device removed\n");
    return 0;
}

// I²C device ID table (match names between board/device tree and driver)
static const struct i2c_device_id i2c_device_id_table[] = {
    { "i2c_comm_dev", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, i2c_device_id_table);

// I²C driver structure
static struct i2c_driver i2c_driver = {
    .driver = {
        .name = "i2c_comm_driver",
        .owner = THIS_MODULE,
    },
    .probe = i2c_driver_probe,
    .remove = i2c_driver_remove,
    .id_table = i2c_device_id_table,
};

static int __init i2c_comm_init(void)
{
    int ret;

    printk(KERN_INFO "I2C Communication Module loading...\n");

    // Register the I²C driver with the I²C subsystem.
    ret = i2c_add_driver(&i2c_driver);
    if (ret) {
        printk(KERN_ERR "Failed to add I2C driver: %d\n", ret);
        return ret;
    }

    // Register the character device interface
    major_number = register_chrdev(0, DEVICE_NAME, &i2c_fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register a major number\n");
        i2c_del_driver(&i2c_driver);
        return major_number;
    }

    i2c_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(i2c_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        i2c_del_driver(&i2c_driver);
        printk(KERN_ERR "Failed to create device class\n");
        return PTR_ERR(i2c_class);
    }

    i2c_device = device_create(i2c_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(i2c_device)) {
        class_destroy(i2c_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        i2c_del_driver(&i2c_driver);
        printk(KERN_ERR "Failed to create device\n");
        return PTR_ERR(i2c_device);
    }

    cdev_init(&i2c_cdev, &i2c_fops);
    ret = cdev_add(&i2c_cdev, MKDEV(major_number, 0), 1);
    if (ret < 0) {
        device_destroy(i2c_class, MKDEV(major_number, 0));
        class_destroy(i2c_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        i2c_del_driver(&i2c_driver);
        printk(KERN_ERR "Failed to add char device\n");
        return ret;
    }

    printk(KERN_INFO "I2C Communication Module Loaded.\n");
    return 0;
}

static void __exit i2c_comm_exit(void)
{
    printk(KERN_INFO "I2C Communication Module unloading...\n");

    cdev_del(&i2c_cdev);
    device_destroy(i2c_class, MKDEV(major_number, 0));
    class_destroy(i2c_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    i2c_del_driver(&i2c_driver);

    printk(KERN_INFO "I2C Communication Module Unloaded.\n");
}

module_init(i2c_comm_init);
module_exit(i2c_comm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AL");
MODULE_DESCRIPTION("A simple I2C communication module");
MODULE_VERSION("1.0");
