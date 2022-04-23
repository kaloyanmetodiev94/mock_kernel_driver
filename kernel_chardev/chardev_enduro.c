#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/limits.h>

#define PLUS_VALUE _IOR('a',0,int32_t*)
#define MINUS_VALUE _IOR('a',2,int32_t*)
#define MULTIPLY_VALUE _IOR('a',1,int32_t*)
#define DIVIDE_VALUE _IOR('a',3,int32_t*)


static dev_t first; // Global variable for the first device number
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl; // Global variable for the device class
static void custom_exit(void);
char *memory_buffer;
int16_t memory_buffer_len = 8; // 2x4bytes for the two signed integers
signed int result_val;
signed int *val1;
signed int *val2;


static int custom_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Driver: Device opened\n");
    return 0;
}

static int custom_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Driver: Device released\n");
    return 0;
}

static ssize_t custom_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    return simple_read_from_buffer(buf, len, off, memory_buffer, memory_buffer_len);
}

static ssize_t custom_write(struct file *f, const char __user *buf, size_t len,
    loff_t *off)
{
    if (len > memory_buffer_len){
    	return -EINVAL;
    }
    if(copy_from_user(memory_buffer,buf,len)){
	printk(KERN_WARNING "Data Write : Err!\n");
    }
    return len;
}

static ssize_t custom_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){
       	//memcpy(&val1,memory_buffer, sizeof(val1));
	//memcpy(&val2,memory_buffer+4, sizeof(val2));
	val1=(uint32_t *) &memory_buffer[0];
	val2=(uint32_t *) &memory_buffer[4];
	printk(KERN_INFO "val1: %X val2: %X\n",*val1,*val2);
        switch(cmd) {
                case PLUS_VALUE:
			result_val=*val1+*val2;
			printk(KERN_INFO "Calculating %d + %d\n",*val1,*val2);
                        break;
                case MINUS_VALUE:
			result_val=*val1-*val2;
			printk(KERN_INFO "Calculating %d - %d\n",*val1,*val2);
                        break;
                case MULTIPLY_VALUE:
			result_val=*val1 * *val2;
			printk(KERN_INFO "Calculating %d * %d\n",*val1,*val2);
                        break;
                case DIVIDE_VALUE:
			if (*val2==0){
				printk(KERN_WARNING "Division by 0 cought");
				result_val=0;
			}else{
				result_val=*val1 / *val2;
				printk(KERN_INFO "Calculating %d / %d\n",*val1,*val2);
			}
                        break;
                default:
                        return -ENOTTY;
        }
        if( copy_to_user((int32_t *) arg, &result_val, sizeof(result_val)) )
        {
                printk(KERN_WARNING "Data Read : Err!\n");
        }
        return 0;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = custom_open,
    .release = custom_close,
    .read = custom_read,
    .write = custom_write,
    .unlocked_ioctl = custom_ioctl
};

static int __init custom_init(void) /* Constructor */
{
    int ret;
    struct device *dev_ret;

    printk(KERN_INFO "chardev_enduro module initialising\n");
    if ((ret = alloc_chrdev_region(&first, 0, 1, "chardev_enduro")) < 0) //get device major number
    {
        return ret;
    }
    if (IS_ERR(cl = class_create(THIS_MODULE, "mynull"))) //device class ("generic" maybe?) leave as null for now
    {
        unregister_chrdev_region(first, 1);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, "chardev_enduro"))) //add as device
    {
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(dev_ret);
    }

    cdev_init(&c_dev, &fops);
    printk(KERN_INFO "The major number is %d \n",MAJOR(first));
    if ((ret = cdev_add(&c_dev, first, 1)) < 0)
    {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return ret;
    }

    /* Allocating memory for the buffer */
    memory_buffer = kmalloc(memory_buffer_len, GFP_KERNEL);
    if (!memory_buffer) {
    	ret = -ENOMEM;
     	goto fail;
    }
    	memset(memory_buffer, 0, memory_buffer_len);
    return 0;
    fail:
   	custom_exit();
   	return ret;

}

static void custom_exit(void) /* Destructor */
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "chardev_enduro module removed\n");

    if (memory_buffer){
	kfree(memory_buffer);
    }
}

module_init(custom_init);
module_exit(custom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me_I_guess");

