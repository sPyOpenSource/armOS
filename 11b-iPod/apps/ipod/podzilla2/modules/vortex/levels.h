/*
 * levels
 *
 *   contains all of the vector information for all of the levels.
 *
 *   01-2005 Scott Lawrence
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * based on information from here:
 *  http://www.ionpool.net/arcade/tempest_code_project/TempEd/technical/technical.html
 */

#ifndef __VORTEX_LEVELS_H__
#define __VORTEX_LEVELS_H__

#define uchar unsigned char

#define NLEVELS (16 * 3)

typedef struct {
	uchar order;            /* remap above */

	int x[16];              /* all x ordinates */
	int y[16];              /* all y ordinates */

	uchar angle[16];        /* all sector angles */

	int   scale;            /* scale */
	int   fscale;           /* flipper scale */
	int   y3d;              /* 3d y offset */
	int   y2d;              /* 2d offset */

	char  flags;            /* flags for the level (use LF_* below) */

	int fx[16];		/* front x values */
	int fy[16];		/* front y values */
	int rx[16];		/* rear x values */
	int ry[16];		/* rear y values */
} LEVELDATA;

/* flags for the above */
#define LF_NONE      (0x00)	/* nothing special */
#define LF_OPENWEB   (0x01)	/* web has start and end points */
#define LF_CLOSEDWEB (0x02)	/* web loops around at the end */

extern LEVELDATA vortex_levels[NLEVELS];

#endif
