#include <linux/module.h>
#include <linux/time.h>
static int var = 10;
long int start_time;
module_param(var, int, S_IRUGO);

static int __init first_init(void)
{
int i;
start_time = ktime_get_seconds();
pr_info("Loading first!\n");

for (i = 1; i <= var; i++)		
	pr_info("[%d/%d] Hello!\n",i,var);
return 0;
}



static void __exit first_exit(void)
{
long int end_time;
end_time = ktime_get_seconds();
pr_info("Unloading module after %ld seconds\n",
		
end_time - start_time);
}

module_init(first_init);
module_exit(first_exit);
MODULE_LICENSE("GPL");

MODULE_AUTHOR("Ouajih Safae <safae.ouajih44@gmail.com>");
MODULE_DESCRIPTION("Get Driver WTime");
