/* kernel/power/earlysuspend.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
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

#include <linux/earlysuspend.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rtc.h>
#include <linux/syscalls.h> /* sys_sync */
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/kallsyms.h>	                     /* Jagan+ ... Jagan- */
#include "power.h"

enum {
	DEBUG_USER_STATE = 1U << 0,
	DEBUG_SUSPEND = 1U << 2,
};
static int debug_mask = DEBUG_USER_STATE;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static DEFINE_MUTEX(early_suspend_lock);
static LIST_HEAD(early_suspend_handlers);
static void early_suspend(struct work_struct *work);
static void late_resume(struct work_struct *work);
static DECLARE_WORK(early_suspend_work, early_suspend);
static DECLARE_WORK(late_resume_work, late_resume);
static DEFINE_SPINLOCK(state_lock);
enum {
	SUSPEND_REQUESTED = 0x1,
	SUSPENDED = 0x2,
	SUSPEND_REQUESTED_AND_SUSPENDED = SUSPEND_REQUESTED | SUSPENDED,
};
static int state;
// Jagan+
static void earlysuspend_timeout(unsigned long data);
static DEFINE_TIMER(earlysuspend_wd, earlysuspend_timeout, 0, 0);
static struct task_struct *suspend_task = NULL;

#ifdef CONFIG_SPEEDUP_KEYRESUME
	struct sched_param earlysuspend_s = { .sched_priority = 66 };
	struct sched_param earlysuspend_v = { .sched_priority = 0 };
	int earlysuspend_old_prio = 0;
	int earlysuspend_old_policy = 0;
#endif
// Jagan-

void register_early_suspend(struct early_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&early_suspend_lock);
	list_for_each(pos, &early_suspend_handlers) {
		struct early_suspend *e;
		e = list_entry(pos, struct early_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	if ((state & SUSPENDED) && handler->suspend)
		handler->suspend(handler);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(register_early_suspend);

void unregister_early_suspend(struct early_suspend *handler)
{
	mutex_lock(&early_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(unregister_early_suspend);
// Jagan+
static void earlysuspend_timeout(unsigned long data)
{
	print_symbol("*** EARLYSUSPEND/LATERESUME timeout: %s\n", data);
	if (suspend_task) show_stack(suspend_task, NULL);
}

static void earlysuspend_wdset(void *func)
{
	earlysuspend_wd.data = (unsigned long)func;
	mod_timer(&earlysuspend_wd, jiffies + HZ*3);
}

static void earlysuspend_wdclr(void *func)
{
	del_timer_sync(&earlysuspend_wd);
}
// Jagan-
static void early_suspend(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
	if (!suspend_task) suspend_task = current;	   /* Jagan+ ... Jagan- */

	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED)
		state |= SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("early_suspend: abort, state %d\n", state);
		mutex_unlock(&early_suspend_lock);
		goto abort;
	}

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("early_suspend: call handlers\n");
	list_for_each_entry(pos, &early_suspend_handlers, link) {
		if (pos->suspend != NULL) {	/* Jagan+  */
			earlysuspend_wdset(pos->suspend);
			pos->suspend(pos);
		earlysuspend_wdclr(NULL);
		}							/* Jagan- */
	}
	mutex_unlock(&early_suspend_lock);

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("early_suspend: sync\n");
// Jagan+
#ifdef CONFIG_SPEEDUP_RESUME_NOTDOSYNC
#else
	sys_sync();
#endif
// Jagan-
	
abort:
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED)
		wake_unlock(&main_wake_lock);
	spin_unlock_irqrestore(&state_lock, irqflags);
}

static void late_resume(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
#ifdef CONFIG_SPEEDUP_KEYRESUME				// Jagan+
 	earlysuspend_old_prio = current->rt_priority;
 	earlysuspend_old_policy = current->policy;

 	/* just for this write, set us real-time */
 	if (!(unlikely(earlysuspend_old_policy == SCHED_FIFO) || unlikely(earlysuspend_old_policy == SCHED_RR))) {
		if ((sched_setscheduler(current, SCHED_RR, &earlysuspend_s)) < 0)
   			printk(KERN_ERR "[SEAN]up late_resume failed\n");
   	}
#endif

	if (!suspend_task) suspend_task = current;  // Jagan-

	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPENDED)
		state &= ~SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("late_resume: abort, state %d\n", state);
		goto abort;
	}
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: call handlers\n");
	list_for_each_entry_reverse(pos, &early_suspend_handlers, link)
		if (pos->resume != NULL) {
			earlysuspend_wdset(pos->resume);	// Jagan+
			pos->resume(pos);
			earlysuspend_wdclr(NULL);			// Jagan-	
		}
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: done\n");
abort:
	mutex_unlock(&early_suspend_lock);
#ifdef CONFIG_SPEEDUP_KEYRESUME				// Jagan+
 	if (!(unlikely(earlysuspend_old_policy == SCHED_FIFO) || unlikely(earlysuspend_old_policy == SCHED_RR))) {
  		earlysuspend_v.sched_priority = earlysuspend_old_prio;
  		if ((sched_setscheduler(current, earlysuspend_old_policy, &earlysuspend_v)) < 0)
   			printk(KERN_ERR "[SEAN]down late_resume failed\n");
  	}
#endif										// Jagan-
}

void request_suspend_state(suspend_state_t new_state)
{
	unsigned long irqflags;
	int old_sleep;

	spin_lock_irqsave(&state_lock, irqflags);
	old_sleep = state & SUSPEND_REQUESTED;
	if (debug_mask & DEBUG_USER_STATE) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		pr_info("request_suspend_state: %s (%d->%d) at %lld "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
			new_state != PM_SUSPEND_ON ? "sleep" : "wakeup",
			requested_suspend_state, new_state,
			ktime_to_ns(ktime_get()),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
	if (!old_sleep && new_state != PM_SUSPEND_ON) {
		state |= SUSPEND_REQUESTED;
		queue_work(suspend_work_queue, &early_suspend_work);
	} else if (old_sleep && new_state == PM_SUSPEND_ON) {
		state &= ~SUSPEND_REQUESTED;
		wake_lock(&main_wake_lock);
		queue_work(suspend_work_queue, &late_resume_work);
	}
	requested_suspend_state = new_state;
	spin_unlock_irqrestore(&state_lock, irqflags);
}

suspend_state_t get_suspend_state(void)
{
	return requested_suspend_state;
}
