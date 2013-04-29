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
	{ 0x89505f22, "__kmap_atomic" },
	{ 0xdbcef96c, "kmem_cache_destroy" },
	{ 0xaa42713a, "iget_failed" },
	{ 0x992847d2, "kmalloc_caches" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x405c1144, "get_seconds" },
	{ 0x6c28e21d, "mark_buffer_dirty_inode" },
	{ 0x270b5d01, "__bread" },
	{ 0xce926d39, "generic_file_llseek" },
	{ 0xb35238ae, "__mark_inode_dirty" },
	{ 0x76ebea8, "pv_lock_ops" },
	{ 0xd0d8621b, "strlen" },
	{ 0xdfb07fe7, "page_address" },
	{ 0x56877f0a, "block_write_begin" },
	{ 0x25820c64, "fs_overflowuid" },
	{ 0xc01cf848, "_raw_read_lock" },
	{ 0x2a74416d, "__lock_page" },
	{ 0x5b8c46a0, "generic_file_aio_read" },
	{ 0x34bc01e0, "block_read_full_page" },
	{ 0x95411160, "end_writeback" },
	{ 0xf1d6c006, "mount_bdev" },
	{ 0xcb53c49e, "generic_read_dir" },
	{ 0x105afe74, "generic_file_aio_write" },
	{ 0xf8e02d44, "__insert_inode_hash" },
	{ 0x2bc95bd4, "memset" },
	{ 0x50eedeb8, "printk" },
	{ 0x7e6dbdd4, "d_rehash" },
	{ 0x5152e605, "memcmp" },
	{ 0x7ba7231b, "find_or_create_page" },
	{ 0x57f5198c, "d_alloc_root" },
	{ 0x6b3c5cc6, "kunmap" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xb4390f9a, "mcount" },
	{ 0xe5dd7ca8, "kmem_cache_free" },
	{ 0x7f658e80, "_raw_write_lock" },
	{ 0xed93f29e, "__kunmap_atomic" },
	{ 0xd3dc5313, "setattr_copy" },
	{ 0xb18def70, "page_symlink" },
	{ 0x47e0468e, "sync_dirty_buffer" },
	{ 0x7cbe1413, "unlock_page" },
	{ 0xfc949717, "__brelse" },
	{ 0xf11543ff, "find_first_zero_bit" },
	{ 0x770eb7a4, "inode_init_once" },
	{ 0x70e4a5d8, "page_follow_link_light" },
	{ 0x4a36829d, "invalidate_inode_buffers" },
	{ 0x1fde756e, "kmem_cache_alloc" },
	{ 0x738803e6, "strnlen" },
	{ 0xd9062079, "generic_file_mmap" },
	{ 0xe2b631e6, "kmap" },
	{ 0x346ffe73, "block_write_full_page" },
	{ 0xf974b989, "block_write_end" },
	{ 0xfa8f68f2, "generic_write_end" },
	{ 0xe449e0a3, "do_sync_read" },
	{ 0x22d06cd3, "unlock_new_inode" },
	{ 0x970611ee, "kill_block_super" },
	{ 0xaf63f22b, "inode_change_ok" },
	{ 0x156b0a42, "kmem_cache_alloc_trace" },
	{ 0x67f7403e, "_raw_spin_lock" },
	{ 0x8a679681, "kmem_cache_create" },
	{ 0x157179bd, "register_filesystem" },
	{ 0xb129c1e0, "iput" },
	{ 0x447ad9cb, "read_cache_page" },
	{ 0x6f36d075, "generic_file_fsync" },
	{ 0x37a0cba, "kfree" },
	{ 0x4f004b50, "do_sync_write" },
	{ 0xddd60fba, "ihold" },
	{ 0x2e60bace, "memcpy" },
	{ 0x50f5e532, "call_rcu_sched" },
	{ 0x64c3533, "vmtruncate" },
	{ 0xf5bd6bb9, "block_truncate_page" },
	{ 0x9f6cdf31, "sb_set_blocksize" },
	{ 0xbafe4e09, "generic_readlink" },
	{ 0x4e3d42fb, "put_page" },
	{ 0xb584ab0d, "__bforget" },
	{ 0x74c134b9, "__sw_hweight32" },
	{ 0xc3f3414e, "__block_write_begin" },
	{ 0xba0ba20c, "mark_buffer_dirty" },
	{ 0x7091194f, "unregister_filesystem" },
	{ 0xd1de94ed, "write_one_page" },
	{ 0xc08f1e1, "init_special_inode" },
	{ 0xeb408b22, "new_inode" },
	{ 0x11c29bca, "generic_file_splice_read" },
	{ 0x52813875, "page_put_link" },
	{ 0x57e2f6a6, "d_instantiate" },
	{ 0x6b470e4, "generic_block_bmap" },
	{ 0x1141b416, "iget_locked" },
	{ 0x3ab09371, "generic_fillattr" },
	{ 0x1d96cee0, "inode_init_owner" },
	{ 0xe914e41e, "strcpy" },
	{ 0x52caa018, "truncate_inode_pages" },
	{ 0xdf929370, "fs_overflowgid" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1EAB42A8E702A62A25C2A73");
