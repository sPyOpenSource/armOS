/*
 * keyboard.c - keyboard driver for iPod
 *
 * Copyright (c) 2003, Bernard Leach (leachbj@bouncycastle.org)
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kbd_ll.h>
#include <linux/mm.h>
#include <linux/kbd_kern.h>
#include <asm/io.h>
#include <asm/arch/irqs.h>
#include <asm/keyboard.h>

/* we use the keycodes and translation is 1 to 1 */
#define R_SC		0x13
#define L_SC		0x26

#define UP_SC		103
#define LEFT_SC		105
#define RIGHT_SC	106
#define DOWN_SC		108

#define ACTION_SC	28

/* send ^S and ^Q for the hold switch */
#define LEFT_CTRL_SC	29
#define Q_SC		16
#define S_SC		31

/* need to pass something becuase we use a shared irq */
#define KEYBOARD_DEV_ID	0x4b455942

static void keyboard_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	static int prev_scroll = -1;
	unsigned char source, state;

	/* get source of interupts */
	source = inb(0xcf000040);
	if ( source == 0 ) {
		return; 	/* not for us */
	}

	/* get current keypad status */
	state = inb(0xcf000030);
	outb(~state, 0xcf000060);

#ifdef CONFIG_VT
	kbd_pt_regs = regs;

	if ( source & 0x1 ) {
		if ( state & 0x1 ) {
			//printk("ff up\n");
#if 0
			handle_scancode(RIGHT_SC, 0);
#else
			scrollfront(0);
#endif
		}
		else {
#if 0
			//printk("ff down\n");
			handle_scancode(RIGHT_SC, 1);
#endif
		}
	}
	if ( source & 0x2 ) {
		if ( state & 0x2 ) {
			//printk("action up\n");
			handle_scancode(ACTION_SC, 0);
		}
		else {
			//printk("action down\n");
			handle_scancode(ACTION_SC, 1);
		}
	}

	if ( source & 0x4 ) {
		if ( state & 0x4 ) {
			//printk("pause up\n");
#if 0
			handle_scancode(DOWN_SC, 0);
#else
			contrast_down();
#endif
		}
		else {
			//printk("pause down\n");
#if 0
			handle_scancode(DOWN_SC, 1);
#endif
		}
	}
	if ( source & 0x8 ) {
		if ( state & 0x8 ) {
			//printk("rr up\n");
#if 0
			handle_scancode(LEFT_SC, 0);
#else
			scrollback(0);
#endif
		}
		else {
			//printk("rr down\n");
#if 0
			handle_scancode(LEFT_SC, 1);
#endif
		}
	}
	if ( source & 0x10 ) {
		if ( state & 0x10 ) {
			//printk("menu up\n");
#if 0
			handle_scancode(LEFT_SC, 0);
#else
			contrast_up();
#endif
		}
		else {
			//printk("menu down\n");
#if 0
			handle_scancode(LEFT_SC, 1);
#endif
		}
	}
	if ( source & 0x20 ) {
		if ( state & 0x20 ) {
			//printk("hold on\n");
			/* CTRL-S down */
			handle_scancode(LEFT_CTRL_SC, 1);
			handle_scancode(S_SC, 1);

			/* CTRL-S up */
			handle_scancode(S_SC, 0);
			handle_scancode(LEFT_CTRL_SC, 0);
		}
		else {
			//printk("hold off\n");
			/* CTRL-Q down */
			handle_scancode(LEFT_CTRL_SC, 1);
			handle_scancode(Q_SC, 1);

			/* CTRL-Q up */
			handle_scancode(Q_SC, 0);
			handle_scancode(LEFT_CTRL_SC, 0);
		}
	}

	if ( source & 0xc0 ) {
		static int scroll_state[4][4] = {
			{0, 1, -1, 0},
			{-1, 0, 0, 1},
			{1, 0, 0, -1},
			{0, -1, 1, 0}
		};
		unsigned now_scroll = (state & 0xc0) >> 6;
						
		if ( prev_scroll == -1 ) {
			prev_scroll = now_scroll;
		}
		else {
			switch (scroll_state[prev_scroll][now_scroll]) {
			case 1:
				/* 'l' keypress */
				handle_scancode(L_SC, 1);
				handle_scancode(L_SC, 0);
				break;
			case -1:
				/* 'r' keypress */
				handle_scancode(R_SC, 1);
				handle_scancode(R_SC, 0);
				break;
			default:
				/* only happens if we get out of sync */
			}
		}

		prev_scroll = now_scroll;
	}

	tasklet_schedule(&keyboard_tasklet);
#endif /* CONFIG_VT */

	/* ack any active interrupts */
	outb(source, 0xcf000070);
}

void __init ipodkb_init_hw(void)
{
	outb(~inb(0xcf000030), 0xcf000060);
	outb(inb(0xcf000040), 0xcf000070);

	outb(inb(0xcf000004) | 0x1, 0xcf000004);
	outb(inb(0xcf000014) | 0x1, 0xcf000014);
	outb(inb(0xcf000024) | 0x1, 0xcf000024);

	if ( request_irq(GPIO_IRQ, keyboard_interrupt, SA_SHIRQ, "keyboard", KEYBOARD_DEV_ID) ) {
		printk("ipodkb: IRQ %d failed\n", GPIO_IRQ);
	}

	outb(0xff, 0xcf000050);
}

