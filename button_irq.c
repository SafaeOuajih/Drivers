/***************************************************************************//**
Driver for button 

*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  
#include <linux/interrupt.h> // irq
#include <linux/gpio.h>     

MODULE_AUTHOR("Ouajih Safae");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module for button/gpio access _ interrupt ");
MODULE_SUPPORTED_DEVICE("Tested on Rasp3b+");


#define GPIO_23 (23)
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev button_cdev;
 
static int __init button_driver_init(void);
static void __exit button_driver_exit(void);
 
 
/*************** Driver Fuctions **********************/
static int button_open(struct inode *inode, struct file *file);
static int button_release(struct inode *inode, struct file *file);
static ssize_t button_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t button_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
/******************************************************/
static int value = 0;

//req handler
static irqreturn_t handler(int irq, void* ident){
	 value = value +1;
	return IRQ_HANDLED;
}

//File operation structure 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = button_read,
  .write          = button_write,
  .open           = button_open,
  .release        = button_release,
};
 
static int button_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened : button \n");
  return 0;
}
 
static int button_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed : button \n");
  return 0;
}
 
static ssize_t button_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;
  gpio_state = gpio_get_value(GPIO_23);
  len = 1;
  if( copy_to_user(buf, &gpio_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }
  pr_info("Read function : GPIO_23 = %d \n", gpio_state);
  return 0;
}

static ssize_t button_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
  pr_err("Idiot ! what are you doing ! :) \n");
  
  return 0;
}
 
static int __init button_driver_init(void)
{
  /*Allocating Major number*/
  if((alloc_chrdev_region(&dev, 0, 1, "button_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

  /*Creating cdev structure*/
  cdev_init(&button_cdev,&fops);

  /*Adding character device to the system*/
  if((cdev_add(&button_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }

  /*Creating struct class*/
  if((dev_class = class_create(THIS_MODULE,"button_class")) == NULL){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }

  /*Creating device*/
  if((device_create(dev_class,NULL,dev,NULL,"button_device")) == NULL){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
  
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_23) == false){
        pr_err("GPIO %d is not valid\n", GPIO_23);
        goto r_device;
    }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_23,"GPIO_23") < 0){
        pr_err("ERROR: GPIO %d request\n", GPIO_23);
        goto r_gpio;
    }
  
  //configure the GPIO as input
  gpio_direction_input(GPIO_23);
  int err=0;
if((err = request_irq(gpio_to_irq(GPIO_21),handler,IRQF_SHARED | IRQF_TRIGGER_RISING,THIS_MODULE->name,THIS_MODULE->name) ) !=0){
	goto r_gpio;
}
  gpio_export(GPIO_23, false);
  
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;

r_gpio:
  gpio_free(GPIO_23);
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&button_cdev);
r_unreg:
  unregister_chrdev_region(dev,1);
  
  return -1;
}
 
void __exit button_driver_exit(void)
{
  gpio_unexport(GPIO_23);
  gpio_free(GPIO_23);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&button_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info(KERN_INFO "Device Driver Remove...Done!!\n");
}
 
module_init(button_driver_init);
module_exit(button_driver_exit);
 
