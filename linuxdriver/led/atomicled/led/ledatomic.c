#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/types.h>


#define LED_CNT     1
#define LED_NAME    "gpioled"
#define LEDOFF      1
#define LEDON       0

struct gpioled_dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int led_gpio;
    atomic_t lock;
};

struct gpioled_dev gpioled;

static int led_open(struct inode *inode, struct file *filp)
{
    if(!atomic_dec_and_test(&gpioled.lock)){
        atomic_inc(&gpioled.lock);
        return -EBUSY;
    }
	filp->private_data = &gpioled; /* 设置私有数据 */
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;
    struct gpioled_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0]; /* 获取状态值 */

    if (ledstat == LEDON)
    {
        gpio_set_value(dev->led_gpio, 0); /* 打开LED灯 */
    }
    else if (ledstat == LEDOFF)
    {
        gpio_set_value(dev->led_gpio, 1); /* 关闭LED灯 */
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev = filp->private_data;
    gpio_set_value(dev->led_gpio,1);
    atomic_inc(&gpioled.lock);
	return 0;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .write = led_write,
    .open = led_open,
    .release = led_release,

};

static int __init led_init(void)
{
    int err;
    atomic_set(&gpioled.lock,1);
     /*设备节点*/
    gpioled.nd = of_find_node_by_path("/led");
    if(gpioled.nd == NULL){
        printk("gpioled node not find\r\n");
        return -EINVAL;
    }
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd,"led-gpios",0);
    if(gpioled.led_gpio < 0){
        printk("can't get led-gpio\r\n");
        return -EINVAL;
    }
    /*注册设备驱动*/
    gpioled.major = 0;
    if(gpioled.major){
        gpioled.devid = MKDEV(gpioled.major,0); //创建设备号
        err = register_chrdev_region(gpioled.devid,LED_CNT,LED_NAME);//注册
        if(err){
            dev_err(gpioled.device,"Error geting major:%d",gpioled.major);
            goto err_register;
        }
    }else{
        err = alloc_chrdev_region(&gpioled.devid,0,LED_CNT,LED_NAME);
        if(err){
            dev_err(gpioled.device,"Error geting major:%d",gpioled.major);
            goto err_register;
        }
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }
    printk("gpioled major:%d minor:%d\r\n",gpioled.major,gpioled.minor);
    /*初始化cdev*/
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev,&led_fops);
    err = cdev_add(&gpioled.cdev,gpioled.devid,LED_CNT);
    if(err < 0){
        goto err_cdev;
    }
    /*创建类*/
    gpioled.class = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(gpioled.class)){
        return PTR_ERR(gpioled.class);
    }
    /*创建设备*/
    gpioled.device = device_create(gpioled.class,NULL,gpioled.devid,NULL,LED_NAME);
    if(IS_ERR(gpioled.class)){
        return PTR_ERR(gpioled.device);
    }
    printk("led_gpio:%d\r\n",gpioled.led_gpio);
    /*io*/
    gpio_request(gpioled.led_gpio,"led-gpios");
    gpio_direction_output(gpioled.led_gpio,1);

    gpio_set_value(gpioled.led_gpio,0);

    return 0;

err_register:
    return -EINVAL;
err_cdev:
    unregister_chrdev_region(gpioled.devid,LED_CNT);
    return -EINVAL;
}



static void __exit led_exit(void)
{
    gpio_set_value(gpioled.led_gpio,1);
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid,LED_CNT);
    device_destroy(gpioled.class,gpioled.devid);
    class_destroy(gpioled.class);
    gpio_free(gpioled.led_gpio);


}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXJ");