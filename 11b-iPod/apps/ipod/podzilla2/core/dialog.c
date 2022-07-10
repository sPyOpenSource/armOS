/*
 * dialo - modal dialog box for podzilla
 * Copyright (C) 2005 Scott Lawrence
 * Major changes made by Joshua Oreman
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

#include <strings.h>	/* for strcasecmp */
#include <string.h>	/* for strncpy */
#include <stdio.h>	/* for printf */
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "pz.h"

typedef struct 
{
    const char *b0, *b1, *b2;
    int sel, buttons, by; // by = y of button tops
    int err;
    int timer;
    ttk_surface dblbuf;
} dialog_data;

extern FILE *errout;
extern void pz_set_backlight_timer(int sec);

#define _MAKETHIS dialog_data *data = (dialog_data *)this->data

// Not thread-safe.
// [text] must NOT be a string constant.
static char **wrap (int wid, ttk_font font, char *text, int *out_lines) 
{
    // *wrapat[] stores pointers to the char we wrap *after* on each line.
    // (If it's a space, tab, or nl, it's eaten.)
    static char *wrapat[1024];
    // Number of lines. Used in computing the size of our surface.
    int lines;
    // Current EOL (pointer in wrapat[])
    char **end;
    // Current char
    char *p;
    // Current beginning-of-word --> candidate EOL
    char *wend;
    // Current xpos, wordwidth
    int xpos, wordwidth;

    lines = 0;
    end = wrapat;
    p = text;
    wend = 0;
    xpos = wordwidth = 0;

    // The text width functions should be very inexpensive, so we can just
    // do them per-char and be simple.

    while (1) {
	char s[2] = { '?', 0 };
	s[0] = *p;

	if (lines > 1023) {
	    fprintf (stderr, "wrap: Too many lines\n");
	    break;
	}
	
	if (*p == ' ' || *p == '\t' || *p == '-' || *p == '\n' || !*p) {
	    int olines = lines;
	    
	    if (wend) {
		s[0] = *wend;
		xpos += ttk_text_width (font, s) * (1 + 4*(*wend == '\t'));
	    }
	    xpos += wordwidth;
	    
	    if (wend && (xpos > wid)) {
		*end++ = wend;
		lines++;
		xpos = wordwidth;
		wend = 0;
	    }
	    if (*p == '\n') {
		*end++ = p;
		lines++;
		xpos = 0;
		wend = 0;
	    }
	    
	    if (wordwidth > wid) {
		while (wordwidth > wid) {
		    int wwid = 0;
		    char *q;

		    if (!wend) wend = *(end-1);
		    if (!wend) {
			char svch = *p;
			*p = 0;
			wend = strrchr (text, ' ');
			if (!wend) wend = text;
			*p = svch;
		    }

		    for (q = wend + 1; wwid <= wid && q < p && *q; q++) {
			s[0] = *q;
			wwid += ttk_text_width (font, s);
		    }
		    wend = q - 2; // break after the last OK letter
		    wordwidth -= wwid;
		    *end++ = wend;
		    lines++;
		    xpos = 0;
		}
		xpos = 0;
		if (!*p) break;
		p--; // recheck this space/hyphen/whatever next time around
	    }
	    
	    if (olines == lines) // not NL
		wend = p;
	    
	    wordwidth = 0;
	    if (!*p) break;
	} else {
	    wordwidth += ttk_text_width (font, s);
	}
	
	p++;
    }
    lines++;
    *end = 0;

    if (out_lines) *out_lines = lines;
    return wrapat;
}


static TApItem *dialog_get (int err, const char *prop) 
{
    TApItem *ap;
    char *buf = malloc (strlen (prop) + 8); // +8: d i a l o g . \0
    if (err)
	strcpy (buf, "error.");
    else
	strcpy (buf, "dialog.");
    strcat (buf, prop);
    ap = ttk_ap_get (buf);
    free (buf);
    return ap;
}


