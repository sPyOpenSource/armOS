/*
 * fb.c - Frame-buffer driver for iPod
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
 *
 * The LCD uses the HD66753 controller from Hitachi (now owned by Renesas).
 */

#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioctl.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <video/fbcon.h>
#include <video/fbcon-cfb2.h>
#include <video/fbcon-cfb16.h>

/* the ID returned by the controller */
#define HD66753_ID	0x5307

#define IPOD_PP5002_LCD_BASE	0xc0001000
#define IPOD_PP5002_RTC		0xcf001110

#define IPOD_PP5020_LCD_BASE	0x70003000
#define IPOD_PP5020_RTC		0x60005010

#define LCD_DATA 0x10
#define LCD_CMD  0x08

#define IPOD_5G_LCD_WIDTH	320
#define IPOD_5G_LCD_HEIGHT	240

#define IPOD_STD_LCD_WIDTH	160
#define IPOD_STD_LCD_HEIGHT	128

#define IPOD_MINI_LCD_WIDTH	138
#define IPOD_MINI_LCD_HEIGHT	110

#define IPOD_PHOTO_LCD_WIDTH	220
#define IPOD_PHOTO_LCD_HEIGHT	176

#define IPOD_NANO_LCD_WIDTH	176
#define IPOD_NANO_LCD_HEIGHT	132

static int ipod_hw_ver;

static u16 ipod_fbcon_cmap[16];


static unsigned long ipod_rtc;
static unsigned long lcd_base;
static unsigned long lcd_busy_mask;

static unsigned long lcd_width;
static unsigned long lcd_height;

static unsigned long lcd_type;

/* allow for 16bpp for photo res */ /* Sized to max we could possibly need */
static char ipod_scr[IPOD_5G_LCD_HEIGHT * IPOD_5G_LCD_WIDTH * 8];

static unsigned int lcd_contrast = 0x6a;	/* required for mini2 */

/* get current usec counter */
static int timer_get_current(void)
{
	return inl(ipod_rtc);
}

/* check if number of useconds has past */
static int timer_check(int clock_start, int usecs)
{
	unsigned long clock;
	clock = inl(ipod_rtc);
	
	if ( (clock - clock_start) >= usecs ) {
		return 1;
	} else {
		return 0;
	}
}

/* wait for LCD with timeout */
static void lcd_wait_write(void)
{
	if ((inl(lcd_base) & lcd_busy_mask) != 0) {
		int start = timer_get_current();

		do {
			if ((inl(lcd_base) & lcd_busy_mask) == 0) break;
		} while (timer_check(start, 1000) == 0);
	}
}


/* send LCD data */
static void lcd_send_data(int data_lo, int data_hi)
{
	lcd_wait_write();
	if (ipod_hw_ver == 0x7) {
		outl((inl(0x70003000) & ~0x1f00000) | 0x1700000, 0x70003000);
		outl(data_hi | (data_lo << 8) | 0x760000, 0x70003008);
	}
	else {
		outl(data_lo, lcd_base + LCD_DATA);
		lcd_wait_write();
		outl(data_hi, lcd_base + LCD_DATA);
	}
}

/* send LCD command */
static void
lcd_prepare_cmd(int cmd)
{
	lcd_wait_write();
	if (ipod_hw_ver == 0x7) {
		outl((inl(0x70003000) & ~0x1f00000) | 0x1700000, 0x70003000);
		outl(cmd | 0x740000, 0x70003008);
	}
	else {
		outl(0x0, lcd_base + LCD_CMD);
		lcd_wait_write();
		outl(cmd, lcd_base + LCD_CMD);
	}
}

/* send LCD command and data */
/* this is only used for b&w lcds */
static void lcd_cmd_and_data(int cmd, int data_lo, int data_hi)
{
	lcd_prepare_cmd(cmd);
	lcd_send_data(data_lo, data_hi);
}

static unsigned
get_contrast(void)
{
	if (ipod_hw_ver < 0x6) {
		unsigned data_lo, data_hi = 0;

		/* data_lo has the scan line */
		lcd_wait_write();

		data_lo = inl(lcd_base + LCD_CMD);

		/* data_hi has the contrast */
		lcd_wait_write();

		data_hi = inl(lcd_base + LCD_CMD);

		return data_hi & 0xff;
	}
	else if (ipod_hw_ver == 0x7) {
		return lcd_contrast;
	}

	return 0;
}

static void
set_contrast(int contrast)
{
	if (ipod_hw_ver < 0x6 || ipod_hw_ver == 0x7) {
		lcd_cmd_and_data(0x4, 0x4, contrast);
		lcd_contrast = contrast;
	}
}

static int
get_backlight(void)
{
	if (ipod_hw_ver >= 0x04) {
		/* is Port B03 on or off */
		if (inl(0x6000d824) & (1<<3)) {
			if (ipod_hw_ver == 0x5 || ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
				return (inl(0x7000a010) >> 16) & 0xff;
			}

			return 1;
		}

		return 0;
	} else {
		return inl(lcd_base) & 0x2 ? 1 : 0;
	}
}

