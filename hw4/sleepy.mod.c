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
	{ 0x68d372d2, "module_layout" },
	{ 0x3ec8886f, "param_ops_int" },
	{ 0x48eb0c0d, "__init_waitqueue_head" },
	{ 0xc60796c9, "device_create" },
	{ 0x80d78e54, "cdev_add" },
	{ 0xd46f48e8, "cdev_init" },
	{ 0x466cddcf, "__mutex_init" },
	{ 0x34d76c42, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0xe45f60d8, "__wake_up" },
	{ 0x4158396c, "mutex_lock" },
	{ 0x75bb675a, "finish_wait" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x622fa02a, "prepare_to_wait" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0xc5734835, "current_task" },
	{ 0x42119286, "mutex_unlock" },
	{ 0x7d11c268, "jiffies" },
	{ 0x50eedeb8, "printk" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0xea5bac94, "mutex_lock_killable" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x6dcd7881, "class_destroy" },
	{ 0x37a0cba, "kfree" },
	{ 0x8adef804, "cdev_del" },
	{ 0x6d597694, "device_destroy" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "35E1ED0F926775EBB6D0322");
