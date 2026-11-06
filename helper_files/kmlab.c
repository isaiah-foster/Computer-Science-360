#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include "kmlab_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Foster");
MODULE_DESCRIPTION("CPTS360 KM PA");

#define DEBUG 1

#define PROC_DIR_NAME "kmlab"
#define PROC_ENTRY_NAME "status"
#define PROC_ENTRY_MODE 0666
#define UPDATE_PERIOD (5 * HZ)   // Time between CPU time updates, in jiffies
#define PID_BUFFER_SIZE 16       // Large enough for any PID, a newline and a NUL

// store per process data (you may modify it as needed)
typedef struct {
    struct list_head list;
    unsigned int pid;
    unsigned long cpu_time;
} proc_list;

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;
static struct timer_list update_timer;
static struct work_struct update_work;

// Protects process_list, which the /proc callbacks and the work function share
static LIST_HEAD(process_list);
static DEFINE_SPINLOCK(process_list_lock);

// kmlab_proc_read - Reports each registered PID and its CPU time to userspace
static ssize_t kmlab_proc_read(struct file *file, char __user *user_buffer,
                               size_t count, loff_t *offset)
{
    proc_list *entry;
    unsigned long flags;
    size_t length = 0;
    ssize_t result;
    char *buffer;

    // copy_to_user() can sleep, so format into a kernel buffer while holding the lock
    buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;

    spin_lock_irqsave(&process_list_lock, flags);
    list_for_each_entry(entry, &process_list, list) {
        length += scnprintf(buffer + length, PAGE_SIZE - length, "%u: %lu\n",
                            entry->pid, entry->cpu_time);
    }
    spin_unlock_irqrestore(&process_list_lock, flags);

    // Returning 0 once the offset reaches the end tells the reader it has everything
    if (*offset >= length) {
        result = 0;
    } else {
        result = min(count, length - (size_t)*offset);
        if (copy_to_user(user_buffer, buffer + *offset, result))
            result = -EFAULT;
        else
            *offset += result;
    }

    kfree(buffer);
    return result;
}

// kmlab_proc_write - Registers the PID written by a userspace process
static ssize_t kmlab_proc_write(struct file *file, const char __user *user_buffer,
                                size_t count, loff_t *offset)
{
    char buffer[PID_BUFFER_SIZE];
    proc_list *entry;
    unsigned long flags;
    unsigned int pid;

    if (count >= PID_BUFFER_SIZE)
        return -EINVAL;
    if (copy_from_user(buffer, user_buffer, count))
        return -EFAULT;
    buffer[count] = '\0';

    if (kstrtouint(buffer, 10, &pid))
        return -EINVAL;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry)
        return -ENOMEM;
    entry->pid = pid;
    entry->cpu_time = 0;

    spin_lock_irqsave(&process_list_lock, flags);
    list_add_tail(&entry->list, &process_list);
    spin_unlock_irqrestore(&process_list_lock, flags);

    return count;
}

static const struct proc_ops kmlab_proc_ops = {
    .proc_read = kmlab_proc_read,
    .proc_write = kmlab_proc_write,
};

// update_work_function - Updates each process's CPU time and removes terminated processes
static void update_work_function(struct work_struct *work)
{
    proc_list *entry, *next;
    unsigned long flags;

    spin_lock_irqsave(&process_list_lock, flags);
    list_for_each_entry_safe(entry, next, &process_list, list) {
        if (get_cpu_use(entry->pid, &entry->cpu_time) != 0) {
            list_del(&entry->list);
            kfree(entry);
        }
    }
    spin_unlock_irqrestore(&process_list_lock, flags);
}

// update_timer_callback - Defers the update to the workqueue and restarts the single-shot timer
static void update_timer_callback(struct timer_list *timer)
{
    schedule_work(&update_work);
    mod_timer(&update_timer, jiffies + UPDATE_PERIOD);
}

// free_process_list - Frees every registered process; nothing else may be using the list
static void free_process_list(void)
{
    proc_list *entry, *next;

    list_for_each_entry_safe(entry, next, &process_list, list) {
        list_del(&entry->list);
        kfree(entry);
    }
}

// kmlab_init - Called when the module is loaded
static int __init kmlab_init(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE LOADING\n");
   #endif

   proc_dir = proc_mkdir(PROC_DIR_NAME, NULL);
   if (!proc_dir)
      return -ENOMEM;

   proc_entry = proc_create(PROC_ENTRY_NAME, PROC_ENTRY_MODE, proc_dir, &kmlab_proc_ops);
   if (!proc_entry) {
      remove_proc_entry(PROC_DIR_NAME, NULL);
      return -ENOMEM;
   }

   INIT_WORK(&update_work, update_work_function);
   timer_setup(&update_timer, update_timer_callback, 0);
   mod_timer(&update_timer, jiffies + UPDATE_PERIOD);

   pr_info("KMLAB MODULE LOADED\n");
   return 0;
}

// kmlab_exit - Called when the module is unloaded
static void __exit kmlab_exit(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif

   // Remove every user of the list, in dependency order, before freeing it
   remove_proc_entry(PROC_ENTRY_NAME, proc_dir);
   remove_proc_entry(PROC_DIR_NAME, NULL);
   del_timer_sync(&update_timer);
   cancel_work_sync(&update_work);
   free_process_list();

   pr_info("KMLAB MODULE UNLOADED\n");
}

// Register init and exit functions
module_init(kmlab_init);
module_exit(kmlab_exit);
