#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include "led.h"

MODULE_LICENSE("GPL v2");

static int of_led_dts(struct device_node *np, struct led_des *pdes)
{
	int err;
	
	err = of_property_read_u32(np, "pin", &pdes->pin);
	if (err) {
		printk("Not found \"pin\" ""property\n");
		return err;
	}
	
	err = of_property_read_u32(np, "bits", &pdes->bits);
	if (err) {
		printk("Not found \"bits\" ""property\n");
		return err;
	}

	err = of_property_read_u32(np, "mode", &pdes->mode);
	if (err) {
		printk("Not found \"mode\" ""property\n");
		return err;
	}

	err = of_property_read_u32(np, "level", &pdes->level);
	if (err) {
		printk("Not found \"level\" ""property\n");
		return err;
	}

	return 0;
}

//设备和驱动匹配、断开的时候，自动调用
static int led_probe(struct platform_device *pdev)
{
	int err;
	void *regbase;
	struct resource *res;
	struct led_device *pled;
	struct device_node *np = pdev->dev.of_node;
	struct led_des des;
	
	printk("led probe, match device is %s\n", pdev->name);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk("fail to platform_get_resource\n");
		return -ENODEV;
	}

	printk("reg base addr : %#x\n", (unsigned int)res->start);

	//打印物理地址映射的虚拟地址
	regbase = ioremap(res->start, resource_size(res));
	if (!regbase) {
		printk("Fail to ioremap\n");
		return -ENOMEM;
	}

	//解析设备树的属性(reg, pin, bits, mode, level)
	err = of_led_dts(np, &des);
	if (err) {
		printk("Fail to of_led_dts\n");
		return err;
	}

	printk("pin   : %d\n", des.pin);
	printk("bits  : %d\n", des.bits);
	printk("mode  : %d\n", des.mode);
	printk("level : %d\n", des.level);

	pled = register_led_chrdev();
	if (IS_ERR(pled)) {
		printk("Fail to register_led_chrdev\n");
		return PTR_ERR(pled);
	}

	//将pled指针(存放要销毁的led_device结构体)放入pdev来保存
	platform_set_drvdata(pdev, pled);
	pled->regs = regbase;
		
	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	struct led_device *pled = platform_get_drvdata(pdev);

	printk("led remove, match device is %s\n", pdev->name);
	unregister_led_chrdev(pled);
	return 0;
}

static const struct of_device_id led_of_match[] = {
	{ .compatible = "fs4412-led" },
	{ .compatible = "huawei-led" },
	{},
};
MODULE_DEVICE_TABLE(of, led_of_match);

//初始化driver
struct platform_driver led_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name = "fs4412-led",
		.owner = THIS_MODULE,
		.of_match_table = led_of_match,

	}
};

struct class *cls;

//在函数入口注册driver
int led_driver_init(void)
{ 
	//创建一个类:/sys/class/<类名>
	cls = class_create(THIS_MODULE, "led-devices");
	if (IS_ERR(cls)) {
		printk("failed to allocate class\n");
		return PTR_ERR(cls);
	}
	
	platform_driver_register(&led_driver);
	
	return 0;
}

void led_driver_exit(void)
{
	platform_driver_unregister(&led_driver);
	class_destroy(cls);
}

module_init(led_driver_init);
module_exit(led_driver_exit);
