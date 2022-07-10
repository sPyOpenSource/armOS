/*
 * about.c - About box module.
 * Copyright (c) 2005 Joshua Oreman
 * Copyright (c) 2007 Courtney Cavin
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


#include "pz.h"
#include "config.h"
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifdef __linux__
#include <sys/vfs.h>
#endif

static PzModule *module;
typedef struct about_stat
{
#define ABOUT_KV        0
#define ABOUT_TITLE     1
#define ABOUT_SUBTITLE  2
#define ABOUT_IMG       3
    int type;
    char key[64];
    char value[128];
    ttk_surface img;
    struct about_stat *next;
} about_stat;
static about_stat *about_stats;
static int about_menuent = 2;
static int about_textent = 0;

static about_stat *new_stat() 
{
    about_stat *cur = about_stats;
    if (!cur) {
        cur = about_stats = calloc (1, sizeof (about_stat));
    } else {
        cur = about_stats;
        while (cur->next) cur = cur->next;
        cur->next = calloc (1, sizeof (about_stat));
        cur = cur->next;
    }
    cur->next = 0;
    return cur;
}

static about_stat *new_kvstat (const char *k)
{
    about_stat *ret = new_stat();
    ret->type = ABOUT_KV;
    strcpy (ret->key, k);
    about_textent++;
    return ret;
}

static about_stat *new_title (const char *title) 
{
    about_stat *ret = new_stat();
    ret->type = ABOUT_TITLE;
    strcpy (ret->value, title);
    about_menuent++;
    return ret;
}

static about_stat *new_subtitle (const char *subtitle) 
{
    about_stat *ret = new_stat();
    strcpy (ret->value, subtitle);
    ret->type = ABOUT_SUBTITLE;
    about_textent += 2;
    return ret;
}

static about_stat *new_kvstat_sysinfo (const char *key, const char *sikey) 
{
    static char *sidata = 0;
    about_stat *ret = new_kvstat (key);
    char scratch[128];
    
    if (!sidata) {
#ifdef IPOD
        FILE *fp = fopen ("/iPod_Control/Device/SysInfo", "r");
#else
        FILE *fp = fopen ("iPod_Control/Device/SysInfo", "r");
#endif
        if (fp) {
            long off;
            fseek (fp, 0, SEEK_END);
            off = ftell (fp);
            fseek (fp, 0, SEEK_SET);
            sidata = malloc (off + 1);
            if (sidata) {
                fread (sidata, 1, off, fp);
                sidata[off] = 0;
            }
            fclose (fp);
        }
    }
    if (!sidata) {
        strcpy (ret->value, "!!!");
        return ret;
    }

    sprintf (scratch, "\n%s: ", sikey);
    if (!strstr (sidata, scratch)) {
        strcpy (ret->value, "???");
        return ret;
    }
    strncpy (scratch, strstr (sidata, scratch) + strlen (scratch), 120);
    scratch[120] = 0;
    if (strchr (scratch, '\r')) *strchr (scratch, '\r') = 0;
    if (strchr (scratch, '\n')) *strchr (scratch, '\n') = 0;

    if (strncmp (scratch, "0x", 2) != 0) { /* text value */
        strcpy (ret->value, scratch);
    } else if (strchr (scratch, '(')) { /* hex value with explanation */
        strcpy (ret->value, strchr (scratch, '(') + 1);
        if (strchr (ret->value, ')')) *strchr (ret->value, ')') = 0;
    } else {                    /* hex value, no explanation */
        strcpy (ret->value, scratch + 2 + strspn (scratch + 2, "0")); /* skip leading zeroes */
    }
    return ret;
}

// size in KB
static char *humanize (unsigned long size) 
{
    static char buf[64];
    if (size >= 1048576) {
        sprintf (buf, "%ld.%01ld GB", size/1048576, size*10/1048576 % 10);
    } else if (size >= 1024) {
        sprintf (buf, "%ld.%01ld MB", size/1024, size*10/1024 % 10);
    } else {
        sprintf (buf, "%ld kB", size);
    }
    return buf;
}

