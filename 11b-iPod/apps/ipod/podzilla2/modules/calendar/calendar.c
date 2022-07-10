/*
 * calendar.c, a simple calendar for AV3xx
 *
 * Copyright 2004, Goetz Minuth
 * Copyright 2004, Bernard Leach, ported to iPod
 * Copyright 2005, Alastair S, funkified
 * Copyright 2006, Felix Bruns, support for schemes and all iPod screen sizes
 *
 * This File is free software; I give unlimited permission to copy and/or
 * distribute it, with or without modifications, as long as this notice is
 * preserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "pz.h"

PzModule * module;
PzWindow * window;
PzWidget * widget;
ttk_surface surface;

static int DaySpace, WeekSpace, xcalpos, ycalpos;

/*char labels[] = "MTWTFSS";

char calfont[7][24] = {
	{0x00, 0x00, 0x00, 0x86, 0x01, 0xC0, 0xCE, 0x01, 0x80, 0xFE, 0x39, 0xCF,
	 0xB6, 0x6D, 0x5B, 0x86, 0x6D, 0x1B, 0x86, 0x6D, 0x1B, 0x86, 0x39, 0x1B},
	{0x00, 0x00, 0x40, 0x78, 0x00, 0x00, 0x30, 0x00, 0x00, 0x30, 0x9B, 0xC3,
	 0x30, 0xDB, 0x06, 0x30, 0xDB, 0x07, 0x30, 0xDB, 0x80, 0x30, 0x9E, 0x03},
	{0x00, 0x00, 0x00, 0x86, 0x01, 0x18, 0xB6, 0x01, 0x18, 0xB6, 0x39, 0x1E,
	 0xFE, 0x6D, 0x1B, 0xFE, 0x7D, 0x1B, 0xCC, 0x0C, 0x1B, 0xCC, 0x38, 0x1E},
	{0x00, 0x00, 0xC0, 0x78, 0x03, 0x80, 0x30, 0x03, 0x80, 0x30, 0xCF, 0x06,
	 0x30, 0xDB, 0xC6, 0x30, 0xDB, 0x06, 0x30, 0xDB, 0xC6, 0x30, 0x9B, 0xC7},
	{0x00, 0x00, 0x40, 0xE0, 0xC1, 0x40, 0x60, 0x00, 0xC0, 0x60, 0xD4, 0x00,
	 0xE0, 0xDC, 0x00, 0x60, 0xCC, 0x40, 0x60, 0xCC, 0x00, 0x60, 0xCC, 0x80},
	{0x00, 0x00, 0x40, 0x70, 0x00, 0x43, 0xD8, 0x00, 0xC3, 0x38, 0x9C, 0x07,
	 0x70, 0x30, 0x03, 0xE0, 0x3C, 0x43, 0xD8, 0x36, 0x03, 0x70, 0x3C, 0x86},
	{0x00, 0x00, 0x40, 0x38, 0x00, 0x40, 0x6C, 0x00, 0xC0, 0x1C, 0xDB, 0x03,
	 0x38, 0xDB, 0x06, 0x70, 0xDB, 0x46, 0x6C, 0xDB, 0x06, 0x38, 0xDE, 0x86},
};*/
		
static unsigned short cal_font[10][6] = { //remove when font support works...
	{0x6700, 0x9700, 0x9700, 0x9700, 0x9700, 0x6700}, // 0
	{0x2700, 0x6700, 0x2700, 0x2700, 0x2700, 0x2700}, // 1
	{0x6700, 0x9700, 0x1700, 0x2700, 0x4700, 0xF700}, // 2
	{0x6700, 0x9700, 0x2700, 0x1700, 0x9700, 0x6700}, // 3
	{0x2700, 0x6700, 0xA700, 0xF700, 0x2700, 0x2700}, // 4
	{0xE700, 0x8700, 0xE700, 0x1700, 0x1700, 0xE700}, // 5
	{0x6700, 0x8700, 0xE700, 0x9700, 0x9700, 0x6700}, // 6
	{0xF700, 0x1700, 0x2700, 0x4700, 0x4700, 0x4700}, // 7
	{0x6700, 0x9700, 0x6700, 0x9700, 0x9700, 0x6700}, // 8
	{0x6700, 0x9700, 0x9700, 0x7700, 0x1700, 0x6700}, // 9
};