static TApItem dialog_empty = { "NO_SUCH_ITEM", 0, 0, 0, 0, 0 };

static TApItem *dialog_getx (int err, const char *prop) 
{
    TApItem *ret = dialog_get (err, prop);
    
    if (!ret) {
	ret = &dialog_empty;
	ret->color = ttk_makecol (0, 0, 0);
    }

    return ret;
}


static void dialog_draw (TWidget *this, ttk_surface srf) 
{
    int x, b;
    _MAKETHIS;

    ttk_blit_image (data->dblbuf, srf, 0, 0);
    x = this->w - 1;
    b = 0;
    while ((x > 0) && (b < data->buttons)) {
	const char *str = (b == 0)? data->b0 : (b == 1)? data->b1 : data->b2;
	int sel = (b == data->sel);

	x -= ttk_text_width (ttk_menufont, str) + 10;

	ttk_ap_fillrect (srf, dialog_get (data->err, (sel? "button.sel.bg" : "button.bg")),
			 x, data->by, x + ttk_text_width (ttk_menufont, str) + 10, data->by + 20);
	ttk_ap_rect (srf, dialog_get (data->err, (sel? "button.sel.border" : "button.border")),
		     x, data->by, x + ttk_text_width (ttk_menufont, str) + 10, data->by + 20);
	ttk_ap_rect (srf, dialog_get (data->err, (sel? "button.sel.inner" : "button.inner")),
		     x, data->by, x + ttk_text_width (ttk_menufont, str) + 10, data->by + 20);
	ttk_text (srf, ttk_menufont, x + 5, data->by + (20 - ttk_text_height (ttk_menufont)) / 2,
		  dialog_getx (data->err, (sel? "button.sel.fg" : "button.fg")) -> color, str);

	x -= 7; // breathing room

	b++;
    }
}


static int dialog_down (TWidget *this, int button) 
{
    _MAKETHIS;
    
    switch (button) {
    case TTK_BUTTON_ACTION:
	return TTK_EV_DONE | TTK_EV_RET(data->sel);
    default:
	ttk_widget_set_timer (this, data->timer); // reset timer
	return TTK_EV_UNUSED;
    }
}


static int dialog_scroll (TWidget *this, int dir) 
{
#ifdef IPOD
    static int sofar = 0;
    sofar += dir;
    if (sofar > -7 && sofar < 7) return 0;
    dir = sofar / 7;
    sofar = 0;
#endif
    _MAKETHIS;
    
    data->sel -= dir; // remember: scrolling right goes to lesser button numbers, since buttons packed RTL
    if (data->sel >= data->buttons) data->sel = data->buttons - 1;
    if (data->sel < 0) data->sel = 0;
    ttk_widget_set_timer (this, data->timer);
    this->dirty++;

    return TTK_EV_CLICK;
}


static int dialog_timer (TWidget *this) 
{
    _MAKETHIS;
    
    return TTK_EV_DONE | TTK_EV_RET(data->sel);
}


static void dialog_destroy (TWidget *this) 
{
    _MAKETHIS;
    ttk_free_surface (data->dblbuf);
    free (data);
}


