#include <linux/module.h>
static int var = 1;
module_param(var, int, S_IRUGO);

static int __init hello_init(void)
{
pr_info("Var= %d\n", var);
return 0;
}

static void __exit hello_exit(void)
{
pr_info("Parameter exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");

MODULE_AUTHOR("Ouajih Safae <safae.ouajih44@gmail.com>");
MODULE_DESCRIPTION("Trying to manage variables");
