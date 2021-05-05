/***************************************************************************//**

*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/interrupt.h> // irq
#include <linux/gpio.h>     //GPIO
#include<linux/slab.h>                 //kmalloc()
#include <linux/kthread.h>             //kernel threads
#include <linux/sched.h>               //task_struct 
#include <linux/mutex.h>

//LED is connected to this GPIO
#define GPIO_21 (23)
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
 
 
 

unsigned long etx_global_variable = 0;
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
////////////////////////////M_Threading///////////////////////////////
static struct task_struct *etx_thread1; 
static struct task_struct *etx_thread2; 
struct mutex etx_mutex; 
/*************** Driver Fuctions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
/******************************************************/


 ////////////////////////////M_Threading///////////////////////////////
int thread_function1(void *pv);  
int thread_function2(void *pv);  
/*
** Thread function 1
*/
int thread_function1(void *pv)
{
    
    while(!kthread_should_stop()) {
        mutex_lock(&etx_mutex);
        etx_global_variable++;
        pr_info("In EmbeTronicX Thread Function1 %lu\n", etx_global_variable);
        mutex_unlock(&etx_mutex);
        msleep(1000);
    }
    return 0;
}
/*
** Thread function 2
*/
int thread_function2(void *pv)
{
    while(!kthread_should_stop()) {
        mutex_lock(&etx_mutex);
        etx_global_variable++;
        pr_info("In EmbeTronicX Thread Function2 %lu\n", etx_global_variable);
        mutex_unlock(&etx_mutex);
        msleep(1000);
    }
    return 0;
}

/////////////////////////M_Threading//////////////////////////////////


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
  .read           = etx_read,
  .write          = etx_write,
  .open           = etx_open,
  .release        = etx_release,
};
 
static int etx_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}
 
static int etx_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}
 
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{

  uint8_t gpio_state = 0;
  
  //reading GPIO value
 //   gpio_state = gpio_get_value(GPIO_21);
  
  //write to user
  len = 1;
//  if( copy_to_user(buf, &value, len) > 0) {
 //   pr_err("ERROR: Not all the bytes have been copied to user\n");
//  }
  
  pr_info("Read function : value = %d \n", value);


	  
  return 0;
}

static ssize_t etx_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};
  
  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_21 Set = %c\n", rec_buf[0]);
  
  if (rec_buf[0]=='1') {
    //set the GPIO value to HIGH
        gpio_set_value(GPIO_21, 1);
    } else if (rec_buf[0]=='0') {
    //set the GPIO value to LOW
        gpio_set_value(GPIO_21, 0);
    }  else {
        pr_err("Unknown command : Please provide either 1 or 0 \n");
  }
  
  return len;
}
 
static int __init etx_driver_init(void)
{
  /*Allocating Major number*/
  if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

  /*Creating cdev structure*/
  cdev_init(&etx_cdev,&fops);

  /*Adding character device to the system*/
  if((cdev_add(&etx_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }

  /*Creating struct class*/
  if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }

  /*Creating device*/
  if((device_create(dev_class,NULL,dev,NULL,"etx_device")) == NULL){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
  
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_21) == false){
        pr_err("GPIO %d is not valid\n", GPIO_21);
        goto r_device;
    }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_21,"GPIO_21") < 0){
        pr_err("ERROR: GPIO %d request\n", GPIO_21);
        goto r_gpio;
    }
  
  //configure the GPIO as output
  gpio_direction_input(GPIO_21);
  
  /* Using this call the GPIO 21 will be visible in /sys/class/gpio/
  ** Now you can change the gpio values by using below commands also.
  ** echo 1 > /sys/class/gpio/gpio21/value  (turn ON the LED)
  ** echo 0 > /sys/class/gpio/gpio21/value  (turn OFF the LED)
  ** cat /sys/class/gpio/gpio21/value  (read the value LED)
  ** 
  ** the second argument prevents the direction from being changed.
  */
int err=0;
if((err = request_irq(gpio_to_irq(GPIO_21),handler,IRQF_SHARED | IRQF_TRIGGER_RISING,THIS_MODULE->name,THIS_MODULE->name) ) !=0){

	goto r_gpio;	

}
  gpio_export(GPIO_21, false);
 
///////////////////////////M_Threading////////////////////////////////
        etx_thread1 = kthread_create(thread_function,NULL,"eTx Thread1");
        if(etx_thread1) {
            wake_up_process(etx_thread1);
        } else {
            pr_err("Cannot create kthread1\n");
            goto r_device;
        }
        etx_thread2 = kthread_create(thread_function,NULL,"eTx Thread2");
        if(etx_thread2) {
            wake_up_process(etx_thread2);
        } else {
            pr_err("Cannot create kthread2\n");
            goto r_device;
        }
#if 0
        /* You can use this method also to create and run the thread */
        etx_thread = kthread_run(thread_function,NULL,"eTx Thread");
        if(etx_thread) {
            pr_info("Kthread Created Successfully...\n");
        } else {
            pr_err("Cannot create kthread\n");
             goto r_device;
        }
#endif 
/////////////////////////////M_Threading////////////////////////////// 
 
  
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;

r_gpio:
  gpio_free(GPIO_21);
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&etx_cdev);
r_unreg:
  unregister_chrdev_region(dev,1);
  
  return -1;
}
 
void __exit etx_driver_exit(void)
{
  kthread_stop(etx_thread1);
  kthread_stop(etx_thread2);
  gpio_unexport(GPIO_21);
  gpio_free(GPIO_21);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&etx_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info(KERN_INFO "Device Driver Remove...Done!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.32");