static void do_partition (const char *showdev, const char *mountpt, const char *fstype) 
{
    about_stat *cur;
    struct statfs fs;

    new_subtitle (showdev);

    cur = new_kvstat (_("Filesystem"));
    if (!strcmp (fstype, "vfat"))
        strcpy (cur->value, _("FAT32 (WinPod)"));
    else if (!strcmp (fstype, "fat"))
        strcpy (cur->value, "FAT");
    else if (!strcmp (fstype, "hfs"))
        strcpy (cur->value, "HFS");
    else if (!strcmp (fstype, "hfsplus"))
        strcpy (cur->value, _("HFS+ (MacPod)"));
    else if (!strcmp (fstype, "ext2"))
        strcpy (cur->value, _("ext2 (Linux)"));
    else if (!strcmp (fstype, "ext3"))
        strcpy (cur->value, _("ext3 (Linux)"));
    else
        strcpy (cur->value, fstype);

    if (statfs (mountpt, &fs) < 0) {
        cur = new_kvstat (_("Error"));
        strcpy (cur->value, strerror (errno));
        return;
    }

    cur = new_kvstat (_("Capacity"));
    strcpy (cur->value, humanize (fs.f_blocks * (fs.f_bsize/1024)));
    cur = new_kvstat (_("Available"));
    strcpy (cur->value, humanize (fs.f_bfree * (fs.f_bsize/1024)));
}

static void do_partitions() 
{
    FILE *fp = fopen ("/proc/mounts", "r");
    char buf[128], showdev[128];
    if (!fp) return;
    
    while (fgets (buf, 128, fp)) {
        if (!strchr (buf, ' ')) continue;

        char *mountpt = strchr (buf, ' ') + 1;
        char *fstype = strchr (mountpt, ' ') + 1;

        showdev[0] = 0;

        if (!strncmp (buf, "/dev/root", 9) && ttk_get_podversion() != TTK_POD_X11)
            strcpy (buf, "/dev/hda3 "); // space is intentional

        if (!strncmp (buf, "/dev/ide/host", 13)) {
            int h = -1, t = -1, p = -1;
            sscanf (buf, "/dev/ide/host%d/bus0/target%d/lun0/part%d", &h, &t, &p);
            if ((h == -1) || (t == -1) || (p == -1))
                continue;
            /* space is intentional . */
            sprintf (buf, "/dev/hd%c%d ", 'a' + (h * 2) + t, p);
        } else if (strncmp (buf, "/dev/hd", 7) != 0) {
            continue;
        }
        *strchr (buf, ' ') = 0;
        *strchr (mountpt, ' ') = 0;
        *strchr (fstype, ' ') = 0;

        if (!strcmp (buf, "/dev/hda2"))
            strcpy (showdev, "Music");
        if (!strcmp (buf, "/dev/hda3")) {
            if (!strncmp (fstype, "hfs", 3))
                strcpy (showdev, "Music + iPL");
            else
                strcpy (showdev, "iPodLinux");
        }

        if (!showdev[0])
            strcpy (showdev, buf);

        do_partition (showdev, mountpt, fstype);
    }
    fclose (fp);
}

static int number_of_modules = 0;
static void count_modules (const char *name, const char *longname, const char *author) 
{
    number_of_modules++;
}

#define MAX_AUTHORS 40
static const char *PZ_Authors[MAX_AUTHORS + 1];
static void list_authors (const char *name, const char *longname, const char *author) 
{
	if (strcmp(author, "podzilla Team") && strcmp(author, _("Anonymous"))) {
		int i;
		for (i = 0; i < MAX_AUTHORS && PZ_Authors[i]; ++i) {
			if (!strcmp(PZ_Authors[i], author))
				return;
		}
		if (i < MAX_AUTHORS)
			PZ_Authors[i] = author;
	}
}

