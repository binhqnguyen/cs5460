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
	{ 0x14522340, "module_layout" },
	{ 0x42e80c19, "cdev_del" },
	{ 0x4f1939c7, "per_cpu__current_task" },
	{ 0x5a34a45c, "__kmalloc" },
	{ 0xc45a9f63, "cdev_init" },
	{ 0x6980fe91, "param_get_int" },
	{ 0xfa2e111f, "slab_buffer_size" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0xd691cba2, "malloc_sizes" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0x7edc1537, "device_destroy" },
	{ 0x3758301, "mutex_unlock" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xff964b25, "param_set_int" },
	{ 0x7d11c268, "jiffies" },
	{ 0x3d8a72cd, "mutex_lock_killable" },
	{ 0xffc7c184, "__init_waitqueue_head" },
	{ 0xf85ccdae, "kmem_cache_alloc_notrace" },
	{ 0x4bf79039, "__mutex_init" },
	{ 0xea147363, "printk" },
	{ 0x85f8a266, "copy_to_user" },
	{ 0xb4390f9a, "mcount" },
	{ 0xfee8a795, "mutex_lock" },
	{ 0x2d2cf7d, "device_create" },
	{ 0xa6d1bdca, "cdev_add" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x642e54ac, "__wake_up" },
	{ 0x37a0cba, "kfree" },
	{ 0x33d92f9a, "prepare_to_wait" },
	{ 0xe06bb002, "class_destroy" },
	{ 0x9ccb2622, "finish_wait" },
	{ 0xa2654165, "__class_create" },
	{ 0x3302b500, "copy_from_user" },
	{ 0x29537c9e, "alloc_chrdev_region" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "B9860C54FE7EC48F12996DB");

static const struct rheldata _rheldata __used
__attribute__((section(".rheldata"))) = {
	.rhel_major = 6,
	.rhel_minor = 2,
};