static void
set_backlight(int on)
{
	if (ipod_hw_ver >= 0x4) {
		if (ipod_hw_ver == 0x5 || ipod_hw_ver == 0x6) {
			if (on) {
				/* brightness full */
				outl(0x80000000 | (0xff << 16), 0x7000a010);

				/* set port B03 on */
				outl(((0x100 | 1) << 3), 0x6000d824);
			}
			else {
				/* fades backlght off on 4g */
				/* GPO D01 disable */
				outl(inl(0x70000084) & ~0x2000000, 0x70000084);
				outl(0x80000000, 0x7000a010);
			}
		} else if (ipod_hw_ver == 0x04 || ipod_hw_ver == 0x7) {
			/* set port B03 */
			outl(((0x100 | (on ? 1 : 0)) << 3), 0x6000d824);
		} else if (ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
			/* set port B03 */
			outl(((0x100 | (on ? 1 : 0)) << 3), 0x6000d824);
			/* set port L07 */
			outl(((0x100 | (on ? 1 : 0)) << 7), 0x6000d12c);
		}
	} else {
		int lcd_state;

		lcd_state = inl(IPOD_PP5002_LCD_BASE);
		if (on) {
			lcd_state = lcd_state | 0x2;
		}
		else {
			lcd_state = lcd_state & ~0x2;
		}
		outl(lcd_state, IPOD_PP5002_LCD_BASE);
	}

	if (ipod_hw_ver < 0x6 || ipod_hw_ver == 0x7) {
		if (on) {
			/* display control (1 00 0 1) */
			/* GSH=01 -> 2/3 level grayscale control */
			/* GSL=00 -> 1/4 level grayscale control */
			/* REV=0 -> don't reverse */
			/* D=1 -> display on */
			if (ipod_hw_ver < 3) {
				/* REV=1 */
				lcd_cmd_and_data(0x7, 0x0, 0x11 | 0x2);
			}
			else {
				lcd_cmd_and_data(0x7, 0x0, 0x11);
			}
		}
		else {
			/* display control (10 0 1) */
			/* GSL=10 -> 2/4 level grayscale control */
			/* REV=0 -> don't reverse */
			/* D=1 -> display on */
			lcd_cmd_and_data(0x7, 0x0, 0x9);
		}
	}
}

static unsigned
read_controller_id(void)
{
	unsigned data_lo, data_hi;

	/* read the Start Osciallation register -> it gives us a id */
	lcd_prepare_cmd(0x0);

	lcd_wait_write();	
	data_lo = inl(lcd_base + LCD_DATA);
	
	lcd_wait_write();
	data_hi = inl(lcd_base + LCD_DATA);

	return ((data_hi & 0xff) << 8) | (data_lo & 0xff);
}

static void lcd_send_lo(int v)
{
	lcd_wait_write();
	outl(v | 0x80000000, 0x70008A0C);
}

static void lcd_send_hi(int v)
{
	lcd_wait_write();
	outl(v | 0x81000000, 0x70008A0C);
}

static void lcd_cmd_data(int cmd, int data)
{
	if (lcd_type == 0) {
		lcd_send_lo(cmd);
		lcd_send_lo(data);
	} else {
		lcd_send_lo(0x0);
		lcd_send_lo(cmd);
		lcd_send_hi((data >> 8) & 0xff);
		lcd_send_hi(data & 0xff);
	}
}

/* initialise the LCD */
static void
init_lcd(void)
{
	if (ipod_hw_ver < 0x6 && read_controller_id() != HD66753_ID )  {
		printk(KERN_ERR "Unknown LCD controller ID: 0x%x id?\n", read_controller_id());
	}

	/* driver output control */
	/* CMS=0, SGS=1 */
	if (ipod_hw_ver == 0x4 || ipod_hw_ver == 0x7) {
		/* driver output control - 160x112 (ipod mini) */
		lcd_cmd_and_data(0x1, 0x0, 0xd);
	}
	else if (ipod_hw_ver < 0x4 || ipod_hw_ver == 0x5) {
		/* driver output control - 160x128 */
		lcd_cmd_and_data(0x1, 0x1, 0xf);
	}

	/* ID=1 -> auto decrement address counter */
	/* AM=00 -> data is continuously written in parallel */
	/* LG=00 -> no logical operation */
	if (ipod_hw_ver < 0x6 || ipod_hw_ver == 0x7) {
		lcd_cmd_and_data(0x5, 0x0, 0x10);
	}

	if (ipod_hw_ver == 0x5 || ipod_hw_ver == 0x6) {
		outl(inl(0x6000d004) | 0x4, 0x6000d004); /* B02 enable */
		outl(inl(0x6000d004) | 0x8, 0x6000d004); /* B03 enable */
		outl(inl(0x70000084) | 0x2000000, 0x70000084); /* D01 enable */
		outl(inl(0x70000080) | 0x2000000, 0x70000080); /* D01 =1 */

		outl(inl(0x6000600c) | 0x20000, 0x6000600c);	/* PWM enable */
	}

	if (ipod_hw_ver == 0x6 && lcd_type == 0) {
		lcd_cmd_data(0xef, 0x0);
		lcd_cmd_data(0x1, 0x0);
		lcd_cmd_data(0x80, 0x1);
		lcd_cmd_data(0x10, 0x8);
		lcd_cmd_data(0x18, 0x6);
		lcd_cmd_data(0x7e, 0x4);
		lcd_cmd_data(0x7e, 0x5);
		lcd_cmd_data(0x7f, 0x1);
	}

	/* backlight off & set grayscale */
	set_backlight(1);
}

