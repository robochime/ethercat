#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x78c3dd0e, "module_layout" },
	{ 0xb1e5f52f, "ecrt_master_receive" },
	{  0x44971, "kmalloc_caches" },
	{ 0xd2b09ce5, "__kmalloc" },
	{ 0x99c3977e, "ecrt_master_create_domain" },
	{ 0x570b3ef7, "ecrt_master_send" },
	{ 0xee61570b, "ecrt_master" },
	{ 0xc55351a3, "ecrt_sdo_request_state" },
	{ 0xb610ea7f, "ecrt_domain_queue" },
	{ 0xdbcbffd3, "ecrt_master_send_ext" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x3a22cdfd, "ecrt_slave_config_reg_pdo_entry" },
	{ 0x7e1c136d, "ecrt_domain_process" },
	{ 0x41898bf0, "ecrt_domain_state" },
	{ 0x97934ecf, "del_timer_sync" },
	{ 0x949622c0, "ecrt_master_callbacks" },
	{ 0x74cfd8e1, "ecrt_master_state" },
	{ 0x263b603, "ecrt_sdo_request_write" },
	{ 0x7c32d0f0, "printk" },
	{ 0xe1537255, "__list_del_entry_valid" },
	{ 0x9166fada, "strncpy" },
	{ 0xd8d58114, "ecrt_master_slave_config" },
	{ 0x6626afca, "down" },
	{ 0x29d3645f, "ecrt_sdo_request_data" },
	{ 0x24d273d1, "add_timer" },
	{ 0xe7966bff, "ecrt_domain_data" },
	{ 0xe6bc1981, "ecrt_slave_config_pdos" },
	{ 0x34f7d8d5, "ectty_create" },
	{ 0x68f31cbd, "__list_add_valid" },
	{ 0xea7485d, "ectty_tx_data" },
	{ 0x162d15de, "ecrt_slave_config_create_sdo_request" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x5beba012, "ecrt_master_get_slave" },
	{ 0xe6a92219, "kmem_cache_alloc_trace" },
	{ 0x37a0cba, "kfree" },
	{ 0xc6800f7c, "ecrt_master_activate" },
	{ 0x6e77bd96, "ecrt_release_master" },
	{ 0xcf2a6966, "up" },
	{  0x37ec7, "ecrt_request_master" },
	{ 0x28318305, "snprintf" },
	{ 0xc044487b, "ectty_rx_data" },
	{ 0x125085b0, "ectty_free" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=ec_master,ec_tty";

