#include <linux/module.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>

#define DEVNAME "mykey"
#define DEVCNT 1
#define KEY0VALUE		0XFF	/* 按键值 		*/
#define INVAKEY			0X00	/* 无效的按键值  */

struct mykey_dev{
    struct device_node *nd;
    unsigned int key_gpio;
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    atomic_t keyvalue;
};

struct mykey_dev dev;

static int key_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &dev; 	/* 设置私有数据 */

	return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	int value;
	struct mykey_dev *keydev = filp->private_data;

	if (gpio_get_value(keydev->key_gpio) == 0) { 		/* key0按下 */
		while(!gpio_get_value(keydev->key_gpio));		/* 等待按键释放 */
		atomic_set(&keydev->keyvalue, KEY0VALUE);	
	} else {	
		atomic_set(&keydev->keyvalue, INVAKEY);		/* 无效的按键值 */
	}

	value = atomic_read(&keydev->keyvalue);
	ret = copy_to_user(buf, &value, sizeof(value));
	return ret;
}

static int key_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .release = key_release,
};

static int __init mykey_init(void)
{
    int ret;

    atomic_set(&dev.keyvalue, INVAKEY);

    dev.nd = of_find_node_by_path("/key");
    if(dev.nd == NULL){
        printk("node is not find\r\n");
        return -EINVAL;
    }
    dev.key_gpio = of_get_named_gpio(dev.nd, "key-gpios", 0);
    if(dev.key_gpio < 0){
        printk("fail to get key_gpio\r\n");
        return -EINVAL;
    }else{
        printk("key_gpio:%d\r\n",dev.key_gpio);
    }

    if(dev.major){
        dev.devid = MKDEV(dev.major,0);
        ret = register_chrdev_region(dev.devid, DEVCNT,DEVNAME);
        if(ret){
            dev_err(dev.device,"fail to register%d\r\n",dev.major);
            return -EINVAL;
        }
    }else{
        ret = alloc_chrdev_region(&dev.devid,0,DEVCNT,DEVNAME);
        if(ret){
            dev_err(dev.device,"fail to register%d\r\n",dev.major);
            return -EINVAL;
        }else{
            dev.major = MAJOR(dev.devid);
            dev.minor = MINOR(dev.devid);
            printk("major id:%d  minjor id:%d\r\n",dev.major,dev.minor);
        }
    }

    dev.cdev.owner = THIS_MODULE;
    cdev_init(&dev.cdev, &dev_fops);
    ret = cdev_add(&dev.cdev, dev.devid, DEVCNT);
    if(ret){
        printk("fail to cdev_add\r\n");
        goto cdev_err;
        ret = -EINVAL;
    }

    dev.class = class_create(THIS_MODULE, DEVNAME);
    if(IS_ERR(dev.class)){
        printk("fail to class_create\r\n");
        goto class_err;
    }

    dev.device = device_create(dev.class, NULL, dev.devid, NULL, DEVNAME);
    if(IS_ERR(dev.device)){
        printk("fail to device_create\r\n");
        goto device_err;
    }

    ret = gpio_request(dev.key_gpio,"key");
    if(ret){
        printk("fail to gpio_request\r\n");
        goto gpio_err;
    }
    gpio_direction_input(dev.key_gpio);

    return 0;

cdev_err:
    unregister_chrdev_region(dev.devid, DEVCNT);
    return ret;

class_err:
    cdev_del(&dev.cdev);
    unregister_chrdev_region(dev.devid, DEVCNT);
    return PTR_ERR(dev.class);

device_err:
    class_destroy(dev.class);
    cdev_del(&dev.cdev);
    unregister_chrdev_region(dev.devid, DEVCNT);
    return PTR_ERR(dev.device);

gpio_err:
    device_destroy(dev.class,dev.devid);
    class_destroy(dev.class);
    cdev_del(&dev.cdev);
    unregister_chrdev_region(dev.devid, DEVCNT);
    return -EINVAL;
}

static void __exit mykey_exit(void)
{
    gpio_free(dev.key_gpio);
    device_destroy(dev.class, dev.devid);
    class_destroy(dev.class);
    cdev_del(&dev.cdev);
    unregister_chrdev_region(dev.devid, DEVCNT);
}



module_init(mykey_init);
module_exit(mykey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXJ");