static void ipod_update_display(struct display *p, int sx, int sy, int mx, int my)
{
	unsigned short cursor_pos;
	unsigned short y;

	cursor_pos = sx + (sy * fontheight(p) * 0x20);

	for ( y = sy * fontheight(p); y < my * fontheight(p); y++ ) {
		unsigned char *img_data;
		unsigned char x;

		/* move the cursor */
		lcd_cmd_and_data(0x11, cursor_pos >> 8, cursor_pos & 0xff);

		/* setup for printing */
		lcd_prepare_cmd(0x12);

		/* cursor pos * image data width */
		img_data = &ipod_scr[y * p->line_length + sx * 2];

		/* 160/8 -> 20 == loops 20 times */
		for ( x = sx; x < mx; x++ ) {
			/* display a character */
			lcd_send_data(*(img_data + 1), *img_data);

			img_data += 2;
		}

		/* update cursor pos counter */
		cursor_pos += 0x20;
	}
}

static void lcd_bcm_write32(unsigned address, unsigned value) {
	/* write out destination address as two 16bit values */
	outw(address, 0x30010000);
	outw((address >> 16), 0x30010000);

	/* wait for it to be write ready */
	while ((inw(0x30030000) & 0x2) == 0);

	/* write out the value low 16, high 16 */
	outw(value, 0x30000000);
	outw((value >> 16), 0x30000000);
}

static void lcd_bcm_setup_rect(unsigned cmd, unsigned start_horiz, unsigned start_vert, unsigned max_horiz, unsigned max_vert, unsigned count) {
	lcd_bcm_write32(0x1F8, 0xFFFA0005);
	lcd_bcm_write32(0xE0000, cmd);
	lcd_bcm_write32(0xE0004, start_horiz);
	lcd_bcm_write32(0xE0008, start_vert);
	lcd_bcm_write32(0xE000C, max_horiz);
	lcd_bcm_write32(0xE0010, max_vert);
	lcd_bcm_write32(0xE0014, count);
	lcd_bcm_write32(0xE0018, count);
	lcd_bcm_write32(0xE001C, 0);
}

static unsigned lcd_bcm_read32(unsigned address) {
	while ((inw(0x30020000) & 1) == 0);

	/* write out destination address as two 16bit values */
	outw(address, 0x30020000);
	outw((address >> 16), 0x30020000);

	/* wait for it to be read ready */
	while ((inw(0x30030000) & 0x10) == 0);

	/* read the value */
	return inw(0x30000000) | inw(0x30000000) << 16;
}

static void lcd_bcm_finishup(void) {
	unsigned data; 

	outw(0x31, 0x30030000); 

	lcd_bcm_read32(0x1FC);

	do {
		data = lcd_bcm_read32(0x1F8);
	} while (data == 0xFFFA0005 || data == 0xFFFF);

	lcd_bcm_read32(0x1FC);
}

