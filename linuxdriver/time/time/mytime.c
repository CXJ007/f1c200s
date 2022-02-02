#include <linux/module.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/kernel.h>

//定时器的api和正点的有点区别linux的版本不同

#define DEVCNT 1
#define DEVNAME "mytime"
#define CLOSE_CMD 		(_IO(0XEF, 0x1))	/* 关闭定时器 */
#define OPEN_CMD		(_IO(0XEF, 0x2))	/* 打开定时器 */
#define SETPERIOD_CMD	(_IO(0XEF, 0x3))	/* 设置定时器周期命令 */

struct timer_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    unsigned int led_gpio;
    struct device_node *nd;

    struct timer_list timer;
    atomic_t period;
};

struct timer_dev mydev;

static int led_init(struct timer_dev *dev)
{
    int ret;
    dev->nd = of_find_node_by_path("/led");
    if (dev->nd == NULL)
    {
        printk("fail to find_node\r\n");
        ret = -EINVAL;
    }
    dev->led_gpio = of_get_named_gpio(dev->nd, "led-gpios", 0);
    if (dev->led_gpio < 0)
    {
        printk("fail to led_gpio\r\n");
        ret = -EINVAL;
    }
    gpio_request(dev->led_gpio, "led");
    gpio_direction_output(dev->led_gpio, 1);
    return 0;
}

void timer_function(struct timer_list *t)
{
    static int led_status = 1;
    struct timer_dev *timedev = from_timer(timedev, t, timer);
    led_status = !led_status;
    gpio_set_value(timedev->led_gpio, led_status);
    mod_timer(&timedev->timer, jiffies + msecs_to_jiffies(atomic_read(&mydev.period)));
}

static int timer_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    filp->private_data = &mydev;
    
    ret = led_init(&mydev);
    if(ret < 0){
        printk("fail to led_init\r\n");
        return ret;
    }

    atomic_set(&mydev.period, 1000);
    timer_setup(&mydev.timer, timer_function, 0);
    mydev.timer.expires = jiffies + msecs_to_jiffies(atomic_read(&mydev.period));
    add_timer(&mydev.timer);

    return ret;
}

static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = filp->private_data;
    switch(cmd){
        case CLOSE_CMD:
            del_timer_sync(&dev->timer);
            break;
        case OPEN_CMD:
            mod_timer(&dev->timer, jiffies+msecs_to_jiffies(atomic_read(&mydev.period)));
            break;
        case SETPERIOD_CMD:
            atomic_set(&dev->period,(int)arg);
            break;
        default:printk("CMD_ERR\r\n");
    }
    return 0;
}

static int timer_release(struct inode *inode, struct file *filp){
    struct timer_dev *dev = filp->private_data;
    del_timer_sync(&dev->timer);
    gpio_set_value(dev->led_gpio, 1);
    return 0;
}

static struct file_operations time_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .unlocked_ioctl = timer_unlocked_ioctl,
    .release = timer_release,
};

static int __init time_init(void)
{
    int ret;

    if (mydev.major)
    {
        mydev.devid = MKDEV(mydev.major, 0);
        ret = register_chrdev_region(mydev.devid, DEVCNT, DEVNAME);
        if (ret < 0)
        {
            printk("fail to register\r\n");
            return -EINVAL;
        }
    }
    else
    {
        ret = alloc_chrdev_region(&mydev.devid, 0, DEVCNT, DEVNAME);
        if (!ret)
        {
            mydev.major = MAJOR(mydev.devid);
            mydev.minor = MINOR(mydev.devid);
            printk("major:%d minjor:%d\r\n", mydev.major, mydev.minor);
        }
        else
        {
            printk("fail to register\r\n");
            return -EINVAL;
        }
    }

    mydev.cdev.owner = THIS_MODULE;
    cdev_init(&mydev.cdev, &time_fops);
    ret = cdev_add(&mydev.cdev, mydev.devid, DEVCNT);
    if (ret)
    {
        printk("fail to cdev_add\r\n");
        goto cdev_err;
    }
    mydev.class = class_create(THIS_MODULE, DEVNAME);
    if (IS_ERR(mydev.class))
    {
        return PTR_ERR(mydev.class);
    }
    mydev.device = device_create(mydev.class, NULL, mydev.devid, NULL, DEVNAME);
    if (IS_ERR(mydev.device))
    {
        return PTR_ERR(mydev.device);
    }
    return 0;

cdev_err:
    unregister_chrdev_region(mydev.devid, DEVCNT);
    return -EINVAL;
}

static void __exit time_exit(void)
{
    gpio_set_value(mydev.led_gpio, 1);
    gpio_free(mydev.led_gpio);

    del_timer(&mydev.timer);
    device_destroy(mydev.class, mydev.devid);
    class_destroy(mydev.class);
    cdev_del(&mydev.cdev);
    unregister_chrdev_region(mydev.devid, DEVCNT);
}

module_init(time_init);
module_exit(time_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXJ");