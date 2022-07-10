/*
 * Copyright (C) 2004 Bernard Leach
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <fcntl.h>
#ifdef IPOD
#include <linux/fb.h>
#endif
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "settings.h"
#include "ipod.h"
#include "pz.h"

#define FBIOGET_CONTRAST	_IOR('F', 0x22, int)
#define FBIOSET_CONTRAST	_IOW('F', 0x23, int)

#define FBIOGET_BACKLIGHT	_IOR('F', 0x24, int)
#define FBIOSET_BACKLIGHT	_IOW('F', 0x25, int)

#define FB_DEV_NAME		"/dev/fb0"
#define FB_DEVFS_NAME		"/dev/fb/0"

#define inb(a) (*(volatile unsigned char *) (a))
#define outb(a,b) (*(volatile unsigned char *) (b) = (a))

#define inl(a) (*(volatile unsigned long *) (a))
#define outl(a,b) (*(volatile unsigned long *) (b) = (a))

static int ipod_ioctl(int request, int *arg)
{
#ifdef IPOD
	int fd;

	fd = open(FB_DEV_NAME, O_NONBLOCK);
	if (fd < 0) fd = open(FB_DEVFS_NAME, O_NONBLOCK);
	if (fd < 0) {
		return -1;
	}
	if (ioctl(fd, request, arg) < 0) {
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
#else
	return -1;
#endif
}

int ipod_constrain( int min, int max, int val )
{
	if( val > max ) val = max;
	if( val < min ) val = min;
	return( val );
}

int ipod_get_contrast(void)
{
	int contrast;

	if (ipod_ioctl(FBIOGET_CONTRAST, &contrast) < 0) {
		return -1;
	}

	return contrast;
}

int ipod_set_contrast(int contrast)
{
	contrast = ipod_constrain( 0, 128, contrast ); /* just in case */
	if (ipod_ioctl(FBIOSET_CONTRAST, (int *) contrast) < 0) {
		return -1;
	}

	return 0;
}

int ipod_get_backlight(void)
{
	int backlight;

	if (ipod_ioctl(FBIOGET_BACKLIGHT, &backlight) < 0) {
		return -1;
	}

	return backlight;
}

int ipod_set_backlight(int backlight)
{
	if (ipod_ioctl(FBIOSET_BACKLIGHT, (int *) backlight) < 0) {
		return -1;
	}

	return 0;
}

int ipod_set_backlight_timer(int timer)
{
	int times[] = {0, 1, 2, 5, 10, 30, 60, 0};
	GrSetScreenSaverTimeout(times[timer]);
	ipod_set_backlight(timer ? 1 : 0);
	return 0;
}

int ipod_set_setting(short setting, int value)
{
	if (value <= 0) {
		value = 0;
	}

	set_int_setting(setting, value);
	switch (setting) {
	case CONTRAST:
		ipod_set_contrast(value);
		break;	
	case BACKLIGHT:
		ipod_set_backlight(value);
		break;
	case BACKLIGHT_TIMER:
		ipod_set_backlight_timer(value);
		break;
	case COLORSCHEME:
		appearance_set_color_scheme(value);
		break;
	case DECORATIONS:
		appearance_set_decorations(value);
		break;
	}
	
	return 0;
}

int ipod_get_setting(short setting)
{
	int value;

	value = get_int_setting(setting);	
	if (value <= 0) {
		value = 0;
	}
	return value;
}

int ipod_load_settings(void)
{
	int ret;

	if ((ret = load_settings(IPOD_SETTINGS_FILE)) < 0) {
		if (ret == -2) {
			pz_error(_("Corrupt settings file, blasting."));
			unlink(IPOD_SETTINGS_FILE);
		}
		else {
			printf(_("Failed to open %s to read settings, using defaults.\n"), IPOD_SETTINGS_FILE);
		}

		ipod_set_setting(CONTRAST, ipod_get_contrast());
		ipod_set_setting(CLICKER, 1);
		ipod_set_setting(WHEEL_DEBOUNCE, 3);
		ipod_set_setting(ACTION_DEBOUNCE, 400);
		ipod_set_setting(DSPFREQUENCY, 0);
		ipod_set_setting(COLORSCHEME, 0);
		ipod_set_setting(SLIDE_TRANSIT, 1);
	}

	ipod_set_contrast(ipod_get_setting(CONTRAST));
	ipod_set_backlight_timer(ipod_get_setting(BACKLIGHT_TIMER));
	appearance_set_color_scheme(ipod_get_setting(COLORSCHEME));
	appearance_set_decorations(ipod_get_setting(DECORATIONS));

	return 0;
}