static unsigned short cal_header_font[7][16] = {
	{0x0000, 0x0000, 0x6180, 0x0300, 0x7380, 0x0100, 0x7F9C, 0xF300, 
	 0x6DB6, 0xDA00, 0x61B6, 0xD800, 0x61B6, 0xD800, 0x619C, 0xD800}, // Mon
	{0x0000, 0x0200, 0x1E00, 0x0000, 0x0C00, 0x0000, 0x0CD9, 0xC300, 
	 0x0CDB, 0x6000, 0x0CDB, 0xE000, 0x0CDB, 0x0100, 0x0C79, 0xC000}, // Tue
	{0x0000, 0x0000, 0x6180, 0x1800, 0x6D80, 0x1800, 0x6D9C, 0x7800, 
	 0x7FB6, 0xD800, 0x7FBE, 0xD800, 0x3330, 0xD800, 0x331C, 0x7800}, // Wed
	{0x0000, 0x0300, 0x1EC0, 0x0100, 0x0CC0, 0x0100, 0x0CF3, 0x6000, 
	 0x0CDB, 0x6300, 0x0CDB, 0x6000, 0x0CDB, 0x6300, 0x0CD9, 0xE300}, // Thu
	{0x0000, 0x0200, 0x0783, 0x0200, 0x0600, 0x0300, 0x062B, 0x0000, 
	 0x073B, 0x0000, 0x0633, 0x0200, 0x0633, 0x0000, 0x0633, 0x0100}, // Fri
	{0x0000, 0x0200, 0x0E00, 0xC200, 0x1B00, 0xC300, 0x1C39, 0xE000, 
	 0x0E0C, 0xC000, 0x073C, 0xC200, 0x1B6C, 0xC000, 0x0E3C, 0x6100}, // Sat
	{0x0000, 0x0200, 0x1C00, 0x0200, 0x3600, 0x0300, 0x38DB, 0xC000, 
	 0x1CDB, 0x6000, 0x0EDB, 0x6200, 0x36DB, 0x6000, 0x1C7B, 0x6100}, // Sun
};

struct today {
	int mday;		/* day of the month */
	int mon;		/* month */
	int year;		/* year since 1900 */
	int wday;		/* day of the week */
};

struct shown {
	int mday;		/* day of the month */
	int mon;		/* month */
	int year;		/* year since 1900 */
	int wday;		/* day of the week */
	int firstday;		/* first (w)day of month */
	int lastday;		/* last (w)day of month */
};

struct today today;
static int leap_year;
struct shown shown;
static int last_mday;

static int istoday;
static int selected;

