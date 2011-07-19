/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/fb.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>

#include "mdp.h"
#include "msm_fb.h"
#include "mdp4.h"
#include "mipi_dsi.h"

static struct mdp4_overlay_pipe *dsi_pipe;
static struct mdp4_overlay_pipe *pending_pipe;
static struct msm_fb_data_type *dsi_mfd;
static struct completion dsi_delay_comp;
static atomic_t dsi_delay_kickoff_cnt;

struct timer_list dsi_timer;

void dsi_delay_tout(unsigned long data)
{
	if (atomic_read(&dsi_delay_kickoff_cnt) != 0) {
		atomic_dec(&dsi_delay_kickoff_cnt);
		if (atomic_read(&dsi_delay_kickoff_cnt) == 0)
			complete(&dsi_delay_comp);
	}
}

void mdp4_overlay_update_dsi_cmd(struct msm_fb_data_type *mfd)
{
	MDPIBUF *iBuf = &mfd->ibuf;
	uint8 *src;
	int ptype;
	struct mdp4_overlay_pipe *pipe;
	int ret;

	if (mfd->key != MFD_KEY)
		return;

	dsi_mfd = mfd;		/* keep it */

	/* MDP cmd block enable */
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);

	if (dsi_pipe == NULL) {
		ptype = mdp4_overlay_format2type(mfd->fb_imgType);
		if (ptype < 0)
			printk(KERN_INFO "%s: format2type failed\n", __func__);
		pipe = mdp4_overlay_pipe_alloc(ptype, MDP4_MIXER0, 0);
		if (pipe == NULL)
			printk(KERN_INFO "%s: pipe_alloc failed\n", __func__);
		pipe->pipe_used++;
		pipe->mixer_stage  = MDP4_MIXER_STAGE_BASE;
		pipe->mixer_num  = MDP4_MIXER0;
		pipe->src_format = mfd->fb_imgType;
		mdp4_overlay_panel_mode(pipe->mixer_num, MDP4_PANEL_DSI_CMD);
		ret = mdp4_overlay_format2pipe(pipe);
		if (ret < 0)
			printk(KERN_INFO "%s: format2type failed\n", __func__);

		init_completion(&dsi_delay_comp);
		dsi_pipe = pipe; /* keep it */
		init_timer(&dsi_timer);
		dsi_timer.function = dsi_delay_tout;
		dsi_timer.data = 0;
		/*
		 * configure dsi stream id
		 * dma_p = 0, dma_s = 1
		 */
		MDP_OUTP(MDP_BASE + 0x000a0, 0x10);
		/* enable dsi trigger on dma_p */
		MDP_OUTP(MDP_BASE + 0x000a4, 0x01);
	} else {
		pipe = dsi_pipe;
	}

	/* whole screen for base layer */
	src = (uint8 *) iBuf->buf;

	{
		struct fb_info *fbi;

		fbi = mfd->fbi;
		pipe->src_height = fbi->var.yres;
		pipe->src_width = fbi->var.xres;
		pipe->src_h = fbi->var.yres;
		pipe->src_w = fbi->var.xres;
		pipe->src_y = 0;
		pipe->src_x = 0;
		pipe->dst_h = fbi->var.yres;
		pipe->dst_w = fbi->var.xres;
		pipe->dst_y = 0;
		pipe->dst_x = 0;
		pipe->srcp0_addr = (uint32)src;
		pipe->srcp0_ystride = fbi->fix.line_length;
	}


	mdp4_overlay_rgb_setup(pipe);

	mdp4_mixer_stage_up(pipe);

	mdp4_overlayproc_cfg(pipe);

	mdp4_overlay_dmap_xy(pipe);

	mdp4_overlay_dmap_cfg(mfd, 0);

	/* MDP cmd block disable */
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);

	wmb();
}

/*
 * mdp4_overlay1_done_dsi_cmd: called from isr
 */
void mdp4_overlay0_done_dsi_cmd()
{
	mdp_disable_irq_nosync(MDP_OVERLAY0_TERM);

	if (pending_pipe)
		complete(&pending_pipe->comp);
}