int ipod_save_settings(void)
{
	if (save_settings(IPOD_SETTINGS_FILE) < 0) {
		pz_error(_("Save failed."));
	}

	return 0;
}

void ipod_reset_settings(void)
{
	unlink(IPOD_SETTINGS_FILE);
	free_all_settings();

	ipod_load_settings();
	ipod_save_settings();
}

void ipod_touch_settings(void)
{
	FILE *fp;
	if(( fp = fopen(IPOD_SETTINGS_FILE, "a+")) != 0 ){
	    fclose(fp);
	}
}

/*
 * 0: screen on
 * 1,2: screen off
 * 3: screen power down
 */
int ipod_set_blank_mode(int blank)
{
#ifdef IPOD
	if (ipod_ioctl(FBIOBLANK, (int *) blank) < 0) {
		return -1;
	}

#endif
	return 0;
}

void ipod_beep(void)
{
#ifdef IPOD
	if (hw_version >= 0x4) {
		int i, j;
		outl(inl(0x70000010) & ~0xc, 0x70000010);
		outl(inl(0x6000600c) | 0x20000, 0x6000600c);    /* enable device */
		for (j = 0; j < 10; j++) {
			for (i = 0; i < 0x888; i++ ) {
				outl(0x80000000 | 0x800000 | i, 0x7000a000); /* set pitch */
			}
		}
		outl(0x0, 0x7000a000);    /* piezo off */
	} else {
		static int fd = -1; 
		static char buf;

		if (fd == -1 && (fd = open("/dev/ttyS1", O_WRONLY)) == -1
				&& (fd = open("/dev/tts/1", O_WRONLY)) == -1) {
			return;
		}
    	
		write(fd, &buf, 1);
	}
#else
	if (isatty(1)) {
		printf("\a");
	}
#endif
}

int ipod_read_apm(int *battery, int *charging)
{
#ifdef IPOD
	FILE *file;
	int ac_line_status = 0xff;
	int battery_status = 0xff;
	int battery_flag = 0xff;
	int percentage = -1;
	int time_units = -1;

	if ((file = fopen("/proc/apm", "r")) != NULL) {
		fscanf(file, "%*s %*d.%*d 0x%*02x 0x%02x 0x%02x 0x%02x %d%% %d", &ac_line_status, &battery_status, &battery_flag, &percentage, &time_units);
		fclose(file);

		if (battery) {
			*battery = time_units;
		}
		if (charging) {
			*charging = (battery_status != 0xff && battery_status & 0x3) ? 1 : 0;
		}

		return 0;
	}
#endif
	if (battery) *battery = BATTERY_LEVEL_FULL;
	if (charging) *charging = 0;
	return 0;
}

int ipod_get_battery_level(void)
{
	int battery;

	ipod_read_apm(&battery, 0);
	return battery;
}

int ipod_is_charging(void)
{
	int charging;

	ipod_read_apm(0, &charging);
	return charging;
}

long ipod_get_hw_version(void)
{
#ifdef IPOD
	int i;
	char cpuinfo[512];
	char *ptr;
	FILE *file;

	if ((file = fopen("/proc/cpuinfo", "r")) != NULL) {
		while (fgets(cpuinfo, sizeof(cpuinfo), file) != NULL)
			if (strncmp(cpuinfo, "Revision", 8) == 0)
				break;
		fclose(file);
	} else {
		return 0;
	}

	for (i = 0; !isspace(cpuinfo[i]); i++);
	for (; isspace(cpuinfo[i]); i++);
	ptr = cpuinfo + i + 2;

	return strtol(ptr, NULL, 16);
#else
	return 0;
#endif
}

