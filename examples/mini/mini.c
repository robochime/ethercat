/******************************************************************************
 *
 *  m i n i . c
 *
 *  Minimal module for EtherCAT.
 *
 *  $Id$
 *
 *  Copyright (C) 2006  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  The right to use EtherCAT Technology is granted and comes free of
 *  charge under condition of compatibility of product made by
 *  Licensee. People intending to distribute/sell products based on the
 *  code, have to sign an agreement to guarantee that products using
 *  software based on IgH EtherCAT master stay compatible with the actual
 *  EtherCAT specification (which are released themselves as an open
 *  standard) as the (only) precondition to have the right to use EtherCAT
 *  Technology, IP and trade marks.
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include "../../include/ecrt.h" // EtherCAT realtime interface

#define ASYNC
#define FREQUENCY 100

/*****************************************************************************/

struct timer_list timer;

// EtherCAT
ec_master_t *master = NULL;
ec_domain_t *domain1 = NULL;
spinlock_t master_lock = SPIN_LOCK_UNLOCKED;

// data fields
//void *r_ssi_input, *r_ssi_status, *r_4102[3];
void *r_kbus_in;

// channels
uint32_t k_pos;
uint8_t k_stat;

ec_field_init_t domain1_fields[] = {
    {&r_kbus_in, "0", "Beckhoff", "BK1120", "Inputs", 0},
    {}
};

/*****************************************************************************/

void run(unsigned long data)
{
    static unsigned int counter = 0;

    spin_lock(&master_lock);

#ifdef ASYNC
    // receive
    ecrt_master_async_receive(master);
    ecrt_domain_process(domain1);
#else
    // send and receive
    ecrt_domain_queue(domain1);
    ecrt_master_run(master);
    ecrt_master_sync_io(master);
    ecrt_domain_process(domain1);
#endif

    // process data
    //k_pos = EC_READ_U32(r_ssi);

#ifdef ASYNC
    // send
    ecrt_domain_queue(domain1);
    ecrt_master_run(master);
    ecrt_master_async_send(master);
#endif

    spin_unlock(&master_lock);

    if (counter) {
        counter--;
    }
    else {
        counter = FREQUENCY;
        //printk(KERN_INFO "k_pos    = %i\n", k_pos);
        //printk(KERN_INFO "k_stat   = 0x%02X\n", k_stat);
    }

    // restart timer
    timer.expires += HZ / FREQUENCY;
    add_timer(&timer);
}

/*****************************************************************************/

int request_lock(void *data)
{
    spin_lock_bh(&master_lock);
    return 0; // access allowed
}

/*****************************************************************************/

void release_lock(void *data)
{
    spin_unlock_bh(&master_lock);
}

/*****************************************************************************/

int __init init_mini_module(void)
{
    ec_slave_t *slave;

    printk(KERN_INFO "=== Starting Minimal EtherCAT environment... ===\n");

    if ((master = ecrt_request_master(0)) == NULL) {
        printk(KERN_ERR "Requesting master 0 failed!\n");
        goto out_return;
    }

    ecrt_master_callbacks(master, request_lock, release_lock, NULL);

    printk(KERN_INFO "Registering domain...\n");
    if (!(domain1 = ecrt_master_create_domain(master)))
    {
        printk(KERN_ERR "Domain creation failed!\n");
        goto out_release_master;
    }

    printk(KERN_INFO "Registering domain fields...\n");
    if (ecrt_domain_register_field_list(domain1, domain1_fields)) {
        printk(KERN_ERR "Field registration failed!\n");
        goto out_release_master;
    }

#if 1
    printk(KERN_INFO "Setting variable data field sizes...\n");
    if (!(slave = ecrt_master_get_slave(master, "0"))) {
        printk(KERN_ERR "Failed to get slave!\n");
        goto out_deactivate;
    }
    ecrt_slave_field_size(slave, "Inputs", 0, 1);
#endif

    printk(KERN_INFO "Activating master...\n");
    if (ecrt_master_activate(master)) {
        printk(KERN_ERR "Failed to activate master!\n");
        goto out_release_master;
    }

#if 0
    if (ecrt_master_fetch_sdo_lists(master)) {
        printk(KERN_ERR "Failed to fetch SDO lists!\n");
        goto out_deactivate;
    }
    ecrt_master_print(master, 2);
#else
    ecrt_master_print(master, 0);
#endif

#if 0
    if (!(slave = ecrt_master_get_slave(master, "5"))) {
        printk(KERN_ERR "Failed to get slave 5!\n");
        goto out_deactivate;
    }

    if (ecrt_slave_sdo_write_exp8(slave, 0x4061, 1,  0) ||
        ecrt_slave_sdo_write_exp8(slave, 0x4061, 2,  1) ||
        ecrt_slave_sdo_write_exp8(slave, 0x4061, 3,  1) ||
        ecrt_slave_sdo_write_exp8(slave, 0x4066, 0,  0) ||
        ecrt_slave_sdo_write_exp8(slave, 0x4067, 0,  4) ||
        ecrt_slave_sdo_write_exp8(slave, 0x4068, 0,  0) ||
        ecrt_slave_sdo_write_exp8(slave, 0x4069, 0, 25) ||
        ecrt_slave_sdo_write_exp8(slave, 0x406A, 0, 25) ||
        ecrt_slave_sdo_write_exp8(slave, 0x406B, 0, 50)) {
        printk(KERN_ERR "Failed to configure SSI slave!\n");
        goto out_deactivate;
    }
#endif

#ifdef ASYNC
    // send once and wait...
    ecrt_master_prepare_async_io(master);
#endif

#if 1
    if (ecrt_master_start_eoe(master)) {
        printk(KERN_ERR "Failed to start EoE processing!\n");
        goto out_deactivate;
    }
#endif

    printk("Starting cyclic sample thread.\n");
    init_timer(&timer);
    timer.function = run;
    timer.expires = jiffies + 10;
    add_timer(&timer);

    printk(KERN_INFO "=== Minimal EtherCAT environment started. ===\n");
    return 0;

#if 1
 out_deactivate:
    ecrt_master_deactivate(master);
#endif
 out_release_master:
    ecrt_release_master(master);
 out_return:
    return -1;
}

/*****************************************************************************/

void __exit cleanup_mini_module(void)
{
    printk(KERN_INFO "=== Stopping Minimal EtherCAT environment... ===\n");

    if (master) {
        del_timer_sync(&timer);
        printk(KERN_INFO "Deactivating master...\n");
        ecrt_master_deactivate(master);
        ecrt_release_master(master);
    }

    printk(KERN_INFO "=== Minimal EtherCAT environment stopped. ===\n");
}

/*****************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR ("Florian Pose <fp@igh-essen.com>");
MODULE_DESCRIPTION ("EtherCAT minimal test environment");

module_init(init_mini_module);
module_exit(cleanup_mini_module);

/*****************************************************************************/
