#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include "mpu6050.h"





#define MPU6050_CNT	1
#define MPU6050_NAME	"mpu6050"


struct mpu6050_dev {
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备 	 */
	struct device_node	*nd; /* 设备节点 */
	int major;			/* 主设备号 */
	void *private_data;	/* 私有数据 */
};

static struct mpu6050_dev mpu6050dev;

static int mpu6050_write_reg(struct mpu6050_dev *dev, u8 reg, u8 *data,u8 len)
{
	u8 buff[256];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	buff[0] = reg;
	memcpy(&buff[1], data, len);
	msg.addr = client->addr;
	msg.flags = 0;
	msg.buf = buff;
	msg.len = len+1;
	return i2c_transfer(client->adapter, &msg, 1);
}

static int mpu6050_read_reg(struct mpu6050_dev *dev, u8 reg, u8 *data,u8 len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->private_data;

	msg[0].addr = client->addr;
	msg[0].flags  = 0;
	msg[0].buf = &reg;
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags  = I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = 1;

	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret == 2){
		ret = 0;
	}else{
		printk("i2c_error");
		ret = -EREMOTEIO;
	}
	return ret;
}


static int mpu6050_open(struct inode *inode, struct file *filp)
{
	int ret;
	u8 data = 0x80;
	filp->private_data = &mpu6050dev;
	ret = mpu6050_write_reg(&mpu6050dev, MPU_PWR_MGMT1_REG, &data, 1);
	if(ret < 0){
		printk("fail mpu6050_init 0\r\n");
		return -EINVAL;
	}
	mdelay(100);
	data = 0x00;
	ret = mpu6050_write_reg(&mpu6050dev, MPU_PWR_MGMT1_REG, &data, 1);
	if(ret < 0){
		printk("fail mpu6050_init 1\r\n");
		return -EINVAL;
	}
    return 0;
}

static ssize_t mpu6050_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    int ret = 0;
	u8 data;
	struct mpu6050_dev *dev = filp->private_data;

	mpu6050_read_reg(dev, MPU_DEVICE_ID_REG, &data, 1);
	ret = copy_to_user(buf, &data, sizeof(data));

    return ret;
}

static int mpu6050_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations mpu6050_fops = {
	.owner = THIS_MODULE,
	.open = mpu6050_open,
	.read = mpu6050_read,
	.release = mpu6050_release,
};

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk("mpu6050 probe\r\n");

    if (mpu6050dev.major) {
		mpu6050dev.devid = MKDEV(mpu6050dev.major, 0);
		register_chrdev_region(mpu6050dev.devid, MPU6050_CNT, MPU6050_NAME);
	} else {
		alloc_chrdev_region(&mpu6050dev.devid, 0, MPU6050_CNT, MPU6050_NAME);
		mpu6050dev.major = MAJOR(mpu6050dev.devid);
        printk("mpu6050 major:%d\r\n",mpu6050dev.major);
	}

    mpu6050dev.cdev.owner = THIS_MODULE;
    cdev_init(&mpu6050dev.cdev, &mpu6050_fops);
	cdev_add(&mpu6050dev.cdev, mpu6050dev.devid, MPU6050_CNT);

    mpu6050dev.class = class_create(THIS_MODULE, MPU6050_NAME);
	if (IS_ERR(mpu6050dev.class)) {
		return PTR_ERR(mpu6050dev.class);
	}

	/* 4、创建设备 */
	mpu6050dev.device = device_create(mpu6050dev.class, NULL, mpu6050dev.devid, NULL, MPU6050_NAME);
	if (IS_ERR(mpu6050dev.device)) {
		return PTR_ERR(mpu6050dev.device);
	}

    mpu6050dev.private_data = client;

    return 0;
}

static int mpu6050_remove(struct i2c_client *client)
{
    printk("mpu6050 remove\r\n");

    cdev_del(&mpu6050dev.cdev);
	unregister_chrdev_region(mpu6050dev.devid, MPU6050_CNT);

	/* 注销掉类和设备 */
	device_destroy(mpu6050dev.class, mpu6050dev.devid);
	class_destroy(mpu6050dev.class);
    return 0;
}


static const struct of_device_id mpu6050_of_match[] = {
    {.compatible = "CXJ,mpu6050"},
    { }
};

static struct i2c_driver mpu6050_driver = {
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "mpu6050",
        .of_match_table = mpu6050_of_match,
    },
};

static int __init mpu6050_init(void)
{
    return i2c_add_driver(&mpu6050_driver);
}


static void __exit mpu6050_exit(void)
{
    i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXJ");