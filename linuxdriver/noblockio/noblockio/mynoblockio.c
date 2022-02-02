#include <linux/module.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/poll.h>

//意识到一件事情定时器这些的开启因该在open里面比较好

#define IRQCNT 1
#define IRQNAME "irqkey"
#define KEYDOWN 1
#define KEYUP 0

struct irq_key{
    int key_gpio;
    struct device_node *nd;
    int irq_num;
    irqreturn_t (*handler)(int, void *);
    struct timer_list timer;
    int key_value;
    atomic_t key_status;
    atomic_t key_flag;
    wait_queue_head_t r_wait;
};


struct irq_dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    struct irq_key irqkey;
};

struct irq_dev irqdev;


static int irq_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &irqdev;
    init_waitqueue_head(&irqdev.irqkey.r_wait);
    atomic_set(&irqdev.irqkey.key_flag,0);
    return 0;
}

static ssize_t irq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = -EINVAL;
    int keyvalue;

    struct irq_dev *dev = filp->private_data;

    keyvalue = KEYDOWN;
    ret = copy_to_user(buf,&keyvalue,sizeof(keyvalue));
    
    

    // atomic_set(&dev->irqkey.key_flag,0);
    // DECLARE_WAITQUEUE(wait, current);

    // if(atomic_read(&dev->irqkey.key_flag) == 0){
    //     add_wait_queue(&dev->irqkey.r_wait,&wait);
    //     set_current_state(TASK_INTERRUPTIBLE);
    //     schedule();
    //     set_current_state(TASK_RUNNING);
    //     remove_wait_queue(&dev->irqkey.r_wait, &wait);
    // }
    // keyvalue = KEYDOWN;


    return ret;  
}

static unsigned int irq_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct irq_dev *dev = filp->private_data;

    poll_wait(filp, &dev->irqkey.r_wait,wait);
    if(atomic_read(&dev->irqkey.key_flag)){
        atomic_set(&dev->irqkey.key_flag,0);
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

static int irq_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static struct file_operations irq_fops = {
    .owner = THIS_MODULE,
    .open = irq_open,
    .read = irq_read,
    .poll = irq_poll,
    .release = irq_release,
};

static irqreturn_t key_handler(int irq, void *dev_id)
{
    struct irq_key *key = (struct irq_key *)dev_id;
    mod_timer(&key->timer, jiffies+msecs_to_jiffies(10));
    return IRQ_RETVAL(IRQ_HANDLED);
}

void timer_function(struct timer_list *t)
{
    //按下为高
    static int keyvalue = 0;
    struct irq_key *dev = from_timer(dev, t, timer);
    dev->key_value = gpio_get_value(dev->key_gpio);

    if((keyvalue==0) && (dev->key_value==0)){
        atomic_set(&dev->key_status, KEYUP);
    }
    if((keyvalue==0) && (dev->key_value==1)){
        atomic_set(&dev->key_status, KEYUP);
        keyvalue = 1;
    }
    if((keyvalue==1) && (dev->key_value==0)){
        atomic_set(&dev->key_status, KEYDOWN);
        atomic_set(&dev->key_flag,1);
        keyvalue=0;
    }
    
    //printk("status:%d\r\n",atomic_read(&dev->key_status));
}

 static int mykey_init(struct irq_key *key){
    int ret;

    timer_setup(&irqdev.irqkey.timer, timer_function, 0);

    key->nd = of_find_node_by_path("/keyirq");
    if(key->nd == NULL){
        printk("key node not find!\r\n");
		return -EINVAL;
    }
    key->key_gpio = of_get_named_gpio(key->nd, "irq-gpios", 0);
    if(key->key_gpio < 0){
        printk("fail to get gpio\r\n");
        return -EINVAL;
    }
    printk("irq gpio:%d\r\n",key->key_gpio);
    gpio_request(key->key_gpio,"key");
    gpio_direction_input(key->key_gpio);

    key->irq_num = gpio_to_irq(key->key_gpio);
    printk("gpio_num:%d  irq_num:%d\r\n",key->key_gpio, key->irq_num);
    key->handler = key_handler;
    ret = request_irq(key->irq_num, key->handler, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                      IRQNAME, key);
    if(ret<0){
        printk("fail to request_irq\r\n");
        gpio_free(key->key_gpio);
        return -EFAULT;
    }

    return 0;

}
 
static int __init myirq_init(void)
{
    int ret;
    if(irqdev.major){
        irqdev.devid = MKDEV(irqdev.major,0);
        ret = register_chrdev_region(irqdev.devid, IRQCNT, IRQNAME);
        if(ret){
            printk("fail to register_cdev\r\n");
            ret = - EINVAL;
            goto register_err;
        }
    }else{
        ret = alloc_chrdev_region(&irqdev.devid, 0, IRQCNT, IRQNAME);
        if(ret == 0){
            irqdev.major = MAJOR(irqdev.devid);
            printk("major:%d\r\n", irqdev.major);
        }else{
            printk("fail to callo_cdev\r\n");
            ret = - EINVAL;
            goto callo_err;
        }
    }

    irqdev.cdev.owner = THIS_MODULE;
    cdev_init(&irqdev.cdev, &irq_fops);
    ret = cdev_add(&irqdev.cdev, irqdev.devid, IRQCNT);
    if(ret < 0){
        printk("fail to cdev_add\r\n");
        goto cdev_err;
    }

    irqdev.class = class_create(THIS_MODULE, IRQNAME);
    if(IS_ERR(irqdev.class)){
        printk("fail to class_create\r\n");
        ret =   PTR_ERR(irqdev.class);
        goto class_err;
    }
    irqdev.device = device_create(irqdev.class, NULL, irqdev.devid, NULL, IRQNAME);
    if(IS_ERR(irqdev.device)){
        printk("fail to device_create\r\n");
        ret = PTR_ERR(irqdev.device);
        goto device_err;
    }

    ret = mykey_init(&irqdev.irqkey);
    if(ret < 0){
        printk("fail to key_init\r\n");
        goto key_init_err;
    }

    return 0;

callo_err:
register_err:
    return ret;

cdev_err:
class_err:
    unregister_chrdev_region(irqdev.devid, IRQCNT); 
    return ret;

key_init_err:
device_err:
    class_destroy(irqdev.class);
    unregister_chrdev_region(irqdev.devid, IRQCNT); 
    return ret;
}

static void __exit myirq_exit(void)
{
    del_timer_sync(&irqdev.irqkey.timer);
    free_irq(irqdev.irqkey.irq_num, &irqdev.irqkey);
    gpio_free(irqdev.irqkey.key_gpio);
    device_destroy(irqdev.class, irqdev.devid);
    class_destroy(irqdev.class);
    unregister_chrdev_region(irqdev.devid, IRQCNT);
}



module_init(myirq_init);
module_exit(myirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXJ");