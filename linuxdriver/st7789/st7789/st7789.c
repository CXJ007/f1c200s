#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include "st7789bus.h"



#define ST7789_CNT	1
#define ST7789_NAME	"st7789"

struct st7789_dev st7789dev;

static struct file_operations st7789_fops = {
	.owner = THIS_MODULE,
};

static int st7789_probe(struct platform_device *dev)
{
	int ret;
	printk("st7789_probe\r\n");
	ret = alloc_chrdev_region(&st7789dev.devid, 0, ST7789_CNT, ST7789_NAME);
	if(ret < 0){
		printk("devid_err\r\n");
		return -EINVAL;
	}
	st7789dev.major = MAJOR(st7789dev.devid);
	printk("st7789 major:%d\r\n",st7789dev.major);

	cdev_init(&st7789dev.cdev, &st7789_fops);
	cdev_add(&st7789dev.cdev, st7789dev.devid, ST7789_CNT);

	st7789dev.class = class_create(THIS_MODULE, ST7789_NAME);
	if(IS_ERR(st7789dev.class)){
		return PTR_ERR(st7789dev.class);
	}

	st7789dev.device = device_create(st7789dev.class, NULL, st7789dev.devid, NULL, ST7789_NAME);
	if(IS_ERR(st7789dev.device)){
		return PTR_ERR(st7789dev.device);
	}

	st7789_gpio_request(&st7789dev);

	Lcd_Init();
	LCD_Clear(0x07E0);

	return 0;
}

static int st7789_remove(struct platform_device *dev)
{
	int i;
	printk("st7789_remove\r\n");

	gpio_free(st7789dev.res);
	gpio_free(st7789dev.cs);
	gpio_free(st7789dev.dc);
	gpio_free(st7789dev.wr);
	gpio_free(st7789dev.rd);
	for(i=0;i<8;i++){
		gpio_free(st7789dev.db[i]);
	}

	device_destroy(st7789dev.class, st7789dev.devid);
	class_destroy(st7789dev.class);
	cdev_del(&st7789dev.cdev);
	unregister_chrdev_region(st7789dev.devid, ST7789_CNT);

	return 0;
}

static struct of_device_id st7789_of_match[] = {
	{.compatible = "cxj,8080"},
	{}
};

static struct platform_driver st7789_driver = {
	.probe = st7789_probe,
	.remove = st7789_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "st7789",
		.of_match_table = st7789_of_match,
	},

};

static int __init st7789_init(void)
{
    return platform_driver_register(&st7789_driver);
}


static void __exit st7789_exit(void)
{
    platform_driver_unregister(&st7789_driver);
}

module_init(st7789_init);
module_exit(st7789_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXJ");