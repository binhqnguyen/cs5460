#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x98397cc5, "module_layout" },
	{ 0x3ec8886f, "param_ops_int" },
	{ 0xf1faf509, "device_create" },
	{ 0x8c71be19, "cdev_add" },
	{ 0x9f4ce64, "cdev_init" },
	{ 0xa62925c6, "__mutex_init" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xd6e17aa6, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x15568631, "lookup_address" },
	{ 0x47d0592e, "mutex_unlock" },
	{ 0x699605d2, "mutex_lock_killable" },
	{ 0x50eedeb8, "printk" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xf62d269d, "class_destroy" },
	{ 0x37a0cba, "kfree" },
	{ 0x47c4a86d, "cdev_del" },
	{ 0x21a21ea4, "device_destroy" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "8B92B3BF3223324889CD63C");