int pz_default_do_dialog (const char *title, const char *text,
			  const char *button0, const char *button1, const char *button2,
			  int timeout, int e) 
{
    TWindow *dialog = ttk_new_window();
    TWidget *dwid = ttk_new_widget (0, 0);
    
    // wrap() will probably be called about 16 times, 17 for lots of text.
    int w, h, lines, tfh, mfh, inc = 64;
    char **wrapat;
    char *wtext = strdup (text);
    int trunc = 0;
    int wantfactor = ttk_screen->w * 100 / ttk_screen->h;

    // Height is, computed from the buttom up, 
    // 20 (for buttons) + 10 (spacing) + lines*tfheight + 7 (spacing/line) + mfheight
    // ~= 40 + lines*tfh + mfh

    tfh = ttk_text_height (ttk_textfont);
    mfh = ttk_text_height (ttk_menufont);

    do {
	// We want to wrap it such that the w/h ~= screen w/h (looks better that way).
	int gotfactor, lastgot = wantfactor;
	
	w = ttk_screen->w / 2;

	if (trunc) {
	    *(wtext + strlen (wtext) - 63) = 0;
	    strcat (wtext, "...");
	    fprintf (stderr, "%s%d",
		     (trunc > 1)? ", no " : "",
		     60*trunc);
	}
	
	do {
	    wrapat = wrap (w, ttk_textfont, wtext, &lines);
	    h = 40 + lines*tfh + mfh;
	    gotfactor = w * 100 / h;
	    
	    if (gotfactor < wantfactor) { // need more width
		if (lastgot > wantfactor) inc >>= 1;
		w += inc;
	    } else { // need less width
		if (lastgot < wantfactor) inc >>= 1;
		w -= inc;
	    }
	    
	    lastgot = gotfactor;
	} while (inc);

	// Height will always be limiting factor, because of the header.
	if (h > dialog->h) {
	    if (!trunc) {
		fprintf (stderr, "Warning: dialog text (%dx%d) truncated by ", w, h);
	    }
	    trunc++;
	}
    } while (h > dialog->h);

    if (trunc) {
	fprintf (stderr, " characters (new size %dx%d)\n", w, h);
    }

    int minw = 0;
    if (button0) minw += 10 + ttk_text_width (ttk_menufont, button0);
    if (button1) minw += 10 + ttk_text_width (ttk_menufont, button1);
    if (button2) minw += 10 + ttk_text_width (ttk_menufont, button2);
    minw += 10; // breathing room
    if (minw < ttk_screen->w*3/5) minw = ttk_screen->w*3/5;
    if (w < minw) w = minw;

    wrapat = wrap (w, ttk_textfont, wtext, &lines);
    h = 40 + lines*tfh + mfh;

#define TEXTOFS 5
    dwid->w = w + TEXTOFS*2;
    dwid->h = h;
    dwid->focusable = 1;
    dwid->draw = dialog_draw;
    dwid->down = dialog_down;
    dwid->scroll = dialog_scroll;
    dwid->timer = dialog_timer;
    dwid->destroy = dialog_destroy;

    if (timeout)
	ttk_widget_set_timer (dwid, timeout * 1000);

    dwid->data = calloc (1, sizeof(dialog_data));
    dialog_data *data = (dialog_data *)dwid->data;
    
    if (button0) {
	data->buttons = 1;
	if (button1) {
	    data->buttons = 2;
	    if (button2) {
		data->buttons = 3;
	    }
	}
    } else {
	data->buttons = 0;
    }

    data->sel = 0;
    data->b0 = button0;
    data->b1 = button1;
    data->b2 = button2;
    data->err = e;
    data->timer = timeout * 1000;
    data->dblbuf = ttk_new_surface (dialog->w, dialog->h, ttk_screen->bpp);
    ttk_ap_fillrect (data->dblbuf, dialog_get (e, "bg"), 0, 0, dialog->w, dialog->h);
    
    // -- Populate data->dblbuf
#define BASELINESKIP (tfh + 1)
    int y = 0;
    char *End = wtext + strlen (wtext);
    char **end = wrapat;
    char *p = wtext;
    
    dialog->background = dialog_get (e, "bg");
    if (!dialog->background)
	dialog->background = ttk_ap_get ("window.bg");
    
    ttk_text (data->dblbuf, ttk_menufont, 0, y, dialog_getx (e, "title.fg") -> color, title); y += mfh;
    ttk_ap_hline (data->dblbuf, dialog_get (e, "line"), 0, dwid->w, y + 2); y += 7;

    while (p < End) {
	if (!*end) {
	    ttk_text (data->dblbuf, ttk_textfont, TEXTOFS, y, dialog_getx (e, "fg") -> color, p);
	    p = End;
	} else {
	    char *nextstart = *end + 1;
	    char svch = *nextstart;
	    *nextstart = 0;
	    ttk_text (data->dblbuf, ttk_textfont, TEXTOFS, y, dialog_getx (e, "fg") -> color, p);
	    if ((**end != ' ') && (**end != '\t') && (**end != '\n') && (**end != '-')) { // midword brk
		ttk_pixel (data->dblbuf, dwid->w - 2, y + (BASELINESKIP / 2), ttk_makecol (BLACK));
	    }
	    p = nextstart;
	    *nextstart = svch;
	    y += BASELINESKIP;
	    end++;	    
	}
    }
    y += 20;
    // Buttons will be drawn here each time box is redrawn.
    data->by = y;

    int ret;

    ttk_add_widget (dialog, dwid);
    pz_set_backlight_timer (PZ_BL_RESET);
    pz_reset_idle_timer();
    ttk_popup_window (dialog);
    ret = ttk_run();
    ttk_popdown_window (dialog);
    ttk_free_window (dialog);
    if (ttk_windows) {
	ttk_draw_window (ttk_windows->w);
    } else {
	ttk_fillrect (ttk_screen->srf, ttk_screen->wx,
		      ttk_screen->wy,
		      ttk_screen->w, ttk_screen->h,
		      ttk_makecol (255, 255, 255));
    }
    ttk_gfx_update (ttk_screen->srf);

    return ret;
}

