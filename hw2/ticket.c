/*
 * THIS FILE IS ADAPTED FROM:
 *
 * cfake.c - implementation of a simple module for a character device
 * can be used for testing, demonstrations, etc.
 */

/* ========================================================================
 * Copyright (C) 2010-2011, Institute for System Programming
 *                          of the Russian Academy of Sciences (ISPRAS)
 * Authors:
 *      Eugene A. Shatokhin <spectre@ispras.ru>
 *      Andrey V. Tsyvarev  <tsyvarev@ispras.ru>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 ======================================================================== */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

/* Number of devices to create (default: ticket0) */
#ifndef TICKET_NDEVICES
#define TICKET_NDEVICES 1
#endif

/* The structure to represent 'ticket' devices.
 *  ticket_mutex - a mutex to protect the fields of this structure;
 *  cdev - character device structure.
 */
struct ticket_dev {
  struct mutex ticket_mutex;
  struct cdev cdev;
  int ticket_num;	//ticket number
};

MODULE_AUTHOR("Eugene A. Shatokhin / YOUR NAME");
MODULE_LICENSE("GPL");

#define TICKET_DEVICE_NAME "ticket"

/* parameters */
static int ticket_ndevices = TICKET_NDEVICES;

/* ================================================================ */

static unsigned int ticket_major = 0;
static struct ticket_dev *ticket_devices = NULL;
static struct class *ticket_class = NULL;
/* ================================================================ */


int
ticket_open(struct inode *inode, struct file *filp)
{
  unsigned int mj = imajor(inode);
  unsigned int mn = iminor(inode);

  struct ticket_dev *dev = NULL;

  if (mj != ticket_major || mn < 0 || mn >= ticket_ndevices)
    {
      printk(KERN_WARNING "[target] "
	     "No device found with minor=%d and major=%d\n",
	     mj, mn);
      return -ENODEV; /* No such device */
    }

  /* store a pointer to struct ticket_dev here for other methods */
  dev = &ticket_devices[mn];
  filp->private_data = dev;

  if (inode->i_cdev != &dev->cdev)
    {
      printk(KERN_WARNING "[target] open: internal error\n");
      return -ENODEV; /* No such device */
    }

  return 0;
}

int
ticket_release(struct inode *inode, struct file *filp)
{
  return 0;
}

ssize_t
ticket_read(struct file *filp, char __user *buf, size_t count,
	    loff_t *f_pos)
{
  struct ticket_dev *dev = (struct ticket_dev *)filp->private_data;
  ssize_t retval = 0;

  if (mutex_lock_killable(&dev->ticket_mutex))
    return -EINTR;

  // your code starts here
  retval = -EINVAL;
  if (count == 4){	//only when the process reads 4 bytes (an integer).
  	if (copy_to_user(buf, &dev->ticket_num, 4) == 0){	//if copy to user succeeded
  		dev->ticket_num++;
  		retval = count;
  	}
	else	//if copy to user failed, return -1
		retval = -1;
  }
  // your code ends here

  mutex_unlock(&dev->ticket_mutex);
  return retval;
}

ssize_t
ticket_write(struct file *filp, const char __user *buf, size_t count,
	     loff_t *f_pos)
{
  return -EINVAL;
}

loff_t
ticket_llseek(struct file *filp, loff_t off, int whence)
{
  return -EINVAL;
}

struct file_operations ticket_fops = {
  .owner =    THIS_MODULE,
  .read =     ticket_read,
  .write =    ticket_write,
  .open =     ticket_open,
  .release =  ticket_release,
  .llseek =   ticket_llseek,
};

/* ================================================================ */
/* Setup and register the device with specific index (the index is also
 * the minor number of the device).
 * Device class should be created beforehand.
 */
static int
ticket_construct_device(struct ticket_dev *dev, int minor,
			struct class *class)
{
  int err = 0;
  dev_t devno = MKDEV(ticket_major, minor);
  struct device *device = NULL;

