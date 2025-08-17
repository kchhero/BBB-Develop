#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "my_leds"
#define CLASS_NAME  "myleds"
#define NUM_LEDS    4

static struct gpio_desc *leds_gpios[NUM_LEDS];
static struct class *myleds_class;
static struct cdev myleds_cdev;
static dev_t devt;

static ssize_t myleds_write(struct file *file, const char __user *buf,
                            size_t len, loff_t *ppos)
{
    char kbuf;
    int led_index;
    int current_value;

    if (len == 0)
        return 0;

    // 사용자 공간에서 한 글자만 복사해옵니다.
    if (copy_from_user(&kbuf, buf, 1))
        return -EFAULT;

    led_index = kbuf - '0'; // '0' -> 0, '1' -> 1 ...

    if (led_index >= 0 && led_index < NUM_LEDS) {
        if (IS_ERR_OR_NULL(leds_gpios[led_index])) {
            pr_warn("GPIO for LED %d is not available\n", led_index);
            return -EINVAL;
        }
        // 현재 LED의 상태를 읽어와서 그 반대 값으로 설정합니다 (토글).
        current_value = gpiod_get_value(leds_gpios[led_index]);
        gpiod_set_value(leds_gpios[led_index], !current_value);

        pr_info("Toggled LED %d to %d\n", led_index, !current_value);
    } else {
        pr_warn("Invalid LED index: %d\n", led_index);
    }

    return len;
}

static const struct file_operations myleds_fops = {
    .owner  = THIS_MODULE,
    .write  = myleds_write,
};

static int myleds_probe(struct platform_device *pdev)
{
    int ret, i;
    struct device *dev = &pdev->dev;

    pr_info("myleds: probe called\n");
    pr_info("myleds: device created at /dev/%s\n", DEVICE_NAME);

    for (i = 0; i < NUM_LEDS; i++) {
        leds_gpios[i] = devm_gpiod_get_index(dev, "led", i, GPIOD_OUT_LOW);
        if (IS_ERR(leds_gpios[i])) {
            dev_err(dev, "Failed to get GPIO from 'led-gpios' property at index %d\n", i);
            return PTR_ERR(leds_gpios[i]);
        }
    }

    /* allocate device number */
    ret = alloc_chrdev_region(&devt, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&myleds_cdev, &myleds_fops);
    ret = cdev_add(&myleds_cdev, devt, 1);
    if (ret)
        goto unregister_chrdev;

    myleds_class = class_create(CLASS_NAME);
    if (IS_ERR(myleds_class)) {
        ret = PTR_ERR(myleds_class);
        goto del_cdev;
    }

    device_create(myleds_class, NULL, devt, NULL, DEVICE_NAME);

    dev_info(&pdev->dev, "myleds driver loaded successfully!\n");
    return 0;

del_cdev:
    cdev_del(&myleds_cdev);
unregister_chrdev:
    unregister_chrdev_region(devt, 1);
    return ret;
}

static void myleds_remove(struct platform_device *pdev)
{
    device_destroy(myleds_class, devt);
    class_destroy(myleds_class);
    cdev_del(&myleds_cdev);
    unregister_chrdev_region(devt, 1);
}

static const struct of_device_id myleds_of_match[] = {
    { .compatible = "mycompany,myleds" },
    { }
};
MODULE_DEVICE_TABLE(of, myleds_of_match);

static struct platform_driver myleds_driver = {
    .driver = {
        .name = "myleds",
        .of_match_table = myleds_of_match,
    },
    .probe  = myleds_probe,
    .remove = myleds_remove,
};

module_platform_driver(myleds_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Suker");
MODULE_DESCRIPTION("Custom LED driver for BeagleBone Black (kernel 6.12.x)");
