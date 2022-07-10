/*
 * Copyright (c) 2005 Joshua Oreman
 *
 * This file is a part of TTK.
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

#ifndef _TTK_SLIDER_H_
#define _TTK_SLIDER_H_

TWidget *ttk_new_slider_widget (int x, int y, int w, int min, int max, int *val, const char *title);
void ttk_slider_set_bar (TWidget *_this, ttk_surface empty, ttk_surface full);
void ttk_slider_set_callback (TWidget *_this, void (*cb)(int cdata, int val), int cdata);
void ttk_slider_set_val (TWidget *_this, int newval);

void ttk_slider_draw (TWidget *_this, ttk_surface srf);
int ttk_slider_scroll (TWidget *_this, int dir);
int ttk_slider_down (TWidget *_this, int button);
void ttk_slider_free (TWidget *_this);

TWindow *ttk_mh_slider (struct ttk_menu_item *_this);
void *ttk_md_slider (int w, int min, int max, int *val);

#endif