static void ipod_update_photo(struct display *p, int sx, int sy, int mx, int my)
{
	int startx = sy * fontheight(p);
	int starty = sx * fontwidth(p);
	int height = (my - sy) * fontheight(p);
	int width = (mx - sx) * fontwidth(p);
	int rect1, rect2, rect3, rect4;

	unsigned short *addr = (unsigned short *)ipod_scr;

	/* calculate the drawing region */
	if (ipod_hw_ver != 0x6) {
		rect1 = starty;			/* start horiz */
		rect2 = startx;			/* start vert */
		rect3 = (starty + width) - 1;	/* max horiz */
		rect4 = (startx + height) - 1;	/* max vert */
	} else {
		rect1 = startx;			/* start vert */
		rect2 = (lcd_width - 1) - starty;	/* start horiz */
		rect3 = (startx + height) - 1;	/* end vert */
		rect4 = (rect2 - width) + 1;		/* end horiz */
	}

	/* setup the drawing region */
	if (lcd_type == 0) {
		lcd_cmd_data(0x12, rect1);	/* start vert */
		lcd_cmd_data(0x13, rect2);	/* start horiz */
		lcd_cmd_data(0x15, rect3);	/* end vert */
		lcd_cmd_data(0x16, rect4);	/* end horiz */
	} else if (ipod_hw_ver != 0xb) {
		/* swap max horiz < start horiz */
		if (rect3 < rect1) {
			int t;
			t = rect1;
			rect1 = rect3;
			rect3 = t;
		}

		/* swap max vert < start vert */
		if (rect4 < rect2) {
			int t;
			t = rect2;
			rect2 = rect4;
			rect4 = t;
		}

		/* max horiz << 8 | start horiz */
		lcd_cmd_data(0x44, (rect3 << 8) | rect1);
		/* max vert << 8 | start vert */
		lcd_cmd_data(0x45, (rect4 << 8) | rect2);

		if (ipod_hw_ver == 0x6) {
			/* start vert = max vert */
			rect2 = rect4;
		}

		/* position cursor (set AD0-AD15) */
		/* start vert << 8 | start horiz */
		lcd_cmd_data(0x21, (rect2 << 8) | rect1);

		/* start drawing */
		lcd_send_lo(0x0);
		lcd_send_lo(0x22);
	} 

	addr += startx * p->line_length + starty;

	while (height > 0) {
		int x, y;
		int h, pixels_to_write;

		pixels_to_write = (width * height) * 2;

		/* calculate how much we can do in one go */
		h = height;
		if (pixels_to_write > 64000) {
			h = (64000/2) / width;
			pixels_to_write = (width * h) * 2;
		}

		outl(0x10000080, 0x70008a20);
		outl((pixels_to_write - 1) | 0xc0010000, 0x70008a24);
		outl(0x34000000, 0x70008a20);

		/* for each row */
		for (x = 0; x < h; x++) {
			/* for each column */
			for (y = 0; y < width; y += 2) {
				unsigned two_pixels;
				two_pixels =  (((addr[0]&0x00FF) << 8) |
					       ((addr[0]&0xFF00) >> 8)) |
					     ((((addr[1]&0x00FF) << 8) |
					       ((addr[1]&0xFF00) >> 8)) << 16);
				addr += 2;

				while ((inl(0x70008a20) & 0x1000000) == 0);

				/* output 2 pixels */
				outl(two_pixels, 0x70008b00);
			}

			addr += lcd_width - width;
		}
		while ((inl(0x70008a20) & 0x4000000) == 0);
	
		outl(0x0, 0x70008a24);

		height = height - h;
	}

}


static void ipod_update_video(struct display *p, int sx, int sy, int mx, int my)
{
	int startx = sy * fontheight(p);
	int starty = sx * fontwidth(p);
	int height = (my - sy) * fontheight(p);
	int width = (mx - sx) * fontwidth(p);
	int rect1, rect2, rect3, rect4;
	int x, y;
	unsigned short *addr = (unsigned short *)ipod_scr;

	/* calculate the drawing region */
	rect1 = starty;			/* start horiz */
	rect2 = startx;			/* start vert */
	rect3 = (starty + width) - 1;	/* max horiz */
	rect4 = (startx + height) - 1;	/* max vert */
	

	/* setup the drawing region */
	lcd_bcm_setup_rect(0x34, rect1, rect2, rect3, rect4, (width*height)<<1);

	addr += startx * p->line_length + starty;

	outw((0xE0020 & 0xffff), 0x30010000);
	outw((0xE0020 >> 16), 0x30010000);

	while ((inw(0x30030000) & 0x2)==0);

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x+=2) {
			outw(*(addr++), 0x30000000);
			outw(*(addr++), 0x30000000);
		}
		addr += (lcd_width - width);
	}
	
	
	lcd_bcm_finishup();
}





struct ipodfb_info {
	/*
	 *  Choose _one_ of the two alternatives:
	 *
	 *    1. Use the generic frame buffer operations (fbgen_*).
	 */
	struct fb_info_gen gen;

#if 0
	/*
	 *    2. Provide your own frame buffer operations.
	 */
	struct fb_info info;
#endif

	/* Here starts the frame buffer device dependent part */
	/* You can use this to store e.g. the board number if you support */
	/* multiple boards */
};


struct ipodfb_par {
	/*
	 *  The hardware specific data in this structure uniquely defines a video
	 *  mode.
	 *
	 *  If your hardware supports only one video mode, you can leave it empty.
	 */
};


void ipod_fb_setup(struct display *p)
{
	fbcon_cfb2.setup(p);
}

void ipod_fb_bmove(struct display *p, int sy, int sx, int dy, int dx,
		     int height, int width)
{
	fbcon_cfb2.bmove(p, sy, sx, dy, dx, height, width);
	ipod_update_display(p, 0, 0, lcd_width/8, lcd_height/fontheight(p));
}

void ipod_fb_clear(struct vc_data *conp, struct display *p, int sy, int sx,
		     int height, int width)
{
	fbcon_cfb2.clear(conp, p, sy, sx, height, width);
	ipod_update_display(p, sx, sy, sx+width, sy+height);
}

void ipod_fb_putc(struct vc_data *conp, struct display *p, int c, int yy,
		    int xx)
{
	fbcon_cfb2.putc(conp, p, c, yy, xx);
	ipod_update_display(p, xx, yy, xx+1, yy+1);
}

