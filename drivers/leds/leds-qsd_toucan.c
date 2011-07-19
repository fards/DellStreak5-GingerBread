
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/custmproc.h>
#include <mach/smem_pc_oem_cmd.h>
#include <mach/pm_log.h>
#include <linux/jiffies.h>

#define MSG(format, arg...)
#define MSG2(format, arg...) printk(KERN_INFO "[LED]" format "\n", ## arg)

struct hrtimer qsd_timed_vibrator_timer;
spinlock_t qsd_timed_vibrator_lock;
unsigned long jiff_vib_on_timeout;  

static __inline void vibrator_onOff(char onOff)
{
  unsigned int arg1, arg2=0;
  int ret;
  if(onOff)
  {
    arg1 = SMEM_PC_OEM_LED_CTL_VIB_ON;
    PM_LOG_EVENT(PM_LOG_ON, PM_LOG_VIBRATOR);
    jiff_vib_on_timeout = jiffies + 5*HZ;
  }
  else
  {
    arg1 = SMEM_PC_OEM_LED_CTL_VIB_OFF;
    PM_LOG_EVENT(PM_LOG_OFF, PM_LOG_VIBRATOR);
  }
  ret = cust_mproc_comm1(&arg1,&arg2);
  
  if(ret != 0)
  {
    MSG2("VIB %s: FAIL!", onOff?"ON":"OFF");
    jiff_vib_on_timeout = jiffies + 30*24*60*60*HZ;  
  }
  else
  {
    if(onOff == 0)  
    {
      if(time_after(jiffies, jiff_vib_on_timeout))
        MSG2("VIB OFF! Wait more than 5 sec after VIB ON");
      jiff_vib_on_timeout = jiffies + 30*24*60*60*HZ;  
    }
  }
}
static enum hrtimer_restart qsd_timed_vibrator_timer_func(struct hrtimer *timer)
{
  MSG("%s", __func__);
  vibrator_onOff(0);
	return HRTIMER_NORESTART;
}
static void qsd_timed_vibrator_init(void)
{
  
	hrtimer_init(&qsd_timed_vibrator_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  qsd_timed_vibrator_timer.function = qsd_timed_vibrator_timer_func;
	spin_lock_init(&qsd_timed_vibrator_lock);
}
static void qsd_timed_vibrator_deinit(void)
{
  
}
int qsd_timed_vibrator_get_time(struct timed_output_dev *sdev)
{
  ktime_t remain;
  int value = 0;
  MSG("%s", __func__);
	if(hrtimer_active(&qsd_timed_vibrator_timer))
	{
		remain = hrtimer_get_remaining(&qsd_timed_vibrator_timer);
		value = remain.tv.sec * 1000 + remain.tv.nsec / 1000000;
	}
  MSG("timeout = %d",value);
  return value;
}
void qsd_timed_vibrator_enable(struct timed_output_dev *sdev, int timeout)
{
	unsigned long	flags;
  MSG("%s", __func__);
	spin_lock_irqsave(&qsd_timed_vibrator_lock, flags);
	hrtimer_cancel(&qsd_timed_vibrator_timer);
  if(!timeout)  
    vibrator_onOff(0);
  else  
    vibrator_onOff(1);
	if(timeout > 0) 
	{
		hrtimer_start(&qsd_timed_vibrator_timer,
			ktime_set(timeout / 1000, (timeout % 1000) * 1000000),
			HRTIMER_MODE_REL);
	  MSG("%s Set timeout", __func__);
	}
	spin_unlock_irqrestore(&qsd_timed_vibrator_lock, flags);
}

static struct timed_output_dev qsd_timed_vibrator = {
  .name     = "vibrator",
  .enable   = qsd_timed_vibrator_enable,
  .get_time = qsd_timed_vibrator_get_time,
};


#if 0 
  static int qsd_led_suspend(struct platform_device *dev, pm_message_t state)
  {
    led_classdev_suspend(&qsd_led_keyboard_backlight);
    return 0;
  }

  static int qsd_led_resume(struct platform_device *dev)
  {
    led_classdev_resume(&qsd_led_keyboard_backlight);
    return 0;
  }
#else
  #define qsd_led_suspend NULL
  #define qsd_led_resume NULL
#endif


static int qsd_led_probe(struct platform_device *pdev)
{
  int ret=0, fail=0;  
  MSG("%s+", __func__);

  
  qsd_timed_vibrator_init();
  ret = timed_output_dev_register(&qsd_timed_vibrator);
  if(ret < 0) {fail = 6;  goto error_exit;}

  jiff_vib_on_timeout = jiffies + 30*24*60*60*HZ;  

  MSG("%s- PASS, ret=%d", __func__,ret);
  return ret;

error_exit:
  if(fail > 6)
    timed_output_dev_unregister(&qsd_timed_vibrator);
  if(fail > 5)
    qsd_timed_vibrator_deinit();
  MSG("%s- FAIL, ret=%d!", __func__,ret);
  return ret;
}

static int qsd_led_remove(struct platform_device *pdev)
{
  
  MSG("%s+", __func__);

  timed_output_dev_unregister(&qsd_timed_vibrator);
  qsd_timed_vibrator_deinit();
  MSG("%s-", __func__);
  return 0;
}


static struct platform_driver qsd_led_driver = {
  .probe    = qsd_led_probe,
  .remove   = qsd_led_remove,
  .suspend  = qsd_led_suspend,
  .resume   = qsd_led_resume,
  .driver   = {
    .name   = "qsd_led",
    .owner    = THIS_MODULE,
  },
};


static int __init qsd_led_init(void)
{
  int ret;
  printk("BootLog, +%s\n", __func__);
  ret = platform_driver_register(&qsd_led_driver);
  printk("BootLog, -%s, ret=%d\n", __func__,ret);
  return ret;
}

static void __exit qsd_led_exit(void)
{
  platform_driver_unregister(&qsd_led_driver);
}


module_init(qsd_led_init);
module_exit(qsd_led_exit);


