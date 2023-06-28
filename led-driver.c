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

//�豸������ƥ�䡢�Ͽ���ʱ���Զ�����
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

	//��ӡ�����ַӳ��������ַ
	regbase = ioremap(res->start, resource_size(res));
	if (!regbase) {
		printk("Fail to ioremap\n");
		return -ENOMEM;
	}

	//�����豸��������(reg, pin, bits, mode, level)
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

	//��pledָ��(���Ҫ���ٵ�led_device�ṹ��)����pdev������
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

//��ʼ��driver
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

//�ں������ע��driver
int led_driver_init(void)
{ 
	//����һ����:/sys/class/<����>
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