static void contributors()
{
	int i;
	new_title("Developers");
	for (i = 0; PZ_Developers[i]; ++i)
		new_kvstat(PZ_Developers[i]);

	new_title("Module Developers");
	pz_module_iterate(list_authors);
	for (i = 0; PZ_Authors[i]; ++i)
		new_kvstat(PZ_Authors[i]);
}

static void mpd_stats()
{
	int sock, n, c;
	fd_set rd;
	struct timeval tv;
	struct hostent *he;
	struct sockaddr_in addr;
	char *hostname = getenv("MPD_HOST");
	char *port = getenv("MPD_PORT");
	unsigned char ch;
	char line[256];

	if (hostname == NULL) hostname = "127.0.0.1";
	if (port == NULL) port = "6600";

	if ((he = gethostbyname(hostname)) == NULL) {
		herror(hostname);
		return;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr = *((struct in_addr *)he->h_addr);
	addr.sin_port = htons(atoi(port));
	memset(&(addr.sin_zero), 0, 8);
	if (connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr))==-1) {
		return;
	}

	new_title("MPD");
	send(sock, "stats\n", 6, 0);
	c = 0;
	for (;;) {
		FD_ZERO(&rd);
		FD_SET(sock, &rd);

		tv.tv_sec = 0;
		tv.tv_usec = 500;

		n = select(sock+1, &rd, NULL, NULL, &tv);
		if (!(FD_ISSET(sock, &rd) && n > 0)) break;
		if (!read(sock, &ch, 1)) break;
		if (ch == '\n')	{
			int len;
			about_stat *stat = NULL;
			line[c] = '\0'; 
			if (!strncmp(line, "artists", len = 7))
    				stat = new_kvstat(_("Artists"));
			else if (!strncmp(line, "albums", len = 6))
    				stat = new_kvstat(_("Albums"));
			else if (!strncmp(line, "songs", len = 5))
    				stat = new_kvstat(_("Songs"));
			if (stat)
				strcpy(stat->value, line+len+2);
#if 0
			if (!strncmp(line, "db_playtime", len = 11)) {
				struct tm *ti;
				time_t tim;
				stat = new_kvstat(_("DB Playtime"));
				tim = atoi(line+len+2) - 86400;
				ti = gmtime(&tim);
#ifndef IPOD
				strftime(stat->value, 128, "%-j days, "
						"%-H:%M:%S", ti);
#else
				strftime(stat->value, 128, "%j days, "
						"%H:%M:%S", ti);
#endif
			}
#endif
			c = 0;
		}
		else
			line[c++] = ch;
	}
	close(sock);
}

static void populate_stats() 
{
    about_stat *cur;
    struct utsname uts;

    /***/ new_title ("Linux");
    cur = new_kvstat (_("Modules"));
    pz_module_iterate (count_modules);
    sprintf (cur->value, "%d", number_of_modules);
    cur = new_kvstat (_("PZ Version"));
    strcpy (cur->value, VERSION);
    cur = new_kvstat (_("Kernel Version"));
    uname (&uts);
    strcpy (cur->value, uts.release);

    /***/ new_title ("iPod");

    cur = new_kvstat (_("Generation"));
    switch (ttk_get_podversion()) {
        case TTK_POD_1G: strcpy (cur->value, "1G"); break;
        case TTK_POD_2G: strcpy (cur->value, "2G"); break;
        case TTK_POD_3G: strcpy (cur->value, "3G"); break;
        case TTK_POD_4G: strcpy (cur->value, "4G"); break;
        case TTK_POD_MINI_1G: strcpy (cur->value, "Mini 1G"); break;
        case TTK_POD_MINI_2G: strcpy (cur->value, "Mini 2G"); break;
        case TTK_POD_PHOTO: strcpy (cur->value, "Photo"); break;
        case TTK_POD_NANO: strcpy (cur->value, "Nano"); break;
        case TTK_POD_VIDEO: strcpy (cur->value, "Video?!"); break;
        case TTK_POD_X11: strcpy (cur->value, _("Not an iPod")); break;
        default: strcpy (cur->value, _("SuperSecretPod")); break;
    }

    cur = new_kvstat (_("HW Version"));
    sprintf (cur->value, "%05lx", pz_ipod_get_hw_version());
    
#ifdef CONFIG_ABOUT_SHOW_SN
    new_kvstat_sysinfo (_("S/N"), "pszSerialNumber");
#endif
    new_kvstat_sysinfo (_("Model"), "ModelNumStr");
    new_kvstat_sysinfo (_("Apple FW Ver"), "buildID");

    /***/ new_title ("Disk");
    do_partitions();

    mpd_stats();
    contributors();
}