void mdp4_dsi_cmd_overlay_restore(void)
{
	/* mutex holded by caller */
	if (dsi_mfd && dsi_pipe) {
		mdp4_dsi_cmd_dma_busy_wait(dsi_mfd, dsi_pipe);
		mdp4_overlay_update_dsi_cmd(dsi_mfd);
		mdp4_dsi_cmd_overlay_kickoff(dsi_mfd, dsi_pipe);
		dsi_mfd->dma_update_flag = 1;
	}
}

void dsi_add_delay_timer(int delay_ms)
{
	dsi_timer.expires = jiffies + ((delay_ms * HZ) / 1000);
	add_timer(&dsi_timer);
}

void mdp4_dsi_cmd_dma_busy_wait(struct msm_fb_data_type *mfd,
				struct mdp4_overlay_pipe *pipe)
{
	unsigned long flag;

	if (pipe == NULL) /* first time since boot up */
		return;

	spin_lock_irqsave(&mdp_spin_lock, flag);
	if (mfd->dma->busy == TRUE) {
		INIT_COMPLETION(pipe->comp);
		pending_pipe = pipe;
	}
	spin_unlock_irqrestore(&mdp_spin_lock, flag);

	if (pending_pipe != NULL) {
		/* wait until DMA finishes the current job */
		wait_for_completion_killable(&pipe->comp);
		mfd->dma_update_flag = 0;
		pending_pipe = NULL;
	}
}

void mdp4_dsi_cmd_kickoff_video(struct msm_fb_data_type *mfd,
				struct mdp4_overlay_pipe *pipe)
{
	unsigned long flag;

	if (atomic_read(&dsi_delay_kickoff_cnt) != 0) {
		spin_lock_irqsave(&mdp_spin_lock, flag);
		complete(&dsi_delay_comp);
		atomic_set(&dsi_delay_kickoff_cnt, 0);
		del_timer(&dsi_timer);
		mdp4_stat.kickoff_piggy++;
		spin_unlock_irqrestore(&mdp_spin_lock, flag);
		return;
	}

	mdp4_dsi_cmd_overlay_kickoff(mfd, pipe);
}

void mdp4_dsi_cmd_kickoff_ui(struct msm_fb_data_type *mfd,
				struct mdp4_overlay_pipe *pipe)
{

	if (mdp4_overlay_mixer_play(dsi_pipe->mixer_num) > 0) {
		dsi_add_delay_timer(10);
		atomic_set(&dsi_delay_kickoff_cnt, 1);
		INIT_COMPLETION(dsi_delay_comp);
		mutex_unlock(&mfd->dma->ov_mutex);
		wait_for_completion_killable(&dsi_delay_comp);
		mutex_lock(&mfd->dma->ov_mutex);
	}

	mdp4_dsi_cmd_overlay_kickoff(mfd, pipe);
}


void mdp4_dsi_cmd_overlay_kickoff(struct msm_fb_data_type *mfd,
				struct mdp4_overlay_pipe *pipe)
{

	down(&mfd->sem);
	mdp_enable_irq(MDP_OVERLAY0_TERM);
	mfd->dma->busy = TRUE;
	/* start OVERLAY pipe */
	mdp_pipe_kickoff(MDP_OVERLAY0_TERM, mfd);
	/* trigger dsi cmd engine */
	mipi_dsi_cmd_mdp_sw_trigger();
	up(&mfd->sem);
}

void mdp4_dsi_cmd_overlay(struct msm_fb_data_type *mfd)
{
	mutex_lock(&mfd->dma->ov_mutex);

	if (mfd && mfd->panel_power_on) {
		mdp4_dsi_cmd_dma_busy_wait(mfd, dsi_pipe);
		mdp4_overlay_update_dsi_cmd(mfd);

		mdp4_dsi_cmd_kickoff_ui(mfd, dsi_pipe);

		mdp4_stat.kickoff_dsi++;

	/* signal if pan function is waiting for the update completion */
		if (mfd->pan_waiting) {
			mfd->pan_waiting = FALSE;
			complete(&mfd->pan_comp);
		}
	}

	mutex_unlock(&mfd->dma->ov_mutex);
}
