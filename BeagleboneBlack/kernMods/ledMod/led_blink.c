#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define LED_GPIO 540  // Change to the correct GPIO

static int __init led_blink_init(void) {
    int i;

    printk(KERN_INFO "LED Blink Module: Initializing\n");

    // Request the GPIO
    if (gpio_request(LED_GPIO, "LED_GPIO")) {
        printk(KERN_ERR "LED Blink Module: GPIO %d request failed\n", LED_GPIO);
        return -1;
    }

    // Set GPIO as output
    if (gpio_direction_output(LED_GPIO, 0)) {
        printk(KERN_ERR "LED Blink Module: Failed to set GPIO %d as output\n", LED_GPIO);
        gpio_free(LED_GPIO);
        return -1;
    }

    // Blink LED 15 times
    for (i = 0; i < 15; i++) {
        gpio_set_value(LED_GPIO, 1);
        msleep(500);
        gpio_set_value(LED_GPIO, 0);
        msleep(500);
    }

    printk(KERN_INFO "LED Blink Module: Blinking done. Sleeping...\n");

    return 0;
}

static void __exit led_blink_exit(void) {
    printk(KERN_INFO "LED Blink Module: Exiting\n");
    gpio_set_value(LED_GPIO, 0);  // Ensure LED is off
    gpio_free(LED_GPIO);          // Free GPIO
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AL");
MODULE_DESCRIPTION("Simple LED Blink Module without Kernel Thread");

module_init(led_blink_init);
module_exit(led_blink_exit);