typedef struct {
    ttk_surface dblbuf;
    int top;
    int scroll, spos, sheight;
    int height;
    int epoch;
} about_data;

#define _MAKETHIS about_data *data = (about_data *)this->data

static void about_render (ttk_surface srf, int width) 
{
    about_stat *cur = about_stats;
    int y = 0;
    int mfh = ttk_text_height (ttk_menufont)*3/2, tfh = ttk_text_height (ttk_textfont) + 1;
    ttk_color col = ttk_ap_getx ("window.fg")->color;
    
    ttk_text (srf, ttk_menufont,
              (width - ttk_text_width (ttk_menufont, _("About Podzilla"))) / 2, y + 2,
              col, _("About Podzilla"));
    y += mfh;

    while (cur) {
        switch (cur->type) {
        case ABOUT_TITLE:
            ttk_text (srf, ttk_menufont, 0, y + mfh/3 - 2, col, cur->value);
            y += mfh;
            break;
        case ABOUT_SUBTITLE:
            ttk_text (srf, ttk_textfont, 0, y+(tfh*3/4), col, cur->value);
            y += 2*tfh;
            break;
        case ABOUT_KV:
            ttk_text (srf, ttk_textfont, 5, y, col, cur->key);
            if (cur->value[0]) {
            	if (ttk_text_width (ttk_textfont, cur->key) +
			ttk_text_width (ttk_textfont, cur->value) > width)
                    y += tfh;
            
                ttk_text (srf, ttk_textfont, width - ttk_text_width (ttk_textfont, cur->value) - 6,
                      y, col, cur->value);
            }
            y += tfh;
            break;
        }
        cur = cur->next;
    }
}

static void about_draw (TWidget *this, ttk_surface srf) 
{
    _MAKETHIS;
    int wid = this->w - 10*data->scroll;
    ttk_color bgcol = ttk_ap_getx ("window.bg")->color;

    if (data->epoch < ttk_epoch) {
        ttk_fillrect (data->dblbuf, 0, 0, this->w, data->height, bgcol);
	about_render (data->dblbuf, this->w);
	data->epoch = ttk_epoch;
    }

    ttk_blit_image_ex (data->dblbuf, 0, data->top, wid, this->h,
		       srf, this->x + 2, this->y);

    if (data->scroll) {
	ttk_ap_fillrect (srf, ttk_ap_get ("scroll.bg"), this->x + this->w - 10,
			 this->y + ttk_ap_getx ("header.line") -> spacing,
			 this->x + this->w, this->y + this->h);
	ttk_ap_rect (srf, ttk_ap_get ("scroll.box"), this->x + this->w - 10,
		     this->y + ttk_ap_getx ("header.line") -> spacing,
		     this->x + this->w, this->y + this->h);
	ttk_ap_fillrect (srf, ttk_ap_get ("scroll.bar"), this->x + this->w - 10,
			 this->y + ttk_ap_getx ("header.line") -> spacing + data->spos,
			 this->x + this->w,
			 this->y - ttk_ap_getx ("header.line") -> spacing + data->spos + data->sheight + 1);
    }
}

int about_button (TWidget *this, int button, int time) 
{
    if (button == TTK_BUTTON_MENU) {
        ttk_hide_window (this->win);
        return 0;
    }
    return TTK_EV_UNUSED;
}

