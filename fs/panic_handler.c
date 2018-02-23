/*
 * Author: Shilin Victor aka ChronoMonochrome <chrono.monochrome@gmail.com>
 *
 * Copyright 2018 Shilin Victor
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/sysrq.h>

static int panic_handler_panic_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	pr_emerg("kernel panic - rebooting!\n");
	sysrq_handle_reboot(0);

	return NOTIFY_DONE;
}

static struct notifier_block panic_handler_panic_block = {
	.notifier_call  = panic_handler_panic_event,
	.priority       = INT_MAX,
};

static int panic_handler_init(void)
{
	atomic_notifier_chain_register(&panic_notifier_list,
		&panic_handler_panic_block);
	return 0;
}

static void panic_handler_exit(void)
{
	atomic_notifier_chain_unregister(&panic_notifier_list,
		&panic_handler_panic_block);

}

module_init(panic_handler_init);
module_exit(panic_handler_exit);
