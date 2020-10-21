#include <linux/module.h> 
#include <linux/random.h> 
#include <linux/init.h>   
#include <linux/kernel.h>  
#include <linux/types.h>  
#include <linux/fs.h>     
#include <linux/uaccess.h>
#include <linux/device.h>
#define DEVICE_NAME "RNGChar" 
#define CLASS_NAME "RNG"      
#define MAX 1000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TOAN");
MODULE_DESCRIPTION("Random number generation Task");
MODULE_VERSION("1.0");

static int majorNumber;
static int timesOfOpens = 0;
static struct class* RNGClass = NULL;
static struct device* RNGDevice = NULL;

static int dev_open(struct inode*, struct file*);
static int dev_release(struct inode*, struct file*);
static ssize_t dev_read(struct file*, char*, size_t, loff_t*);

static struct file_operations fops = 
{
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
};


static int __init RandomGenerator_init(void)
{
	// Dynamically allocate a major number for device file
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber < 0){
		return majorNumber;
	}
		
	// Register the device class
	RNGClass = class_create(THIS_MODULE, CLASS_NAME);
	
	if (IS_ERR(RNGClass)){
		// clean up if there's error
		unregister_chrdev(majorNumber, DEVICE_NAME);
		// return an error on a pointer
		return PTR_ERR(RNGClass);
	}
	
	// Register the device driver
	RNGDevice = device_create(RNGClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(RNGDevice)){
		unregister_chrdev(majorNumber, DEVICE_NAME);
		return PTR_ERR(RNGDevice);
	}
	return 0;
}

static void __exit RandomGenerator_exit(void)
{
	// remove the device
	device_destroy(RNGClass, MKDEV(majorNumber, 0)); 
	// unregister the device class
	class_unregister(RNGClass);
	// remove the device class
	class_destroy(RNGClass);
	// unregister the major number
	unregister_chrdev(majorNumber, DEVICE_NAME);
}

static int dev_open(struct inode *inodep, struct file *filep)
{
	timesOfOpens++;
	printk(KERN_INFO "RNG: Device has been opened %d time(s)\n", timesOfOpens);
	return 0;
}

static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "RNG: Device successfully closed\n");
	return 0;
}

static ssize_t dev_read(struct file *filep, char* usr_space, size_t len, loff_t* offset)
{
	int randNum;
	int error_cnt;
	get_random_bytes(&randNum, sizeof(randNum));
	randNum %= MAX;

	// copy_to_user has the format (*to, *from, size) and returns 0 on success
	error_cnt = copy_to_user(usr_space, &randNum, sizeof(randNum));

	if (error_cnt == 0){
		return 0;
	}
	else{
		return -EFAULT;
	}
}

module_init(RandomGenerator_init);
module_exit(RandomGenerator_exit);

