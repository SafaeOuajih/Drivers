#include <linux/module.h>
static int __init hello_init(void)
{
pr_info("Hello world my first driver: Safae :) \n");
return 0;
}
static void __exit hello_exit(void)
{
pr_info("Hello world Bye Bye :-( \n");
}
module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");

MODULE_AUTHOR("Ouajih Safae <safae.ouajih44@gmail.com>");
MODULE_DESCRIPTION("My first HelloWorld driver");