static int days_in_month[2][13] = {
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

static char *month_name[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#define CAP_BG		(0)
#define CAP_BTOP	(1)
#define CAP_BBOT	(2)
#define CAP_BSIDE	(3)
#define CAP_BCORN	(4)
#define CAP_TEXT	(5)

static char *appearance[3][6] = {
	{	"box.selected.bg",
		"box.selected.border",
		"box.selected.border",
		"box.selected.border",
		"window.bg",
		"box.selected.fg" },

	{	"box.special.bg",
		"box.special.border",
		"box.special.border",
		"box.special.border",
		"window.bg",
		"box.special.fg" },

	{	"box.default.bg",
		"box.default.border",
		"box.default.border",
		"box.default.border",
		"window.bg",
		"box.default.fg" },
};

/*
 * leap year -- account for gregorian reformation in 1752
 */
static int is_leap_year(int yr);

/*
 * searches the weekday of the first day in month, relative to the given
 * values
 */
static int calc_weekday(struct shown *shown);
static void calendar_init();
static void clear_calendar();
static void draw_headers();
static void draw_calendar(int redraw, int last);

static void next_month(struct shown *shown, int step);
static void prev_month(struct shown *shown, int step);

/*
static void cal_print_bmps(void)
{
	int i, j;
	GR_BITMAP *imagebits;
	for (i=0; i < 7; i++) {
		imagebits = GrNewBitmapFromData(22, 8, 22, 8, calfont[i],
		GR_BMDATA_BYTEREVERSE);
		//printf("%d\n", GR_BITMAP_SIZE(22, 8));
		printf("\t{");
		for (j=0; j < 16; j++) {
			if (j==16-1) {
				printf("0x%04X}, // %c\n", imagebits[j], labels[i]);
			} else {
				printf("0x%04X, ", imagebits[j]);
				if (j==7)
					printf("\n\t ");
			}
		}
	}
}*/

static int event_calendar( PzEvent *event )
{
	int ret = 0;
	last_mday = shown.mday;
	switch(event->type) {
	case PZ_EVENT_SCROLL:
			TTK_SCROLLMOD( event->arg, 1 );
			if( event->arg > 0 ) {
				shown.mday++;
				if (shown.mday > days_in_month[leap_year][shown.mon]) {
					next_month(&shown, 1);
				} else {
					draw_calendar(0, -1);
				}
			} else {
				shown.mday--;
				if (shown.mday < 1) {
					prev_month(&shown, 1);
				} else {
					draw_calendar(0, 1);
				}
			}
			ret = 1;
			event->wid->dirty = 1;
		break;
	case PZ_EVENT_BUTTON_DOWN:
		switch (event->arg) {
		case PZ_BUTTON_PREVIOUS:
			prev_month(&shown, 0);
			ret = 1;
			break;

		case PZ_BUTTON_NEXT:
			next_month(&shown, 0);
			ret = 1;
			break;

		case PZ_BUTTON_MENU:
			pz_close_window (event->wid->win);
			ret = 1;
			break;
		}
		event->wid->dirty = 1;
		break;
		
	}
	return ret;
}


/*
 * leap year -- account for gregorian reformation in 1752
 */
static int is_leap_year(int yr)
{
	return ((yr) <= 1752 ? !((yr) % 4) :
		(!((yr) % 4) && ((yr) % 100)) || !((yr) % 400)) ? 1 : 0;
}

/*
 * searches the weekday of the first day in month, relative to the given
 * values
 */
static int calc_weekday(struct shown *shown)
{
	return (shown->wday + 36 - shown->mday) % 7;
}


static void calendar_init()
{
	time_t now;
	struct tm *tm;

	time(&now);
	tm = localtime(&now);

	today.wday = tm->tm_wday - 1;
	today.year = tm->tm_year + 1900;
	today.mon = tm->tm_mon + 1;
	today.mday = tm->tm_mday;

	shown.mday = today.mday;
	shown.mon = today.mon;
	shown.year = today.year;
	shown.wday = today.wday;

	shown.firstday = calc_weekday(&shown);
	leap_year = is_leap_year(shown.year);
}

static void draw_headers()
{
	int i;
	int ws = xcalpos;
	int clip[7][2] = { // dirty, dirty hack until fonts are supported.
		{4, 9}, {4, 7}, {4, 9}, {4, 7}, {3, 9}, {4, 8}, {4, 8}
	};
	
	for (i = 0; i < 7; i++) {
		if (ttk_screen->w > 138) {
			ttk_bitmap (surface, ws, ycalpos-11, 22, 8, cal_header_font[i], ttk_ap_getx("window.fg")->color);
		} else {
			ttk_bitmap (surface, ws+clip[i][0], ycalpos-11, 22, 8, cal_header_font[i], ttk_ap_getx("window.fg")->color);
			ttk_ap_fillrect (surface, ttk_ap_getx("window.bg"),
					ws+clip[i][0]+clip[i][1], ycalpos-11,
					(ws+clip[i][0]+clip[i][1]) + (22-clip[i][1]), (ycalpos-11) + 8);
		}
		ws += DaySpace;
	}
}

static void cal_draw_number(const int x, const int y, const int number)
{
	int tens, units, off=2, app;
	ttk_color col;
	if (selected) {
		app = 0;
	} else if (istoday) {
		app = 1;
	} else {
		app = 2;
	}
	col = ttk_ap_getx(appearance[app][CAP_TEXT])->color;
	if (number<100) {
		units = number%10;
		if (number-units) {
			tens = (number-units)/10;
			ttk_bitmap (surface, x, y, 5, 6, cal_font[tens], col);
			off=5;
		}
		ttk_bitmap (surface, x+off, y, 5, 6, cal_font[units], col);
	}
}

static void cal_draw_rect(int xoff, int row) {
	if(ttk_screen->bpp == 16){
		int app;
		if (selected) {
			app = 0;
		} else if (istoday) {
			app = 1;
		} else {
			app = 2;
		}

		/* background fill */
		ttk_ap_fillrect(surface, ttk_ap_getx(appearance[app][CAP_BG]),
				xoff + 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + DaySpace, ycalpos + (row-1) * WeekSpace + WeekSpace);
		/* border - this should be a proper roundrect */
			/* top */
		ttk_ap_hline(surface, ttk_ap_getx(appearance[app][CAP_BTOP]),
				xoff + 1, xoff + DaySpace - 1,
				ycalpos + (row-1) * WeekSpace + 1);
			/* bottom */
		ttk_ap_hline(surface, ttk_ap_getx(appearance[app][CAP_BBOT]),
				xoff + 1, xoff + DaySpace - 1,
				ycalpos + (row-1) * WeekSpace + WeekSpace - 1);
			/* sides */
		ttk_ap_fillrect(surface, ttk_ap_getx(appearance[app][CAP_BSIDE]),
				xoff + 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + 1, ycalpos + (row-1) * WeekSpace + WeekSpace);
		ttk_ap_fillrect(surface, ttk_ap_getx(appearance[app][CAP_BSIDE]),
				xoff + DaySpace - 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + DaySpace - 1, ycalpos + (row-1) * WeekSpace + WeekSpace);
			/* corners */
		ttk_ap_fillrect(surface, ttk_ap_getx(appearance[app][CAP_BCORN]),
				xoff + 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + 1, ycalpos + (row-1) * WeekSpace + 1);
		ttk_ap_fillrect(surface, ttk_ap_getx(appearance[app][CAP_BCORN]),
				xoff + DaySpace - 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + DaySpace - 1, ycalpos + (row-1) * WeekSpace + 1);
		ttk_ap_fillrect(surface, ttk_ap_getx(appearance[app][CAP_BCORN]),
				xoff + 1, ycalpos + (row-1) * WeekSpace + WeekSpace - 1,
				xoff + 1, ycalpos + (row-1) * WeekSpace + WeekSpace - 1);
		ttk_ap_fillrect(surface, ttk_ap_getx(appearance[app][CAP_BCORN]),
				xoff + DaySpace - 1, ycalpos + (row-1) * WeekSpace + WeekSpace - 1,
				xoff + DaySpace - 1, ycalpos + (row-1) * WeekSpace + WeekSpace - 1);
	} else {
		if (selected) {
			ttk_fillrect(surface,
				xoff + 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + DaySpace, ycalpos + (row-1) * WeekSpace + WeekSpace, ttk_makecol(BLACK));
		} else if (istoday) {
			ttk_fillrect(surface,
				xoff + 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + DaySpace, ycalpos + (row-1) * WeekSpace + WeekSpace, ttk_makecol(192, 192, 192));
		} else {
			ttk_fillrect(surface,
				xoff + 1, ycalpos + (row-1) * WeekSpace + 1,
				xoff + DaySpace, ycalpos + (row-1) * WeekSpace + WeekSpace, ttk_makecol(WHITE));
		}
	}
}

static void do_draw_calendar( PzWidget *wid, ttk_surface srf )
{
	widget = wid;
	surface = srf;
	draw_calendar(1, 0);
}

static void draw_calendar( int redraw, int last )
{
	int ws, row, pos, days_per_month, j, islast;
	char buffer[9];

	if(redraw) {
		clear_calendar();

		ttk_ap_fillrect (surface, ttk_ap_getx("window.bg"),
				xcalpos, ycalpos,
				xcalpos + (ttk_screen->w - xcalpos), ycalpos + (2 * WeekSpace));

		snprintf(buffer, 9, "%s %04d", month_name[shown.mon - 1], shown.year);
		ttk_window_title(window, buffer);

		draw_headers();

		if (shown.firstday > 6) {
			shown.firstday -= 7;
		}
	}
	row = 1;
	pos = shown.firstday;
	days_per_month = days_in_month[leap_year][shown.mon];
	ws = xcalpos + (pos * DaySpace);

	for (j = 1; j <= days_per_month; j++) {
		if ((j == today.mday) && (shown.mon == today.mon)
			&& (shown.year == today.year)) {	
			istoday=1;
		} else {
			istoday=0;
		}

		if (j == shown.mday) {
			selected = 1;
		} else {
			selected = 0;
		}

		islast = (j == shown.mday + last) ? 1 : 0;
		snprintf(buffer, 4, "%02d", j);
		if(ttk_screen->bpp == 2){
			ttk_line (surface,
				ws, ycalpos + (row-1) * WeekSpace,
				ws, ycalpos + row * WeekSpace, ttk_makecol(GREY));
			ttk_line (surface,
				ws, ycalpos + (row-1) * WeekSpace,
				ws + DaySpace, ycalpos + (row-1) * WeekSpace, ttk_makecol(GREY));
			if (j + 7 > days_per_month){
				ttk_line (surface,
				ws, ycalpos + (row) * WeekSpace,
				ws + DaySpace, ycalpos + (row) * WeekSpace, ttk_makecol(GREY));
			}
		}

		cal_draw_rect(ws, row);
		cal_draw_number(ws + 3, ycalpos + (row-1) * WeekSpace + 3, j);
		
		if (shown.mday == j) {
			shown.wday = pos;
		}
		ws += DaySpace;
		pos++;
		if (pos >= 7 && j != days_per_month) {
			if(ttk_screen->bpp == 2){
				ttk_line (surface,
					ws, ycalpos + (row-1) * WeekSpace,
					ws, ycalpos + row * WeekSpace, ttk_makecol(GREY));
			}
			row++;
			pos = 0;
			ws = xcalpos;
		}
	}

	if(ttk_screen->bpp == 2){
		ttk_line (surface,
			ws, ycalpos + (row-1) * WeekSpace,
			ws, ycalpos + row * WeekSpace, ttk_makecol(GREY));
	}

	shown.lastday = pos;
}

static void next_month(struct shown *shown, int step)
{
	shown->mon++;
	if (shown->mon > 12) {
		shown->mon = 1;
		shown->year++;
		leap_year = is_leap_year(shown->year);
	}
	if (step > 0) {
		//shown->mday = shown->mday - days_in_month[leap_year][shown->mon - 1];
		shown->mday = 1;
	}
	else if (shown->mday > days_in_month[leap_year][shown->mon]) {
		shown->mday = days_in_month[leap_year][shown->mon];
	}
	shown->firstday = shown->lastday;
	draw_calendar(1, 0);
}

static void prev_month(struct shown *shown, int step)
{
	shown->mon--;
	if (shown->mon < 1) {
		shown->mon = 12;
		shown->year--;
		leap_year = is_leap_year(shown->year);
	}
	if (step > 0) {
		shown->mday = shown->mday + days_in_month[leap_year][shown->mon];
	}
	else if (shown->mday > days_in_month[leap_year][shown->mon]) {
		shown->mday = days_in_month[leap_year][shown->mon];
	}
	shown->firstday += 7 - (days_in_month[leap_year][shown->mon] % 7);
	draw_calendar(1, 0);
}

static void clear_calendar()
{
	ttk_ap_fillrect (surface, ttk_ap_getx("window.bg"), 0, 0, ttk_screen->w, ttk_screen->h);
}


PzWindow *new_calendar_window(void)
{
	calendar_init();

	//DaySpace = round(ttk_screen->w/7)-1;
	//WeekSpace = round((ttk_screen->h)/6)-7;

	DaySpace = (int)((double)ttk_screen->w/7.0+0.5)-1;
	WeekSpace = (int)((double)ttk_screen->h/6.0+0.5)-7;

	xcalpos = (ttk_screen->w-(7*DaySpace))/2;
	ycalpos = 16;

	window = pz_new_window( "Calendar", PZ_WINDOW_NORMAL );

	widget = pz_add_widget( window, do_draw_calendar, event_calendar );

	return pz_finish_window( window );
}

void load_calendar() 
{
    module = pz_register_module ("calendar", 0);
    pz_menu_add_action_group ("/Extras/Utilities/Calendar", "Desk", new_calendar_window);
}

PZ_MOD_INIT (load_calendar)