void ipod_fb_putcs(struct vc_data *conp, struct display *p, 
		     const unsigned short *s, int count, int yy, int xx)
{
	fbcon_cfb2.putcs(conp, p, s, count, yy, xx);
	ipod_update_display(p, xx, yy, xx+count, yy+1);
}

void ipod_fb_revc(struct display *p, int xx, int yy)
{
	fbcon_cfb2.revc(p, xx, yy);
	ipod_update_display(p, xx, yy, xx+1, yy+1);
}


/*
 *  `switch' for the low level operations
 */

struct display_switch fbcon_ipod = {
	setup:		ipod_fb_setup,
	bmove:		ipod_fb_bmove,
	clear:		ipod_fb_clear,
	putc:		ipod_fb_putc,
	putcs:		ipod_fb_putcs,
	revc:		ipod_fb_revc,
	fontwidthmask:	FONTWIDTH(8)
};

void ipod_fb16_setup(struct display *p)
{
	fbcon_cfb16.setup(p);
}

void ipod_fb16_bmove(struct display *p, int sy, int sx, int dy, int dx,
		     int height, int width)
{
	fbcon_cfb16.bmove(p, sy, sx, dy, dx, height, width);
	if (ipod_hw_ver!=0xb)	
		ipod_update_photo(p, 0, 0, lcd_width/fontwidth(p), lcd_height/fontheight(p));
	else
		ipod_update_video(p, 0, 0, lcd_width/fontwidth(p), lcd_height/fontheight(p));

}

void ipod_fb16_clear(struct vc_data *conp, struct display *p, int sy, int sx,
		     int height, int width)
{
	fbcon_cfb16.clear(conp, p, sy, sx, height, width);
	if (ipod_hw_ver!=0xb)
		ipod_update_photo(p, sx, sy, sx+width, sy+height);
	else
		ipod_update_video(p, sx, sy, sx+width, sy+height);
}

void ipod_fb16_putc(struct vc_data *conp, struct display *p, int c, int yy,
		    int xx)
{
	fbcon_cfb16.putc(conp, p, c, yy, xx);
	if (ipod_hw_ver!=0xb)
		ipod_update_photo(p, xx, yy, xx+1, yy+1);
	else
		ipod_update_video(p, xx, yy, xx+1, yy+1);
}

void ipod_fb16_putcs(struct vc_data *conp, struct display *p, 
		     const unsigned short *s, int count, int yy, int xx)
{
	fbcon_cfb16.putcs(conp, p, s, count, yy, xx);
	if (ipod_hw_ver!=0xb)
		ipod_update_photo(p, xx, yy, xx+count, yy+1);
	else
		ipod_update_video(p, xx, yy, xx+count, yy+1);
}

void ipod_fb16_revc(struct display *p, int xx, int yy)
{
	fbcon_cfb16.revc(p, xx, yy);
	if (ipod_hw_ver!=0xb)
		ipod_update_photo(p, xx, yy, xx+1, yy+1);
	else
		ipod_update_video(p, xx, yy, xx+1, yy+1);
		
}

void ipod_fb16_clear_margins(struct vc_data *conp, struct display *p,
			       int bottom_only)
{
	fbcon_cfb16.clear_margins(conp, p, bottom_only);
}


/*
 *  `switch' for the low level operations
 */

struct display_switch fbcon_ipod16 = {
	setup:		ipod_fb16_setup,
	bmove:		ipod_fb16_bmove,
	clear:		ipod_fb16_clear,
	putc:		ipod_fb16_putc,
	putcs:		ipod_fb16_putcs,
	revc:		ipod_fb16_revc,
	clear_margins:	ipod_fb16_clear_margins,
	fontwidthmask:	FONTWIDTH(4)|FONTWIDTH(8)|FONTWIDTH(12)|FONTWIDTH(16)
};
static struct ipodfb_info fb_info;
static struct ipodfb_par current_par;
static int current_par_valid = 0;
static struct display disp;

static struct fb_var_screeninfo default_var;

int ipodfb_init(void);
int ipodfb_setup(char*);

/* ------------------- chipset specific functions -------------------------- */

static void ipod_get_par(struct ipodfb_par *, const struct fb_info *);
static int ipod_encode_var(struct fb_var_screeninfo *, struct ipodfb_par *, const struct fb_info *);

static void ipod_detect(void)
{
	/*
	 *  This function should detect the current video mode settings and store
	 *  it as the default video mode
	 */

	struct ipodfb_par par;

	ipod_get_par(&par, NULL);
	ipod_encode_var(&default_var, &par, NULL);
}

