#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Primerov <primerovmax@gmail.com");
MODULE_DESCRIPTION("Driver for output direction device information");
MODULE_VERSION("0.1");

/* Registed addresses */
#define REG_CONFIG		0x1A
#define REG_GYRO_CONFIG		0x1B
#define REG_ACCEL_CONFIG	0x1C
#define REG_FIFO_EN		0x23
#define REG_INT_PIN_CFG		0x37
#define REG_INT_ENABLE		0x38
#define REG_ACCEL_XOUT_H	0x3B
#define REG_ACCEL_XOUT_L	0x3C
#define REG_ACCEL_YOUT_H	0x3D
#define REG_ACCEL_YOUT_L	0x3E
#define REG_ACCEL_ZOUT_H	0x3F
#define REG_ACCEL_ZOUT_L	0x40
#define REG_TEMP_OUT_H		0x41
#define REG_TEMP_OUT_L		0x42
#define REG_GYRO_XOUT_H		0x43
#define REG_GYRO_XOUT_L		0x44
#define REG_GYRO_YOUT_H		0x45
#define REG_GYRO_YOUT_L		0x46
#define REG_GYRO_ZOUT_H		0x47
#define REG_GYRO_ZOUT_L		0x48
#define REG_USER_CTRL		0x6A
#define REG_PWR_MGMT_1		0x6B
#define REG_PWR_MGMT_2		0x6C
#define REG_WHO_AM_I		0x75

/* Register values */
#define MPU6050_WHO_AM_I	0x68


struct mpu6050_data {
        struct i2c_client *client;
        int accel_value[2];
};

static struct task_struct *master_thread;

static struct mutex data_lock;

static struct mpu6050_data mpu6050_data;

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;

	pr_info("mpu6050: i2c client address is 0x%X\n", client->addr);

	err = i2c_smbus_read_byte_data(client, REG_WHO_AM_I);
	if (IS_ERR_VALUE(err)) {
		pr_err("mpu6050: i2c_smbus_read_byte_data() failed with error: %d\n", err);
		goto error;
	}

	if (err != MPU6050_WHO_AM_I) {
		pr_err("mpu6050: wrong i2c device found: expected 0x%X, found 0x%X\n", MPU6050_WHO_AM_I, err);
		err = -1;
		goto error;
	}
	pr_info("mpu6050: i2c mpu6050 device found, WHO_AM_I register value = 0x%X\n", err);

        i2c_smbus_write_byte_data(client, REG_CONFIG, 0);
	i2c_smbus_write_byte_data(client, REG_GYRO_CONFIG, 0);
	i2c_smbus_write_byte_data(client, REG_ACCEL_CONFIG, 0);
	i2c_smbus_write_byte_data(client, REG_FIFO_EN, 0);
	i2c_smbus_write_byte_data(client, REG_INT_PIN_CFG, 0);
	i2c_smbus_write_byte_data(client, REG_INT_ENABLE, 0);
	i2c_smbus_write_byte_data(client, REG_USER_CTRL, 0);
	i2c_smbus_write_byte_data(client, REG_PWR_MGMT_1, 0);
	i2c_smbus_write_byte_data(client, REG_PWR_MGMT_2, 0);

	mpu6050_data.client = client; 

	pr_info("mpu6050: i2c driver probed\n");

	return 0;

error:
	return err;
}

static int mpu6050_remove(struct i2c_client *client)
{
        mpu6050_data.client = 0;

        pr_info("mpu6050: i2c driver removed\n");

        return 0;
}

static const struct i2c_device_id mpu6050_idtable[] = {
        { "mpu6050", 0 },
        {  }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_idtable);

static struct i2c_driver mpu6050_i2c_driver = {
        .driver = {
                .name = "mpu6050",
        },

        .probe = mpu6050_probe,
        .remove = mpu6050_remove,
        .id_table = mpu6050_idtable,
};

static int mpu6050_read_data(void)
{
        int err;

        if (mpu6050_data.client == 0) {
                err = -ENODEV;
                goto error;
        }

        mpu6050_data.accel_value[0] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_data.client, REG_ACCEL_XOUT_H));
	mpu6050_data.accel_value[1] = (s16)((u16)i2c_smbus_read_word_swapped(mpu6050_data.client, REG_ACCEL_YOUT_H));
	
	//pr_info("mpu6050: accel output cord - [%d, %d]", mpu6050_data.accel_value[0],
	//           mpu6050_data.accel_value[1]);

	return 0;

error:
	return err;
}

static int master_fun(void *args)
{
	while(!kthread_should_stop()){

		if(!mutex_trylock(&data_lock)){
		mpu6050_read_data();
		mutex_unlock(&data_lock);
		}
		
		mdelay(10);
	}
	
	return 0;
}

static ssize_t direction_y_show(struct class *class, struct class_attribute *attr, char *buf)
{
		mutex_lock(&data_lock);
	
        if (mpu6050_data.accel_value[1] <= -2000) {
                sprintf(buf, "1\n"); // right
        } else if (mpu6050_data.accel_value[1] >= 1500) {
                sprintf(buf, "0\n"); // left
        } else { 
                sprintf(buf, "-1\n"); // static
        }
		
		mutex_unlock(&data_lock);
		
        return strlen(buf);
}

CLASS_ATTR_RO(direction_y);

static struct class *attr_class;

static void __exit mpu6050_exit(void)
{
		kthread_stop(master_thread);
		
        if (attr_class) { 
		class_remove_file(attr_class, &class_attr_direction_y);
		pr_info("mpu6050: sysfs class attributes removed\n");

		class_destroy(attr_class);
		pr_info("mpu6050: sysfs class destroyed\n");
	}

        i2c_del_driver(&mpu6050_i2c_driver);
        pr_info("mpu6050: i2c driver deleted\n");

        pr_info("mpu6050: module exited\n");
}

static int __init mpu6050_init(void)
{
        int ret;

        ret = i2c_register_driver(THIS_MODULE, &mpu6050_i2c_driver);
        if (ret) {
                pr_err("mpu6050: failed to add new i2c driver: %d\n", ret);
                goto out;
        }
        pr_info("mpu6050: i2c driver created %d\n", ret);
		
		master_thread = kthread_run(master_fun, NULL, "master_thread");

        attr_class = class_create(THIS_MODULE, "mpu6050");
	if (IS_ERR(attr_class)) {
		ret = PTR_ERR(attr_class);
		pr_err("mpu6050: failed to create sysfs class: %d\n", ret);
		goto out;
	}
	pr_info("mpu6050: sysfs class created\n");

	ret = class_create_file(attr_class, &class_attr_direction_y);
	if (ret) {
		pr_err("mpu6050: failed to created sysfs class attribute direction: %d\n", ret);
		goto out;
	}
	pr_info("mpu6050: sysfs class attributes created\n");

        ret = mpu6050_read_data();
        if (ret == -ENODEV) {
                goto out;
        }

        pr_info("mpu6050: module loaded\n");

        return 0;

out:
        return ret;
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);
