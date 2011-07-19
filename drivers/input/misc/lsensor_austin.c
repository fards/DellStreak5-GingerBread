
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/custmproc.h>
#include <mach/smem_pc_oem_cmd.h>
#include <mach/lsensor.h>
#include <mach/bkl_cmd.h>
#include <mach/pm_log.h>

#define MSG(format, arg...)

DEFINE_MUTEX(lsensor_enable_lock);

struct lsensor_data ls_data = {
  .enable = 0,
  .enable_inited = 0,
  .vreg_state = 0,
  .vreg_mmc = 0
};

static struct notifier_block lsensor_notifier_block;

void qsd_lsensor_enable_onOff(unsigned int mode, unsigned int onOff)
{
  MSG("%s+ mode=%d onOff=%d", __func__,mode,onOff);

  mutex_lock(&lsensor_enable_lock);
  if(mode==LSENSOR_ENABLE_SENSOR || mode==LSENSOR_ENABLE_LCD || mode==LSENSOR_ENABLE_LCD_ENG)
  {
    if(onOff)
      ls_data.enable |=   mode;
    else
      ls_data.enable &= (~mode);
  }
  if(ls_data.enable_inited)
  {
    if(ls_data.enable)
    {
      if(ls_data.vreg_state == 0)
      {
        MSG("LSensor (ON)");
        ls_data.vreg_state = 1;
        if(ls_data.vreg_mmc)  vreg_enable(ls_data.vreg_mmc);
        gpio_set_value(ALS_GPIO_SD,0);
        PM_LOG_EVENT(PM_LOG_ON, PM_LOG_SENSOR_ALS);
      }
      else
      {
      }
    }
    else
    {
      if(ls_data.vreg_state == 1)
      {
        MSG("LSensor (OFF)");
        ls_data.vreg_state = 0;
        gpio_set_value(ALS_GPIO_SD,1);
        if(ls_data.vreg_mmc)  vreg_disable(ls_data.vreg_mmc);
        PM_LOG_EVENT(PM_LOG_OFF, PM_LOG_SENSOR_ALS);
      }
      else
      {
      }
    }
  }
  mutex_unlock(&lsensor_enable_lock);
  MSG("%s- enable=%d", __func__,ls_data.enable);
}
EXPORT_SYMBOL(qsd_lsensor_enable_onOff);

unsigned int qsd_lsensor_read_cal_data(void)
{
  unsigned int arg1=SMEM_PC_OEM_GET_ALS_CAL_NV_DATA, arg2=0;
  int ret, stage;
  MSG("%s", __func__);
  ret = cust_mproc_comm1(&arg1,&arg2);
  if(ret)
  {
    stage = 4;
    printk("[LSEN]%s read fail, stage = 2\n",__func__);
  }
  else
  {
    if(arg2 == 0xFFFFFFFF)
      stage = 4;
    else if(arg2 <= 71)   
      stage = 0;
    else if(arg2 <= 81)   
      stage = 1;
    else if(arg2 <= 91)   
      stage = 2;
    else if(arg2 <= 101)  
      stage = 3;
    else if(arg2 <= 111)  
      stage = 4;
    else
      stage = 5;
    printk("[LSEN]%s = 0x%08X, stage = %d\n",__func__,arg2, stage);
  }
  return stage;
}
EXPORT_SYMBOL(qsd_lsensor_read_cal_data);

/*unsigned int qsd_lsensor_read(void)
{
  #if 0
    unsigned int als;
    als = bkl_eng_mode2_read_als();
    return als;
  #else
    unsigned int arg1, arg2;
    int ret;
    MSG("%s", __func__);
    arg1 = SMEM_PC_OEM_LED_CTL_READ_ALS;
    arg2 = 0;
    ret = cust_mproc_comm1(&arg1,&arg2);
    if(ret)
      return 0;
    else
      return arg2;
  #endif
}
EXPORT_SYMBOL(qsd_lsensor_read);*/