static int ipod_encode_fix(struct fb_fix_screeninfo *fix, struct ipodfb_par *par,
			  const struct fb_info *info)
{
	/*
	 *  This function should fill in the 'fix' structure based on the values
	 *  in the `par' structure.
	 */

	memset(fix, 0x0, sizeof(*fix));

	strcpy(fix->id, "iPod");
	/* required for mmap() */
	fix->smem_start = (unsigned long)ipod_scr;

	fix->type = FB_TYPE_PACKED_PIXELS;

	if (ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
		fix->visual = FB_VISUAL_TRUECOLOR;
		fix->line_length = lcd_width << 1;	/* cfb16 default */
		fix->smem_len = lcd_height * lcd_width * 2;
	} else {
		fix->visual = FB_VISUAL_PSEUDOCOLOR;	/* fixed visual */
		fix->line_length = lcd_width >> 2;	/* cfb2 default */
		fix->smem_len = lcd_height * (lcd_width/4);
	}

	fix->xpanstep = 0;	/* no hardware panning */
	fix->ypanstep = 0;	/* no hardware panning */
	fix->ywrapstep = 0;	/* */

	fix->accel = FB_ACCEL_NONE;

	return 0;
}

static int ipod_decode_var(struct fb_var_screeninfo *var, struct ipodfb_par *par,
			  const struct fb_info *info)
{
	/*
	 *  Get the video params out of 'var'. If a value doesn't fit, round it up,
	 *  if it's too big, return -EINVAL.
	 *
	 *  Suggestion: Round up in the following order: bits_per_pixel, xres,
	 *  yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,
	 *  bitfields, horizontal timing, vertical timing.
	 */

	if ( var->xres > lcd_width ||
		var->yres > lcd_height ||
		var->xres_virtual != var->xres ||
		var->yres_virtual != var->yres ||
		var->xoffset != 0 ||
		var->yoffset != 0 ) {
		return -EINVAL;
	}

	if (ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
		if ( var->bits_per_pixel != 16 ) {
			return -EINVAL;
		}
	} else {
		if ( var->bits_per_pixel != 2 ) {
			return -EINVAL;
		}
	}

	return 0;
}

static int ipod_encode_var(struct fb_var_screeninfo *var, struct ipodfb_par *par,
			  const struct fb_info *info)
{
	/*
	 *  Fill the 'var' structure based on the values in 'par' and maybe other
	 *  values read out of the hardware.
	 */

	var->xres = lcd_width;
	var->yres = lcd_height;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;
	var->xoffset = 0;
	var->yoffset = 0;

	if (ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
		var->bits_per_pixel = 16;

		var->red.offset = 0;
		var->red.length = 5;

		var->green.offset = 5;
		var->green.length = 6;

		var->blue.offset = 11;
		var->blue.length = 5;

		var->transp.offset = 0;
		var->transp.length = 0;

		var->red.msb_right = 0;
		var->green.msb_right = 0;
		var->blue.msb_right = 0;
		var->transp.msb_right = 0;
	} else {
		var->bits_per_pixel = 2;
		var->grayscale = 1;
	}

	return 0;
}

static void ipod_get_par(struct ipodfb_par *par, const struct fb_info *info)
{
	/*
	 *  Fill the hardware's 'par' structure.
	 */

	if ( current_par_valid ) {
		*par = current_par;
	}
	else {
		/* ... */
	}
}

static void ipod_set_par(struct ipodfb_par *par, const struct fb_info *info)
{
	/*
	 *  Set the hardware according to 'par'.
	 */

	current_par = *par;
	current_par_valid = 1;

	/* ... */
}

static int ipod_getcolreg(unsigned regno, unsigned *red, unsigned *green,
			 unsigned *blue, unsigned *transp,
			 const struct fb_info *info)
{
	/*
	 *  Read a single color register and split it into colors/transparent.
	 *  The return values must have a 16 bit magnitude.
	 *  Return != 0 for invalid regno.
	 */

	if (regno >= 16) return 1;
	if (ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
		*red   = (ipod_fbcon_cmap[regno]      ) & 0xf800;
		*green = (ipod_fbcon_cmap[regno] <<  5) & 0xfc00;
		*blue  = (ipod_fbcon_cmap[regno] << 11) & 0xf800;
		*transp = 0;
	}
	return 0;
}

static int ipod_setcolreg(unsigned regno, unsigned red, unsigned green,
			 unsigned blue, unsigned transp,
			 const struct fb_info *info)
{
	/*
	 *  Set a single color register. The values supplied have a 16 bit
	 *  magnitude.
	 *  Return != 0 for invalid regno.
	 */

	if (regno >= 16) return 1;
	if (ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
		ipod_fbcon_cmap[regno] = ((red & 0xf800) |
					 ((green & 0xfc00) >> 5) |
					 ((blue & 0xf800) >> 11));
	}
	return 0;
}

static int ipod_pan_display(struct fb_var_screeninfo *var,
			   struct ipodfb_par *par, const struct fb_info *info)
{
	/*
	 *  Pan (or wrap, depending on the `vmode' field) the display using the
	 *  `xoffset' and `yoffset' fields of the `var' structure.
	 *  If the values don't fit, return -EINVAL.
	 */

	/* ... */
	return -EINVAL;
}

