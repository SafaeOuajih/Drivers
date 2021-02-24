#include <linux/kernel.h>	
#include <linux/module.h>	
#include <linux/fs.h>
#include <asm/uaccess.h>	


MODULE_AUTHOR("Ouajih Safae");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module for led access");
MODULE_SUPPORTED_DEVICE("none");


volatile uint16_t *PFDIR = (uint16_t *)0xffc00730;
volatile uint16_t *PFDAT = (uint16_t *)0xffc00700;

#define LED_MASK (0x3f << 6)
#define LED_BASE 6


#define MAJOR_NUM 		    100
#define DEVICE_FILE_NAME 	  "leds"
#define DEVICE_NAME 		  "leds"

#undef DEBUG
#undef DEBUG2

static int Device_Open = 0;
static char buf[1];

static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
  printk(KERN_ALERT "device_open(%p)\n", file);
#endif

  if (Device_Open)
    return -EBUSY;

  Device_Open++;

  try_module_get(THIS_MODULE);
  return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
  printk(KERN_ALERT "device_release(%p,%p)\n", inode, file);
#endif

  Device_Open--;

  module_put(THIS_MODULE);

  return 0;
}

static ssize_t device_write(struct file *file, const char __user * buffer, size_t count, loff_t * offset)
{

#ifdef DEBUG
  printk(KERN_ALERT "device_write(%p,%s,%d)", file, buffer, count);
#endif
  if (copy_from_user(buf, buffer, count))               
    return -EFAULT;                                           

#ifdef DEBUG2
  printk(KERN_ALERT "data=%x", buf[0]);
#endif

  buf[0] = 0x00; // ALL +
  return count; 
}

struct file_operations Fops = {
  .read = NULL,
  .write = device_write,
  .ioctl = NULL,
  .open = device_open,
  .release = device_release,	
};

static int device_init(void)
{
  int ret_val;

  ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

  if (ret_val < 0) {
    printk(KERN_ALERT "%s failed with %d\n",
           "Sorry, registering the character device ", ret_val);
    return ret_val;
  }

  printk(KERN_ALERT "Leds registration success. The major is %d.\n", MAJOR_NUM);
  printk(KERN_ALERT "# mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);

  *PFDIR |= LED_MASK; // out config

  return 0;
}

static void device_exit(void)
{
  unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}
module_init(device_init);
module_exit(device_exit);
