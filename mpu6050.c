#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Primerov <primerovmax@gmail.com");
MODULE_DESCRIPTION("Driver for output direction device information");
MODULE_VERSION("0.1");

static void __exit mpu6050_exit(void)
{
        pr_info("mpu6050: module exited\n");
}

static int __init mpu6050_init(void)
{
        int ret = 0;

        pr_info("mpu6050: module loaded\n");

        return ret;
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);