static int ipod_blank(int blank_mode, const struct fb_info *info)
{
	static int backlight_on = -1;

	if (ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
		return 0;
	}

	switch (blank_mode) {
	case VESA_NO_BLANKING:
		/* start oscillation
		 * wait 10ms
		 * cancel standby
		 * turn on LCD power
		 */
		lcd_cmd_and_data(0x0, 0x0, 0x1);
		udelay(10000);
		lcd_cmd_and_data(0x3, 0x15, 0x0);
		lcd_cmd_and_data(0x3, 0x15, 0xc);

		if (backlight_on != -1) {
			set_backlight(backlight_on);
		}
		backlight_on = -1;
		break;

	case VESA_VSYNC_SUSPEND:
	case VESA_HSYNC_SUSPEND:
		if (backlight_on == -1) {
			backlight_on = get_backlight();
			set_backlight(0);
		}

		/* go to SLP = 1 */
		/* 10101 00001100 */
		lcd_cmd_and_data(0x3, 0x15, 0x0);
		lcd_cmd_and_data(0x3, 0x15, 0x2);
		break;

	case VESA_POWERDOWN:
		if (backlight_on == -1) {
			backlight_on = get_backlight();
			set_backlight(0);
		}

		/* got to standby */
		lcd_cmd_and_data(0x3, 0x15, 0x1);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static void ipod_set_disp(const void *par, struct display *disp,
			 struct fb_info_gen *info)
{
	/*
	 *  Fill in a pointer with the virtual address of the mapped frame buffer.
	 *  Fill in a pointer to appropriate low level text console operations (and
	 *  optionally a pointer to help data) for the video mode `par' of your
	 *  video hardware. These can be generic software routines, or hardware
	 *  accelerated routines specifically tailored for your hardware.
	 *  If you don't have any appropriate operations, you must fill in a
	 *  pointer to dummy operations, and there will be no text output.
	 */

	disp->screen_base = ipod_scr;
	if (ipod_hw_ver == 0x6 || ipod_hw_ver == 0xb || ipod_hw_ver == 0xc) {
		disp->dispsw = &fbcon_ipod16;
		disp->dispsw_data = ipod_fbcon_cmap;
	}
	else {
		disp->dispsw = &fbcon_ipod;
	}
}


/* ------------ Interfaces to hardware functions ------------ */


struct fbgen_hwswitch ipod_switch = {
	detect:		ipod_detect,
	encode_fix:	ipod_encode_fix,
	decode_var:	ipod_decode_var,
	encode_var:	ipod_encode_var,
	get_par:	ipod_get_par,
	set_par:	ipod_set_par,
	getcolreg:	ipod_getcolreg,
	setcolreg:	ipod_setcolreg,
	pan_display:	ipod_pan_display,
	blank:		ipod_blank,
	set_disp:	ipod_set_disp,
};


/* ------------------------------------------------------------------------- */


/*
 *  Frame buffer operations
 */

static int ipod_fp_open(const struct fb_info *info, int user)
{
	return 0;
}

#define FBIOGET_CONTRAST	_IOR('F', 0x22, int)
#define FBIOPUT_CONTRAST	_IOW('F', 0x23, int)

#define FBIOGET_BACKLIGHT	_IOR('F', 0x24, int)
#define FBIOPUT_BACKLIGHT	_IOW('F', 0x25, int)

#define IPOD_MIN_CONTRAST 0
#define IPOD_MAX_CONTRAST 0x7f

static int ipod_fb_ioctl(struct inode *inode, struct file *file, u_int cmd,
	u_long arg, int con, struct fb_info *info)

{
	int val;

	switch (cmd) {
	case FBIOGET_CONTRAST:
		val = get_contrast();
		if (put_user(val, (int *)arg))
			return -EFAULT;
		break;

	case FBIOPUT_CONTRAST:
		val = (int)arg;
		if (val < IPOD_MIN_CONTRAST || val > IPOD_MAX_CONTRAST)
			return -EINVAL;
		set_contrast(val);
		break;

	case FBIOGET_BACKLIGHT:
		val = get_backlight();
		if (put_user(val, (int *)arg))
			return -EFAULT;
		break;

	case FBIOPUT_BACKLIGHT:
		val = (int)arg;
		set_backlight(val);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}


/*
 *  In most cases the `generic' routines (fbgen_*) should be satisfactory.
 *  However, you're free to fill in your own replacements.
 */

static struct fb_ops ipodfb_ops = {
	owner:		THIS_MODULE,
	fb_open:	ipod_fp_open,
	fb_get_fix:	fbgen_get_fix,
	fb_get_var:	fbgen_get_var,
	fb_set_var:	fbgen_set_var,
	fb_get_cmap:	fbgen_get_cmap,
	fb_set_cmap:	fbgen_set_cmap,
	fb_pan_display:	fbgen_pan_display,
	fb_ioctl:	ipod_fb_ioctl,
};


/* ------------ Hardware Independent Functions ------------ */


/*
 *  Initialization
 */

int __init ipodfb_init(void)
{
	
	ipod_hw_ver = ipod_get_hw_version() >> 16;
	switch (ipod_hw_ver)
	{
	case 0xb: /* 5g/video */
		lcd_type = 5;
		lcd_width = IPOD_5G_LCD_WIDTH;
		lcd_height = IPOD_5G_LCD_HEIGHT;
		ipod_rtc = IPOD_PP5020_RTC;
		break;

	case 0xc: /* nano */
		lcd_type = 1;
		lcd_width = IPOD_NANO_LCD_WIDTH;
		lcd_height = IPOD_NANO_LCD_HEIGHT;
		ipod_rtc = IPOD_PP5020_RTC;
		lcd_base = 0x70008a0c;
		lcd_busy_mask = 0x80000000;
		break;

	case 6: /* photo */
		if (ipod_get_hw_version() == 0x60000) {
			lcd_type = 0;
		} else {
			int gpio_a01, gpio_a04;

			/* A01 */
			gpio_a01 = (inl(0x6000D030) & 0x2) >> 1;
			/* A04 */
			gpio_a04 = (inl(0x6000D030) & 0x10) >> 4;

			printk(KERN_ERR "lcd: %d %d\n", gpio_a01, gpio_a04);
			if (((gpio_a01 << 1) | gpio_a04) == 0 || ((gpio_a01 << 1) | gpio_a04) == 2) {
				lcd_type = 0;
			} else {
				lcd_type = 1;
			}
		}

		lcd_width = IPOD_PHOTO_LCD_WIDTH;
		lcd_height = IPOD_PHOTO_LCD_HEIGHT;
		ipod_rtc = IPOD_PP5020_RTC;
		lcd_base = 0x70008a0c;
		lcd_busy_mask = 0x80000000;
		break;
			
	case 5: /* 4g */
		lcd_width = IPOD_STD_LCD_WIDTH;
		lcd_height = IPOD_STD_LCD_HEIGHT;
		ipod_rtc = IPOD_PP5020_RTC;
		lcd_base = IPOD_PP5020_LCD_BASE;
		lcd_busy_mask = 0x8000;
		break;

	case 7: /* mini g2 */
	case 4: /* mini */
		lcd_width = IPOD_MINI_LCD_WIDTH;
		lcd_height = IPOD_MINI_LCD_HEIGHT;	
		ipod_rtc = IPOD_PP5020_RTC;
		lcd_base = IPOD_PP5020_LCD_BASE;
		lcd_busy_mask = 0x8000;
		break;

	case 3: /* 3g */
	case 2: /* 2g */
	case 1: /* 1g */
		lcd_width = IPOD_STD_LCD_WIDTH;
		lcd_height = IPOD_STD_LCD_HEIGHT;
		ipod_rtc = IPOD_PP5002_RTC;
		lcd_base = IPOD_PP5002_LCD_BASE;
		lcd_busy_mask = 0x8000;
		break;
	}

	fb_info.gen.fbhw = &ipod_switch;

	fb_info.gen.fbhw->detect();

	strcpy(fb_info.gen.info.modename, "iPod");

	fb_info.gen.info.changevar = NULL;
	fb_info.gen.info.node = -1;
	fb_info.gen.info.fbops = &ipodfb_ops;
	fb_info.gen.info.disp = &disp;
	fb_info.gen.info.switch_con = &fbgen_switch;
	fb_info.gen.info.updatevar = &fbgen_update_var;
	fb_info.gen.info.blank = &fbgen_blank;
	fb_info.gen.info.flags = FBINFO_FLAG_DEFAULT;

	/* This should give a reasonable default video mode */
	fbgen_get_var(&disp.var, -1, &fb_info.gen.info);
	fbgen_do_set_var(&disp.var, 1, &fb_info.gen);
	fbgen_set_disp(-1, &fb_info.gen);
	fbgen_install_cmap(0, &fb_info.gen);

	if (register_framebuffer(&fb_info.gen.info) < 0) {
		return -EINVAL;
	}

	init_lcd();

	printk(KERN_INFO "fb%d: %s frame buffer device\n", GET_FB_IDX(fb_info.gen.info.node), fb_info.gen.info.modename);

	/* uncomment this if your driver cannot be unloaded */
	/* MOD_INC_USE_COUNT; */
	return 0;
}


/*
 *  Cleanup
 */

void ipodfb_cleanup(struct fb_info *info)
{
	/*
	 *  If your driver supports multiple boards, you should unregister and
	 *  clean up all instances.
	 */

	unregister_framebuffer(info);
	/* ... */
}


/*
 *  Setup
 */

int __init ipodfb_setup(char *options)
{
	/* Parse user speficied options (`video=ipodfb:') */
	return 0;
}



/* ------------------------------------------------------------------------- */


/*
 *  Modularization
 */

#ifdef MODULE
MODULE_LICENSE("GPL");
int init_module(void)
{
	return ipodfb_init();
}

void cleanup_module(void)
{
	ipodfb_cleanup(void);
}
#endif /* MODULE */

