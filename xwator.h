/*
 * (c) Copyright Brian Chapman (bchapman@microsoft.com) and 
 *               Ronald Joe Record (rr@ronrecord.com)
 *                
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors - Brian Chapman and Ronald Joe Record
 */

#include <stdio.h>
#include <X11/StringDefs.h> 
#include <X11/Intrinsic.h> 
#include <Xm/Xm.h> 
#include <Xm/Form.h> 
#include <Xm/PushB.h> 
#include <Xm/DrawingA.h> 
#include <Xm/ToggleB.h> 
#include <X11/Xutil.h>
/* #include "libXs.h" */

void	 curves(), nocurves();
void     resize();
void     redisplay();
void     quit();
void     start_swimming();
void     stop_swimming();
Boolean  go ();

typedef struct {
	int x, y;
} xy_t;

typedef struct {
	int left, right, top, bottom;
} lrtb_t;

#define WATER	0
#define FISH	1
#define SHARK	2

#define MAXPOINTS 4000
#define MAXCOLOR  16

#define MARGIN 50

struct{
   XPoint  coord[MAXCOLOR][MAXPOINTS];
   int     npoints[MAXCOLOR];
} points;

struct{
   XRectangle  coord[MAXCOLOR][MAXPOINTS];
   int     nrects[MAXCOLOR];
} rects;

struct wator_cell {
	char creature;	/* creature type WATER, FISH, SHARK		*/
	int icon;
	int spec;
	char rate;
	char age;	/* time since creature was born or reproduced	*/
	char mv;	/* flag: has creature moved this turn		*/
	char ate;	/* time since shark creature ate		*/
	char blocked;	/* Flag for blocked in fish			*/
} **wator;

struct image_data {
  GC           gc[MAXCOLOR];
  Pixmap       pix;
  Dimension    width, height;
} *Data, *Pop, *Fret, *Sret;

GC Egc;

static char *patterns[] = { 	"background",
				"slant_right",
				"slant_right",
				"slant_right",
				"slant_right",
				"slant_right",
				"slant_right",
				"slant_right",
				"slant_left",
				"slant_left",
				"slant_left",
				"slant_left",
				"slant_left",
				"slant_left",
				"slant_left",
				"slant_left",
			};

unsigned long shark_icon;
unsigned long fish_icon;
unsigned long water_icon;
int shark_spec=13;
int fish_spec=1;
int water_spec=0;

int Eflag=0, Rflag=0, Cflag=0, eflag=0, Pflag=1;
int WRES=1, HRES=1;
int generations=1;
int MAX_X, MAX_Y;
int WIDTH=250;
int HEIGHT=200;
int xoff=0, yoff=0;
int SOMECELLS, AHEAD=50, ALLCELLS, SHARK_START, FISH_START;
int FISH_SPAWN=7, FISH_MUTATE=400, FISH_CRUSH=500, SHARK_SPAWN=10; 
int SHARK_STARVE=2, SHARK_MUTATE=400;

extern char *malloc();
char *options[] = {"option1", "option2", "option3"};
Widget framework, canvas, fish_return, shark_return, population, button[4];
XtWorkProcId work_proc_id = (XtWorkProcId)NULL;
Display *dpy = NULL;

char color_names[MAXCOLOR][30] = { "black", "red", "green", "blue", "yellow", 
   "magenta", "cyan", "light blue", "pale green", "brown", "gray", 
   "light cyan", "pink", "violet", "firebrick", "white"};

unsigned long colors[MAXCOLOR];
int mono=0;