int (*pz_do_dialog)(const char *, const char *, const char *, const char *, const char *,
		    int, int) = pz_default_do_dialog;

int pz_dialog (const char *title, const char *text,
	       int nbuttons, int timeout, ...) 
{
    char *b[3];
    char **bp = b;
    va_list ap;
    
    va_start (ap, timeout);
    b[0] = b[1] = b[2] = 0;
    while (nbuttons) {
	*bp++ = va_arg (ap, char *);
	nbuttons--;
    }
    va_end (ap);

    return pz_do_dialog (title, text, b[0], b[1], b[2], timeout, 0);
}

int pz_errdialog (const char *title, const char *text,
		  int nbuttons, int timeout, ...) 
{
    char *b[3];
    char **bp = b;
    va_list ap;
    
    va_start (ap, timeout);
    b[0] = b[1] = b[2] = 0;
    while (nbuttons) {
	*bp++ = va_arg (ap, char *);
	nbuttons--;
    }
    va_end (ap);

    return pz_do_dialog (title, text, b[0], b[1], b[2], timeout, 1);
}

void pz_message_title (const char *title, const char *text) 
{
    pz_do_dialog (title, text, _("Ok"), 0, 0, 0, 0);
}
void pz_message (const char *text) 
{
#ifdef IPOD
    FILE *fp = fopen ("msg.inf", "a");
    fprintf (fp, "I- %%%p \"%s\" len %ld\n", text, text, strlen (text));
    fclose (fp);
#endif
    pz_do_dialog (_("Information"), text, _("Ok"), 0, 0, 0, 0);
}
static void warn_or_err (const char *title, const char *fmt, va_list ap)
{
    char *str = malloc (1024);
    vsnprintf (str, 1024, fmt, ap);
    pz_do_dialog (title, str, _("Ok"), 0, 0, 0, 1);
    fprintf (errout, "%s: %s\n", title, str);
}
void pz_warning (const char *fmt, ...) 
{
    va_list ap;
    va_start (ap, fmt);
    warn_or_err (_("Warning"), fmt, ap);
    va_end (ap);
}
void pz_error (const char *fmt, ...) 
{
    va_list ap;
    va_start (ap, fmt);
    warn_or_err (_("Error"), fmt, ap);
    va_end (ap);
}
void pz_perror (const char *firstpart) 
{
    pz_error ("%s: %s", firstpart, strerror (errno));
}