static int misc_lsensor_open(struct inode *inode_p, struct file *fp)
{
  MSG("%s", __func__);
  return 0;
}
static int misc_lsensor_release(struct inode *inode_p, struct file *fp)
{
  MSG("%s", __func__);
  return 0;
}
static int misc_lsensor_ioctl(struct inode *inode_p, struct file *fp, unsigned int cmd, unsigned long arg)
{
  static unsigned int light_old = 65535;
  unsigned int light;
  int ret = 0;

  if(_IOC_TYPE(cmd) != LSENSOR_IOC_MAGIC)
  {
    MSG("%s: Not LSENSOR_IOC_MAGIC", __func__);
    return -ENOTTY;
  }
  if(_IOC_DIR(cmd) & _IOC_READ)
  {
    ret = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
    if(ret)
    {
      MSG("%s: access_ok check write err", __func__);
      return -EFAULT;
    }
  }
  if(_IOC_DIR(cmd) & _IOC_WRITE)
  {
    ret = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
    if (ret)
    {
      MSG("%s: access_ok check read err", __func__);
      return -EFAULT;
    }
  }

  switch (cmd)
  {
    case LSENSOR_IOC_ENABLE:
      qsd_lsensor_enable_onOff(LSENSOR_ENABLE_SENSOR,1);
      auo_lcd_als_enable(1);
      MSG("%s: LSENSOR_IOC_ENABLE", __func__);
      break;

    case LSENSOR_IOC_DISABLE:
      auo_lcd_als_enable(0);
      qsd_lsensor_enable_onOff(LSENSOR_ENABLE_SENSOR,0);
      MSG("%s: LSENSOR_IOC_DISABLE", __func__);
      break;

    case LSENSOR_IOC_GET_STATUS:
      light = auo_lcd_als_get_lux();
      if(light_old != light)
      {
        light_old = light;
        MSG("%s: LSENSOR_IOC_GET_STATUS = %d", __func__,light);
      }
      if (copy_to_user((void __user*) arg, &light, _IOC_SIZE(cmd)))
      {
        MSG("%s: LSENSOR_IOC_GET_STATUS fail", __func__);
        ret = -EFAULT;
      }
      break;

    default:
      MSG("%s: unknown ioctl=%d", __func__,cmd);
      break;
  }
  return ret;
}

static int lsensor_notifier_handler(struct notifier_block *block, unsigned long status, void *v)
{
  printk(KERN_INFO "%s: status:%lu\n", __func__, status);
  mutex_lock(&lsensor_enable_lock);
  if (status) {
    if (ls_data.enable) gpio_set_value(ALS_GPIO_SD, 0);
    else gpio_set_value(ALS_GPIO_SD, 1);
  } else {
    if (ls_data.enable) printk(KERN_ERR "%s: Error als is on while vreg_mmc is off\n", __func__);
    else {
      printk(KERN_INFO "%s: Pull Down ALS gpio\n", __func__);
      gpio_set_value(ALS_GPIO_SD, 0);
    }
  }
  mutex_unlock(&lsensor_enable_lock);
  return 0;

}

static struct file_operations misc_lsensor_fops = {
  .owner  = THIS_MODULE,
  .open   = misc_lsensor_open,
  .release = misc_lsensor_release,
  .ioctl  = misc_lsensor_ioctl,
};
static struct miscdevice misc_lsensor_device = {
  .minor  = MISC_DYNAMIC_MINOR,
  .name   = "lsensor_cm3204",
  .fops   = &misc_lsensor_fops,
};

static int __init qsd_lsensor_init(void)
{
  int ret;

  printk("BootLog, +%s\n", __func__);

  gpio_tlmm_config(GPIO_CFG(ALS_GPIO_SD,0,GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),GPIO_CFG_ENABLE);

  gpio_set_value(ALS_GPIO_SD,1);

  ls_data.vreg_mmc = vreg_get((void *)&misc_lsensor_device, "mmc");
  if(IS_ERR(ls_data.vreg_mmc))
  {
    printk("%s: vreg_get fail\n", __func__);
    ls_data.vreg_mmc = NULL;
  }
  else
  {
    ret = vreg_set_level(ls_data.vreg_mmc, 2600);
    if(ret) printk("%s: vreg_lsensor set 2.6V Fail, ret=%d\n", __func__,ret);
  }

  ls_data.enable_inited = 1;

  qsd_lsensor_enable_onOff(0,0);

  ret = misc_register(&misc_lsensor_device);
  if (ret)
  {
    printk("%s: misc_register fail\n", __func__);
    goto exit;
  }

  lsensor_notifier_block.notifier_call = lsensor_notifier_handler;
  vreg_add_notifier(ls_data.vreg_mmc, &lsensor_notifier_block);

  printk("BootLog, -%s, ret=%d\n", __func__,ret);
  return ret;

exit:
  qsd_lsensor_enable_onOff(LSENSOR_ENABLE_SENSOR,0);
  vreg_remove_notifier(ls_data.vreg_mmc, &lsensor_notifier_block);

  printk("BootLog, -%s, ret=%d\n", __func__,ret);
  return ret;
}
static void __exit qsd_lsensor_exit(void)
{
  misc_deregister(&misc_lsensor_device);
  qsd_lsensor_enable_onOff(LSENSOR_ENABLE_SENSOR,0);
  ls_data.enable_inited = 0;
}

module_init(qsd_lsensor_init);
module_exit(qsd_lsensor_exit);

