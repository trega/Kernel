#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/slab.h>
#include <asm/uaccess.h>

 
MODULE_LICENSE("GPL");              ///< The license type -- this affects runtime behavior
MODULE_AUTHOR("trega");      ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("Exercice slab allocator");  ///< The description -- see modinfo
MODULE_VERSION("0.1");              ///< The version of the module
 
static char *name = "SlabAllocDrv";
module_param(name, charp, S_IRUGO); ///< Param desc. charp = char ptr, S_IRUGO can be read/not changed
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");  ///< parameter description

#define TAG "SlabAlloc"

#define  DEVICE_NAME TAG    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "slaballoc_c"        ///< The device class -- this is a character device driver

static int    majorNumber;
static struct class*  slabAllocClass  = NULL; ///< The device-driver class struct pointer
static struct device* slabAllocDevice = NULL;
static struct kmem_cache *slabAllocCache;


static int     dev_open(struct inode * anode, struct file * afile);
static int     dev_release(struct inode * anode, struct file * afile){return 0;}
static ssize_t dev_read(struct file * afile, char * acar, size_t aa, loff_t * asize);
static ssize_t dev_write(struct file * adile, const char * achar, size_t aa, loff_t * asize);

struct SlabAllocDevice {
	char msg[255];
};

struct SlabAllocDevice* slabDev;


static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};
 
static int __init SlabAllocDrv_init(void){
   	printk(KERN_INFO "%s: Entering: %s, module name: %s\n", TAG,  __FUNCTION__, name);

	// Try to dynamically allocate a major number for the device -- more difficult but worth it
   	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   	if (majorNumber<0){
      	printk(KERN_ALERT "%s: failed to register a major number\n", TAG);
      	return majorNumber;
   	}
   	printk(KERN_INFO "%s: registered correctly with major number %d\n", TAG, majorNumber);

	// Register the device class
   	slabAllocClass = class_create(THIS_MODULE, CLASS_NAME);
   	if (IS_ERR(slabAllocClass)){                // Check for error and clean up if there is
    	unregister_chrdev(majorNumber, DEVICE_NAME);
      	printk(KERN_ALERT "TAG: Failed to register device class\n");
      	return PTR_ERR(slabAllocClass);          // Correct way to return an error on a pointer
   	}
   	printk(KERN_INFO "%s: device class registered correctly\n", TAG);

	// Register the device driver
   	slabAllocDevice = device_create(slabAllocClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   	if (IS_ERR(slabAllocDevice)){               // Clean up if there is an error
      	class_destroy(slabAllocClass);           // Repeated code but the alternative is goto statements
      	unregister_chrdev(majorNumber, DEVICE_NAME);
      	printk(KERN_ALERT "%s: Failed to create the device\n",TAG);
      	return PTR_ERR(slabAllocDevice);
   	}
   	printk(KERN_INFO "%s: device created correctly\n", TAG); // Made it! device was initialized

	slabAllocCache = kmem_cache_create("Slab Alloc cache", sizeof(struct SlabAllocDevice),
                      0, 0, NULL);
    if (slabAllocCache == NULL){
        printk(KERN_ALERT "%s Couldn't allocate SlabAlloc caches\n", TAG);
		return PTR_ERR(slabAllocDevice);
	}
	else
		printk(KERN_INFO "%s slabAllocCache allocated succesfuly, size:%u\n", TAG, sizeof(struct SlabAllocDevice));
		
	slabDev = kmem_cache_alloc(slabAllocCache, GFP_KERNEL);
	if(slabDev == NULL){
		printk(KERN_ALERT "%s Couldn't allocate slabDev\n", TAG);
        return PTR_ERR(slabAllocDevice);
	}
	
	strcpy(slabDev->msg, "Initial message");	

   	return 0;
}
 
static void __exit SlabAllocDrv_exit(void){
   	printk(KERN_INFO "%s: Enterring %s, module name: %s\n", TAG, __FUNCTION__, name);
	kmem_cache_free(slabAllocCache, slabDev);
	device_destroy(slabAllocClass, MKDEV(majorNumber, 0));     // remove the device
   	class_unregister(slabAllocClass);                          // unregister the device class
   	class_destroy(slabAllocClass);                             // remove the device class
   	unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
	kmem_cache_destroy(slabAllocCache);
	
   	printk(KERN_INFO "%s: Goodbye from the LKM!\n", TAG);
}


static int dev_open(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "%s: Device has been opened\n", TAG);
   	return 0;
}


ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int error_count = 0;
	unsigned size_of_message = strlen(slabDev->msg);
   	// copy_to_user has the format ( * to, *from, size) and returns 0 on success
   	error_count = copy_to_user(buffer, slabDev->msg, size_of_message);
 
   	if (error_count==0){            // if true then have success
      	printk(KERN_INFO "%s: Sent %d characters to the user\n",TAG, size_of_message);
      	return (size_of_message=0);  // clear the position to the start and return 0
   	}
   	else {
      	printk(KERN_INFO "%s: Failed to send %d characters to the user\n", TAG, error_count);
      	return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
  	}
	return error_count;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	unsigned size = copy_from_user (slabDev->msg, buffer, len);
	sprintf(slabDev->msg+len,"%s", "\0");
   	printk(KERN_DEBUG "%s: Received %d characters from the user: %s\n", TAG, strlen(slabDev->msg), slabDev->msg);
   	return len;
}

module_init(SlabAllocDrv_init);
module_exit(SlabAllocDrv_exit);