int about_scroll (TWidget *this, int dir) 
{
    _MAKETHIS;
    int oldtop = data->top;
    
    data->top += 5*dir;
    if (data->top > data->height - this->h) data->top = data->height - this->h;
    if (data->top < 0) data->top = 0;

    this->dirty++;
    if (data->top != oldtop) {
        data->spos = data->top * (this->h + ttk_ap_getx ("header.line") -> spacing) / data->height;
        return TTK_EV_CLICK;
    }
    return 0;
}

void about_destroy (TWidget *this) 
{
    _MAKETHIS;
    ttk_free_surface (data->dblbuf);
    free (data);
}

static TWidget *new_about_widget() 
{
    TWidget *ret = ttk_new_widget (0, 0);
    ret->w = ttk_screen->w - ttk_screen->wx;
    ret->h = ttk_screen->h - ttk_screen->wy;
    ret->focusable = 1;
    ret->draw = about_draw;
    ret->button = about_button;
    ret->scroll = about_scroll;
    ret->destroy = about_destroy;
    ret->dirty = 1;

    if (!about_stats) populate_stats();

    ret->data = malloc (sizeof(about_data));
    about_data *data = (about_data *)ret->data;
    data->height = (about_menuent * 3 / 2) * ttk_text_height (ttk_menufont) + about_textent * (ttk_text_height (ttk_textfont) + 1);
    data->dblbuf = ttk_new_surface (ret->w, data->height, ttk_screen->bpp);
    data->top = 0;
    data->spos = 0;
    data->sheight = (ret->h + ttk_ap_getx ("header.line") -> spacing) * ret->h / data->height;
    data->epoch = ttk_epoch;
    data->scroll = (data->height > ret->h);

    about_render (data->dblbuf, ret->w - 10*data->scroll);

    return ret;
}

static TWindow *new_about_window() 
{
    TWindow *ret = pz_new_window ("About", PZ_WINDOW_NORMAL);
    ttk_add_widget (ret, new_about_widget());
    return pz_finish_window (ret);
}

static TWindow *new_license_window()
{
	TWindow *ret;
	char *buf = NULL;
	char tmp[4096];
	FILE *fp;
	long len, cs;

	if (!(fp = fopen(pz_module_get_datapath(module, "LICENSE"), "r")))
		return TTK_MENU_DONOTHING;
	fseek(fp, 0L, SEEK_END);
	len = ftell(fp);
	rewind(fp);
	if (len == 0) {
		cs = 0;
		while (fgets(tmp, 4096, fp) != NULL) {
			len = buf ? strlen(buf) : 0;
			if (len + (long)strlen(tmp) > cs) {
				cs += 8192;
				if ((buf = realloc(buf, cs)) == NULL) {
					pz_error(_("Memory allocation failed"));
					fclose(fp);
					return TTK_MENU_DONOTHING;
				}
			}
			strncpy(buf + len, tmp, cs - len);
		}
		if (buf == NULL) {
			pz_message(_("Empty file"));
			fclose(fp);
			return TTK_MENU_DONOTHING;
		}
	}
	else {
		cs = len;
		if ((buf = malloc(len + 1)) == NULL) {
			pz_error(_("Memory allocation failed"));
			fclose(fp);
			return TTK_MENU_DONOTHING;
		}
		while (cs > 0) cs -= fread(buf + (len - cs), 1, cs, fp);
		*(buf + len) = '\0'; 
	}
	fclose(fp);

	ret = pz_new_window("License", PZ_WINDOW_NORMAL);
	ret->data = 0x12345678;
	ttk_add_widget (ret, ttk_new_textarea_widget(ret->w, ret->h, buf,
		ttk_textfont, ttk_text_height(ttk_textfont)+2));
	free(buf);
	return pz_finish_window (ret);
}

static void init_about() 
{
    module = pz_register_module ("about", 0);
    pz_menu_add_action ("/Settings/About", new_about_window);
    pz_menu_add_action_group ("/Settings/License", "#General", new_license_window);
}

PZ_MOD_INIT(init_about)