  BUG_ON(dev == NULL || class == NULL);

  /* Memory is to be allocated when the device is opened the first time */
  mutex_init(&dev->ticket_mutex);

  cdev_init(&dev->cdev, &ticket_fops);
  dev->cdev.owner = THIS_MODULE;

  err = cdev_add(&dev->cdev, devno, 1);
  if (err)
    {
      printk(KERN_WARNING "[target] Error %d while trying to add %s%d",
	     err, TICKET_DEVICE_NAME, minor);
      return err;
    }

  device = device_create(class, NULL, /* no parent device */
			 devno, NULL, /* no additional data */
			 TICKET_DEVICE_NAME "%d", minor);

  if (IS_ERR(device)) {
    err = PTR_ERR(device);
    printk(KERN_WARNING "[target] Error %d while trying to create %s%d",
	   err, TICKET_DEVICE_NAME, minor);
    cdev_del(&dev->cdev);
    return err;
  }

  //initialize the ticket_num inside ticket_dev
  dev->ticket_num = 1000;

  return 0;
}

/* Destroy the device and free its buffer */
static void
ticket_destroy_device(struct ticket_dev *dev, int minor,
		      struct class *class)
{
  BUG_ON(dev == NULL || class == NULL);
  device_destroy(class, MKDEV(ticket_major, minor));
  cdev_del(&dev->cdev);
  return;
}

/* ================================================================ */
static void
ticket_cleanup_module(int devices_to_destroy)
{
  int i;

  /* Get rid of character devices (if any exist) */
  if (ticket_devices) {
    for (i = 0; i < devices_to_destroy; ++i) {
      ticket_destroy_device(&ticket_devices[i], i, ticket_class);
    }
    kfree(ticket_devices);
  }

  if (ticket_class)
    class_destroy(ticket_class);

  /* [NB] ticket_cleanup_module is never called if alloc_chrdev_region()
   * has failed. */
  unregister_chrdev_region(MKDEV(ticket_major, 0), ticket_ndevices);
  return;
}

static int __init
ticket_init_module(void)
{
  int err = 0;
  int i = 0;
  int devices_to_destroy = 0;
  dev_t dev = 0;
  printk(KERN_INFO "Loading ticket\n");

  if (ticket_ndevices <= 0)
    {
      printk(KERN_WARNING "[target] Invalid value of ticket_ndevices: %d\n",
	     ticket_ndevices);
      err = -EINVAL;
      return err;
    }

  /* Get a range of minor numbers (starting with 0) to work with */
  err = alloc_chrdev_region(&dev, 0, ticket_ndevices, TICKET_DEVICE_NAME);
  if (err < 0) {
    printk(KERN_WARNING "[target] alloc_chrdev_region() failed\n");
    return err;
  }
  ticket_major = MAJOR(dev);

  /* Create device class (before allocation of the array of devices) */
  ticket_class = class_create(THIS_MODULE, TICKET_DEVICE_NAME);
  if (IS_ERR(ticket_class)) {
    err = PTR_ERR(ticket_class);
    goto fail;
  }

  /* Allocate the array of devices */
  ticket_devices = (struct ticket_dev *)kzalloc(
						ticket_ndevices * sizeof(struct ticket_dev),
						GFP_KERNEL);
  if (ticket_devices == NULL) {
    err = -ENOMEM;
    goto fail;
  }

  /* Construct devices */
  for (i = 0; i < ticket_ndevices; ++i) {
    err = ticket_construct_device(&ticket_devices[i], i, ticket_class);
    if (err) {
      devices_to_destroy = i;
      goto fail;
    }
  }
  return 0; /* success */

 fail:
  ticket_cleanup_module(devices_to_destroy);
  return err;
}

static void __exit
ticket_exit_module(void)
{
  ticket_cleanup_module(ticket_ndevices);
  return;
}

module_init(ticket_init_module);
module_exit(ticket_exit_module);
/* ================================================================ */


