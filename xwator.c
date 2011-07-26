/*
 * (c) Copyright Brian Chapman (bchapman@microsoft.com) and 
 *               Ronald Joe Record (rr@ronrecord.com)
 *
 * Authors - Brian Chapman and Ronald Joe Record
 *
 * Xwator is a simulation of sharks and fish with the fish "swimming"
 * around and the sharks "eating" the fish. We added some rules to
 * simulate evolution amongst both the sharks and the fish.
 *
 * MODIFICATION HISTORY
 *
 * S001 - 06 Sep 1990, sco!rr
 * 	- restore original display mode prior to exiting
 * S002 - 06 Sep 1990, sco!rr
 *	- adapted to work in graphics mode
 * S003 - 09 Sep 1990, sco!rr
 *	- added a bunch of options (set width, height, resolution,
 *	  Continue and infinite modes)
 * S004 - 11 Sep 1990, sco!rr
 *	- added display of population curves (-C option).
 * S005 - 18 Sep 1990, sco!rr
 *	- removed infinite mode and changed Continue to _Continue
 *	- instead of erasing the whole curve display every PPL generations,
 *	  now just clear the next few columns ahead of where you're drawing.
 * S006 - 19 Sep 1990, sco!rr
 *	- ported to X using Motif widget set, Intrinsics, and Xlib
 * S007 - 23 Sep 1990, sco!rr
 *	- split out xwator.h; use XDrawLine() in clear_ahead();
 *	  use XtSetArg() & XtSetValues() to establish attachments dynamically;
 *	  add curve/nocurve buttons; use XFillRectangles() to draw when Rflag
 *	  is specified (i.e. each point is really a box).
 * S008 - 23 May 1995, rr@sco.com
 *  - ported to SCO OpenServer 5 and cleaned up a bit.
 */

#include "xwator.h"

main(ac,av)
int ac;
char *av[];
{
	Widget toplevel;
	int i;

	parseargs(ac,av);
	init_data();
	init_creatures();
	toplevel = XtInitialize(av[0], "Wator", NULL, 0, 
                          &ac, av);
	framework = XtCreateManagedWidget("framework", 
                          xmFormWidgetClass, toplevel, NULL, 0);
	/*
 	* Create the widget to display the wator and register
 	* callbacks for resize and refresh.
 	*/
	canvas = XtCreateManagedWidget("drawing_canvas", 
                     xmDrawingAreaWidgetClass, framework, NULL, 0);
	fish_return = XtCreateManagedWidget("fish", 
                     xmDrawingAreaWidgetClass, framework, NULL, 0);
	shark_return = XtCreateManagedWidget("sharks", 
                     xmDrawingAreaWidgetClass, framework, NULL, 0);
	population = XtCreateManagedWidget("pop_curves", 
                     xmDrawingAreaWidgetClass, framework, NULL, 0);
	/* 
 	* Create the pushbutton widgets. 
 	*/ 
	button[0] =  XtCreateManagedWidget("go_button",
                                 xmPushButtonWidgetClass,
                                 framework, NULL, 0);
	button[1] =  XtCreateManagedWidget("stop_button",
                                 xmPushButtonWidgetClass,
                                 framework, NULL, 0);
	button[2] =  XtCreateManagedWidget("quit_button",
                                 xmPushButtonWidgetClass,
                                 framework, NULL, 0);
	button[3] =  XtCreateManagedWidget("curve_button",
                                 xmPushButtonWidgetClass,
                                 framework, NULL, 0);

	init_canvas();  

	/*  
 	* Add callbacks. 
 	*/ 
	XtAddCallback(canvas, XmNexposeCallback, redisplay, Data); 
	XtAddCallback(canvas, XmNresizeCallback, resize, Data); 
	XtAddCallback(fish_return, XmNexposeCallback, redisplay, Fret); 
	XtAddCallback(fish_return, XmNresizeCallback, resize, Fret); 
	XtAddCallback(shark_return, XmNexposeCallback, redisplay, Sret);
	XtAddCallback(shark_return, XmNresizeCallback, resize, Sret); 
	XtAddCallback(population, XmNexposeCallback, redisplay, Pop); 
	XtAddCallback(population, XmNresizeCallback, resize, Pop); 
	XtAddCallback(button[0], XmNactivateCallback, start_swimming, NULL);
	XtAddCallback(button[1], XmNactivateCallback, stop_swimming, NULL);
	XtAddCallback(button[2], XmNactivateCallback, quit, NULL);
	XtAddCallback(button[3], XmNactivateCallback, curves, NULL);

	XtRealizeWidget(toplevel);
	curves();
	resize(canvas, Data, (caddr_t)NULL);
	resize(fish_return, Fret, (caddr_t)NULL);
	resize(shark_return, Sret, (caddr_t)NULL);
	resize(population, Pop, (caddr_t)NULL);
	/* register the workproc */
  	work_proc_id = XtAddWorkProc(go, canvas);
	XtMainLoop();
}

init_data()
{
	lrtb_t ocean;
	int h, w;
	struct wator_cell *wp;
	char *value;
	XColor used, exact;
	extern char *getenv();

	if((Data=(struct image_data *)malloc(sizeof(struct image_data)))==NULL){
		fprintf(stderr,"Error malloc'ing Data. Exiting\n");
		exit(-1);
	}
	if((Fret=(struct image_data *)malloc(sizeof(struct image_data)))==NULL){
		fprintf(stderr,"Error malloc'ing Fret. Exiting\n");
		exit(-1);
	}
	if((Sret=(struct image_data *)malloc(sizeof(struct image_data)))==NULL){
		fprintf(stderr,"Error malloc'ing Sret. Exiting\n");
		exit(-1);
	}
	if((Pop=(struct image_data *)malloc(sizeof(struct image_data)))==NULL){
		fprintf(stderr,"Error malloc'ing Pop. Exiting\n");
		exit(-1);
	}
	if((wator=(struct wator_cell **)malloc(HEIGHT*sizeof(wator)))==NULL) {
		fprintf(stderr,"Error malloc'ing wator. Exiting\n");
		exit(-1);
	}

	if ((dpy = XOpenDisplay(getenv("DISPLAY")))==NULL) {
		fprintf(stderr, " Error opening display. Exiting\n");
		exit(-2);
	}
	if (DisplayPlanes(dpy, 0) == 1)
		mono++;
	colors[0] = water_icon = BlackPixel(dpy, 0);

	for(h=0; h<HEIGHT; h++)
	{
	if((wator[h]=(struct wator_cell *)malloc(WIDTH*sizeof(struct wator_cell)))==NULL) {
		fprintf(stderr,"Error malloc'ing wator[%d]. Exiting\n",h);
		exit(-1);
	}
		for(w=0; w<WIDTH; w++)
		{
			wp = &(wator[h][w]);
			wp->creature = WATER;
			wp->age = 0;
			wp->mv = 1;
			wp->ate = 0;
			wp->blocked = 0;
			wp->icon = water_icon;
			wp->spec = water_spec;
		}
	}
	value = XGetDefault(dpy, "Wator", "FishColor");
	if (!value) value = "green";
	XAllocNamedColor(dpy, DefaultColormap(dpy,0),value,&used,&exact); 
	colors[1] = fish_icon = (mono) ? WhitePixel(dpy,0) : used.pixel;
	value = XGetDefault(dpy, "Wator", "SharkColor");
	if (!value) value = "yellow";
	XAllocNamedColor(dpy, DefaultColormap(dpy,0),value,&used,&exact); 
	colors[13] = shark_icon = (mono) ? WhitePixel(dpy,0) : used.pixel;
}

init_creatures()
{
	int i, fish, x, limit;
	int h, w;
	struct wator_cell *wp;

	srand( time(0) );

	for(fish=0; fish<2; fish++)
	{
		if(fish)
			limit = FISH_START;
		else
			limit = SHARK_START;
		for(i=0; i<limit; i++)
		{
			do {
				w = rand() % WIDTH;	
				h = rand() % HEIGHT;	
				wp = &(wator[h][w]);
			} while(wp->creature != WATER);

			if(fish)
			{
				wp->creature = FISH;
				wp->age = rand() % FISH_SPAWN;
				wp->icon = fish_icon;
				wp->spec = 2;
				wp->rate = FISH_SPAWN;
			}
			else
			{
				wp->creature = SHARK;
				wp->age = rand() % SHARK_SPAWN;
				wp->icon = shark_icon;
				wp->spec = 14;
				wp->rate = SHARK_STARVE;
			}
			wp->mv = 1;
		}
	}
}

init_canvas()
{
	int i, y, wide, high;
	Arg wargs[4];
	XGCValues gcv;

	/*
 	* Set the size of the drawing areas.
 	*/
	MAX_X = XDisplayWidth(XtDisplay(canvas), XDefaultScreen(XtDisplay(canvas)));
	MAX_Y = XDisplayHeight(XtDisplay(canvas), XDefaultScreen(XtDisplay(canvas)));
	if (Eflag) {
		WRES = MAX_X / WIDTH;
		HRES = MAX_Y / HEIGHT;
	}
	if (!Cflag) {
		if ((wide=(2*WIDTH*WRES)) > MAX_X)
			wide = MAX_X - MARGIN;
		if ((high=(3*HEIGHT*HRES)) > MAX_Y)
			high = MAX_Y - MARGIN;
	}
	else {
		if ((wide=(WIDTH*WRES)+MARGIN) > MAX_X)
			wide = MAX_X - MARGIN;
		if ((high=(HEIGHT*HRES)+MARGIN) > MAX_Y)
			high = MAX_Y - MARGIN;
	}
	XtSetArg(wargs[0], XtNwidth, wide);
	XtSetArg(wargs[1], XtNheight, high);
	XtSetValues(framework, wargs,2);

	for (i=0; i< MAXCOLOR; i++) {
		Data->gc[i] = XCreateGC(XtDisplay(canvas),
                     	DefaultRootWindow(XtDisplay(canvas)),
                     	0, (XGCValues *)NULL); 
		XSetForeground(XtDisplay(canvas), Data->gc[i], get_color_index(i));
		Fret->gc[i] = Sret->gc[i] = Pop->gc[i] = Data->gc[i];
	}
	Data->width = wide; Data->height = high;
	Pop->width = wide; Pop->height = high;
	Fret->width = wide; Fret->height = high;
	Sret->width = wide; Sret->height = high;
	/* Create a graphics context with foreground=background for erasing */
	gcv.foreground = gcv.background = BlackPixel(dpy, 0);
	Egc = XCreateGC(XtDisplay(canvas),
                     	DefaultRootWindow(XtDisplay(canvas)),
                     	GCForeground | GCBackground, &gcv); 
	/*
 	*  Initialize the pixmaps to all the same size.
 	*/
	Data->pix= XCreatePixmap(XtDisplay(canvas), 
			DefaultRootWindow(XtDisplay(canvas)),
            Data->width, Data->height, 
			DefaultDepthOfScreen(XtScreen(canvas)));
	Fret->pix= XCreatePixmap(XtDisplay(fish_return), 
			DefaultRootWindow(XtDisplay(fish_return)),
            Data->width, Data->height, 
			DefaultDepthOfScreen(XtScreen(fish_return)));
	Sret->pix= XCreatePixmap(XtDisplay(shark_return), 
			DefaultRootWindow(XtDisplay(shark_return)),
            Data->width, Data->height, 
			DefaultDepthOfScreen(XtScreen(shark_return)));
	Pop->pix= XCreatePixmap(XtDisplay(population), 
			DefaultRootWindow(XtDisplay(population)),
            Data->width, Data->height, 
			DefaultDepthOfScreen(XtScreen(population)));
	/* Clear the newly created pixmaps */
	XFillRectangle(XtDisplay(canvas), Data->pix, Egc, 0, 0, 
               	Data->width,  Data->height);
	XFillRectangle(XtDisplay(fish_return), Data->pix, Egc, 0, 0, 
               	Data->width,  Data->height);
	XFillRectangle(XtDisplay(shark_return), Data->pix, Egc, 0, 0, 
               	Data->width,  Data->height);
	XFillRectangle(XtDisplay(population), Data->pix, Egc, 0, 0, 
               	Data->width,  Data->height);
	/* calculate offsets */
	xoff = (Data->width - (WIDTH*WRES)) / 2;
	yoff = (Data->height - (HEIGHT*HRES)) / 2;
	if (xoff < 0)
		xoff = 0;
	if (yoff < 0)
		yoff = 0;
	AHEAD = Data->width / 10;
	/* zero the point buffer */
	init_buffer();
}

void 
resize(w, data, call_data)
Widget         w;
struct image_data    *data;
caddr_t        call_data;
{
	Arg wargs[10];
	/*  
 	*   Get the new window size.
 	*/   
	XtSetArg(wargs[0], XtNwidth,  &data->width);
	XtSetArg(wargs[1], XtNheight, &data->height);
	XtGetValues(w, wargs, 2);
	/*
 	* Clear the window.
 	*/
 	if(XtIsRealized(w))
     		XClearArea(XtDisplay(w), XtWindow(w), 0, 0, 0, 0, TRUE);
	/*
 	*  Free the old pixmap and create a new pixmap 
 	*  the size of the window.
 	*/
	if(data->pix)
   		XFreePixmap(XtDisplay(w), data->pix);
	data->pix= XCreatePixmap(XtDisplay(w),
                         	DefaultRootWindow(XtDisplay(w)),
                         	data->width, data->height, 
                         	DefaultDepthOfScreen(XtScreen(w)));
	XFillRectangle(XtDisplay(w), data->pix, Egc, 0, 0, 
               	data->width,  data->height);
	XCopyArea(XtDisplay(w), data->pix, XtWindow(w), 
			Data->gc[2], 0, 0, data->width, data->height, 0, 0);
	/* re-calculate offsets */
	if (Eflag) {
		WRES = data->width / WIDTH;
		HRES = data->height / HEIGHT;
	}
	xoff = (data->width - (WIDTH*WRES)) / 2;
	yoff = (data->height - (HEIGHT*HRES)) / 2;
	if (xoff < 0)
		xoff = 0;
	if (yoff < 0)
		yoff = 0;
	AHEAD = Pop->width / 10;
}

void 
redisplay (w, data, call_data)
Widget          w;
struct image_data     *data;
XmDrawingAreaCallbackStruct    *call_data;
{
	XExposeEvent  *event = (XExposeEvent *) call_data->event;
	/*
 	* Extract the exposed area from the event and copy
 	* from the saved pixmap to the window.
 	*/
	if(data->pix == (Pixmap)NULL)
		data->pix= XCreatePixmap(XtDisplay(w), DefaultRootWindow(XtDisplay(w)),
                  data->width, data->height, DefaultDepthOfScreen(XtScreen(w)));
	XCopyArea(XtDisplay(w), data->pix, XtWindow(w), Data->gc[2], 
           event->x, event->y, event->width, event->height, 
           event->x, event->y);
}

display()
{
	int h, w;
	struct wator_cell *wp;

	for(h=0; h<HEIGHT; h++)
	{
		for(w=0; w<WIDTH; w++)
		{
			wp = &(wator[h][w]);
			if(wp->mv)
			{
				switch(wp->creature)
				{
				case WATER:
					draw_icon(h, w, water_spec);
					/*
					 * This may look odd, it is a
					 * performance kluge.
					 */
					move_water(h, w);
					break;
				case FISH:
					draw_icon(h, w, wp->spec);
					break;
				case SHARK:
					draw_icon(h, w, wp->spec);
					break;
				}
				wp->mv = 0;
			}
		}
	}
}

draw_icon(h, w, spec)
int spec;
{
	static int i,j,H,W;

	if (Rflag) {	/* We've set the resolution to something other than 1 */
		H=h*HRES;
	 	W=w*WRES;
		buffer_box(canvas,Data,spec,xoff+W,yoff+H);
	}
	else
		buffer_bit(canvas,Data,spec, xoff+w, yoff+h);
}

Boolean
go(W, data, call_data)
Widget          W;
struct image_data     *data;
XmDrawingAreaCallbackStruct    *call_data;
{
	int h, w;
	struct wator_cell *wp;
	int fish_left=0, shark_left=0;

	for(h=0; h<HEIGHT; h++)
	{
		for(w=0; w<WIDTH; w++)
		{
			wp = &(wator[h][w]);
			if(FISH == wp->creature && !wp->mv)
			{
				if(!wp->blocked)
				{
					move_fish(h, w, wp);
					++fish_left;
				}
				else if(++wp->blocked > FISH_CRUSH)
				{
					wp->creature = WATER;
					wp->mv = 1;
				}
			}
		}
	}

	for(h=0; h<HEIGHT; h++)
	{
		for(w=0; w<WIDTH; w++)
		{
			wp = &(wator[h][w]);
			if(SHARK == wp->creature && !wp->mv) {
				move_shark(h, w, wp);
				++shark_left;
			}
		}
	}
	display();
	generations++;
	if (Cflag)
		cdisplay();
	flush_buffer();
	if (fish_left)
		return FALSE;
	else
		return TRUE;
}

xy_t moves[8] ={ {0,-1}, {1,-1}, {1,0}, {1,1},		/*   7  0  1	*/
		{0,1}, {-1,1}, {-1,0}, {-1,-1} };	/*   6  +  2	*/
							/*   5  4  3	*/

struct wator_cell *
move(m, h, w)
int m;
int h, w;
{
	int hh, ww;

	if(0==h && (7==m || 0==m || 1==m) )
		hh = HEIGHT-1;
	else if(HEIGHT-1==h && (3==m || 4==m || 5==m) )
		hh = 0;
	else
		hh = h + moves[m].y;

	if(0==w && (m==5 || 6==m || 7==m) )
		ww = WIDTH-1;
	else if(WIDTH-1==w && (1==m || 2==m || 3==m) )
		ww = 0;
	else
		ww = w + moves[m].x;

	return( &(wator[hh][ww]) );
}

struct wator_cell *
find_cell(h, w, creature)
int h, w;
{
	int m;
	int mm;
	struct wator_cell *wp;
	int hh, ww;

	mm = rand() % 8;

	for(m=(mm+1)%8; m!=mm; m=(m+1)%8)
	{
		wp = move(m, h, w);
		if(creature == wp->creature)
			return(wp);
	}
	return(0);
}

move_fish(h, w, old)
struct wator_cell *old;
{
	struct wator_cell *new;
	int x, dir;

	old->age += 1;
	if(0 == (new = find_cell(h, w, WATER)))
	{
		old->blocked = 1;
		return(0);
	}

	old->mv = 1;
	*new = *old;
	if(new->rate > new->age)
		old->creature = WATER;
	else
	{
		old->age = 0;
		new->age = -1;
		if(0 == rand() % FISH_MUTATE)
		{

			if(1 == rand()%2)
			{
				if(new->rate < 25)
					++new->rate;
			}
			else
			{
				if(new->rate > 1)
					--new->rate;
			}
			new->icon=get_color_index((new->rate % 7) + 1);
			new->spec=(new->rate % 7) + 1;
		}
	}
}

move_water(h, w)		/* clear the blocked fish around water */
int h, w;
{
	int m, hh, ww;
	struct wator_cell *wp;

	for(m=0; m<8; ++m)
	{
		wp = move(m, h, w);
		if(wp->creature == FISH)
			wp->blocked = 0;
	}
}

move_shark(h, w, old)
struct wator_cell *old;
{
	struct wator_cell *new;

	old->age += 1;
	old->ate += 1;

	if(0 != (new = find_cell(h, w, FISH)))
		old->ate = 0;
	else
		if(0 == (new = find_cell(h, w, WATER)))
			return;

	old->mv = 1;
	if(old->ate >= old->rate)	/* STARVE */
	{
		old->creature = WATER;
		return;
	}
	*new = *old;
	if(SHARK_SPAWN > new->age)
		old->creature = WATER;
	else
	{
		old->age = 0;
		new->age = -1;
		new->ate = 0;
		if(0 == rand() % SHARK_MUTATE)
		{

			if(1 == rand()%2)
			{
				if(new->rate < 25)
					++new->rate;
			}
			else
			{
				if(new->rate > 0)
					--new->rate;
			}
			new->icon = get_color_index((new->rate % 8) + 8);
			new->spec = (new->rate % 8) + 8;
		}
	}
}

int oldfish[15] = {0};

cdisplay()
{
	int h, w;
	struct wator_cell *wp;
	int numfish[15]; 
	int fish_total, shark_total;

	fish_total = shark_total = 0;
	for(h=0; h<15; h++)
		numfish[h] = 0;
	/* Get a count of the fish and shark totals */
	for(h=0; h<HEIGHT; h++)
	{
		for(w=0; w<WIDTH; w++)
		{
			wp = &(wator[h][w]);
			switch(wp->creature)
			{
			case WATER:
				break;
			case FISH:
				++numfish[wp->spec-1];
				++fish_total;
				break;
			case SHARK:
				++numfish[wp->spec-1];
				++shark_total;
				break;
			}
		}
	}
	/* Display the fish population curves */
	for(h=0; h<7; h++) {
	    /* first, the return map (nth vs (n-1)st generations */
	    if (Pflag) {
		ega_bit(fish_return,Fret,h,
			((oldfish[h] * Fret->width)+
			(3*(oldfish[h]-numfish[h])*Fret->width))/ALLCELLS,
			((numfish[h] * Fret->height) / ALLCELLS));
		}
	    oldfish[h] = numfish[h];
	    /* next, simply plot population vs time */
	    if (numfish[h] > 0) {
		    ega_bit(population,Pop,h,
			(generations % Pop->width),
			Pop->height - ((numfish[h]*Pop->height)/ALLCELLS));
		}
	}
	/* Display the shark population curves */
	for(h=7; h<15; h++) {
		/* first, the return map (nth vs (n-1)st generations */
	    if (Pflag) {
		ega_bit(shark_return,Sret,h,
			((oldfish[h] * Sret->width) / SOMECELLS)
			+(((oldfish[h]-numfish[h])*Sret->width)/SOMECELLS),
			((numfish[h] * Sret->height) / SOMECELLS));
		}
	    oldfish[h] = numfish[h];
	    /* next, simply plot population vs time */
	    if (numfish[h] > 0) {
		ega_bit(population,Pop,h,
			generations % Pop->width,
			Pop->height-((2*numfish[h]*Pop->height)/ALLCELLS));
		}
	}
	/* lastly, do the population vs time plot for total fish & sharks */
	ega_bit(population,Pop,fish_spec, generations % Pop->width,
			Pop->height-((fish_total*Pop->height)/ALLCELLS));
	ega_bit(population,Pop,shark_spec, generations % Pop->width,
			Pop->height-((shark_total*Pop->height)/SOMECELLS));
	clear_ahead(generations % Pop->width);
}

parseargs(ac,av)
int ac;
char *av[];
{
	int c;
	extern int optind;
	extern char *optarg;

	while ((c=getopt(ac,av,"CEPeiuA:M:R:S:T:w:h:m:s:t:")) != EOF)
	{	switch(c)
		{
		case 'A':
			AHEAD=atoi(optarg);
			break;
		case 'C':
			Cflag=0;
			break;
		case 'E':
			Eflag++;
			Cflag=1;
			break;
		case 'M':
			SHARK_MUTATE=atoi(optarg);
			break;
		case 'P':
			Pflag = (!Pflag);
			break;
		case 'R':
			WRES=HRES=atoi(optarg);
			Rflag++;
			break;
		case 'S':
			SHARK_SPAWN=atoi(optarg);
			break;
		case 'T':
			SHARK_STARVE=atoi(optarg);
			break;
		case 'e':
			eflag++;
			break;
		case 'h':
			HEIGHT=atoi(optarg);
			break;
		case 'm':
			FISH_MUTATE=atoi(optarg);
			break;
		case 's':
			FISH_SPAWN=atoi(optarg);
			break;
		case 't':
			FISH_CRUSH=atoi(optarg);
			break;
		case 'u':
			usage();
			exit(1);
			break;
		case 'w':
			WIDTH=atoi(optarg);
			break;
		case '?':
			usage();
			exit(1);
			break;
		}
	}
	ALLCELLS=WIDTH*HEIGHT;
	SOMECELLS=ALLCELLS/6;
	SHARK_START=ALLCELLS / 200;
	FISH_START=30 * SHARK_START;
	if (Rflag || Eflag)
		Rflag=1;
}

clear_ahead(off)
int off;
{
	static int xoffset, yoffset, i;
	Display *dpy = XtDisplay(population);
	Window win = XtWindow(population);

	if ((Pop->width - off) < AHEAD)
		xoffset = off + AHEAD - Pop->width;
	else
		xoffset = off + AHEAD;
	XDrawLine(dpy, win, Egc, xoffset, 0, xoffset, Pop->height);
}

usage()
{
    fprintf(stderr,"usage: xwator [-CEPe] [-R res] [-w width] [-h height]\n");
    fprintf(stderr,"\t[-M SHARK_MUTATE] [-S SHARK_SPAWN] [-T SHARK_STARVE]\n");
    fprintf(stderr,"\t[-m FISH_MUTATE] [-s FISH_SPAWN] [-t FISH_CRUSH]\n");
    fprintf(stderr,"\tWhere :\n\t'-C' indicates display population curves\n");
    fprintf(stderr,"\t'-E' indicates use full screen\n");
    fprintf(stderr,"\t'-P' indicates suppress population return maps\n");
    fprintf(stderr,"\t'-c' indicates continue computing\n");
    fprintf(stderr,"\t'-e' indicates erase screen every PPL generations\n");
    fprintf(stderr,"\tDefaults :\n\twidth=100 height=80\n");
    fprintf(stderr,"\tSHARK_SPAWN=10 SHARK_MUTATE=400 SHARK_STARVE=2\n");
    fprintf(stderr,"\tFISH_SPAWN=7 FISH_MUTATE=400 FISH_CRUSH=500\n");
}

void
quit(w, call_value)
Widget w;
XmAnyCallbackStruct *call_value;
{
	printf("Xwator: exiting after %d generations\n",generations);
	exit(0);
}

void
start_swimming()
{
	if(work_proc_id)
		XtRemoveWorkProc(work_proc_id);
	/*
 	* Register go() as a WorkProc.
 	*/
	work_proc_id = XtAddWorkProc(go, canvas);
}

void stop_swimming()
{
	if(work_proc_id)
		XtRemoveWorkProc(work_proc_id);
	work_proc_id = (XtWorkProcId)NULL; 
}

init_buffer()
{
	int i;

	for(i=0;i<MAXCOLOR;i++) {
		points.npoints[i] = 0;
		rects.nrects[i] = 0;
	}
}

ega_bit(w, data, index, x , y)
Widget      w;
struct image_data *data;
int         x,y,index;
{
	XDrawPoint (XtDisplay(w), data->pix, Data->gc[index], x, y);
	if(XtIsRealized(w))
		XDrawPoint (XtDisplay(w), XtWindow(w), Data->gc[index],x,y);
}

buffer_box(w, data, color, x , y)
Widget      w;
struct image_data *data;
int         color, x, y;
{
	Arg wargs[3];
	Pixel fore, back;

	if(rects.nrects[color] == MAXPOINTS - 1){
  	/*
   	* If the buffer is full, set the foreground color
   	* of the graphics context and draw the rectangles in the pixmap.
   	*/
		/* set the fill style and tiling pattern for rectangles */
		XSetFillStyle(XtDisplay(canvas), Data->gc[color], FillTiled);
		XtSetArg(wargs[0], XtNforeground, &fore);
		XtSetArg(wargs[1], XtNbackground, &back);
		XtGetValues(canvas, wargs, 2);
		XSetTile(XtDisplay(canvas), Data->gc[color], 
			XmGetPixmap(XtScreen(canvas),patterns[color],
			color, 0));
  		XFillRectangles(XtDisplay(canvas), data->pix, Data->gc[color], 
               		rects.coord[color], rects.nrects[color]);
  		/*
   		* Reset the buffer.
   		*/
  		rects.nrects[color] = 0;
		XSetFillStyle(XtDisplay(canvas), Data->gc[color], FillSolid);
	}
	/*
 	* Store the rectangles in the buffer according to its color.
 	*/
	rects.coord[color][rects.nrects[color]].x = x;
	rects.coord[color][rects.nrects[color]].y = y;
	rects.coord[color][rects.nrects[color]].width = WRES;
	rects.coord[color][rects.nrects[color]].height = HRES;
	rects.nrects[color] += 1;
}

buffer_bit(w, data, color, x , y)
Widget      w;
struct image_data *data;
int         color, x,y;
{

	if(points.npoints[color] == MAXPOINTS - 1){
  	/*
   	* If the buffer is full, set the foreground color
   	* of the graphics context and draw the points in the pixmap.
   	*/
  		XDrawPoints (XtDisplay(canvas), data->pix, Data->gc[color], 
               		points.coord[color], points.npoints[color], 
               		CoordModeOrigin);
  		/*
   		* Reset the buffer.
   		*/
  		points.npoints[color] = 0;
	}
	/*
 	* Store the point in the buffer according to its color.
 	*/
	points.coord[color][points.npoints[color]].x = x;
	points.coord[color][points.npoints[color]].y = y;
	points.npoints[color] += 1;
}

flush_buffer()
{ 
	int i;
	Arg wargs[3];
	Pixel fore, back;

	/*
 	* Check each buffer.
 	*/
	for(i=0;i<MAXCOLOR;i++) {
  	/*
   	* If there are any points in this buffer, draw them in the pixmap.
   	*/
	if (Rflag) {
  		if(rects.nrects[i]){
		    /* set the fill style and tiling pattern for rectangles */
			XSetFillStyle(XtDisplay(canvas), Data->gc[i],FillTiled);
			XtSetArg(wargs[0], XtNforeground, &fore);
			XtSetArg(wargs[1], XtNbackground, &back);
			XtGetValues(canvas, wargs, 2);
			XSetTile(XtDisplay(canvas), Data->gc[i], 
				XmGetPixmap(XtScreen(canvas),patterns[i],
				i, 0));
      			XFillRectangles(XtDisplay(canvas),Data->pix,Data->gc[i],
                   		rects.coord[i], rects.nrects[i]);
    			rects.nrects[i] = 0;
			XSetFillStyle(XtDisplay(canvas), Data->gc[i], FillSolid);
  		}
	}
	else
  		if(points.npoints[i]){
      			XDrawPoints (XtDisplay(canvas), Data->pix, Data->gc[i], 
                   		points.coord[i], points.npoints[i], 
                   		CoordModeOrigin);
    			points.npoints[i] = 0;
  		}
	}
	/* copy the pixmap into the window */
	if(XtIsRealized(canvas))
		XCopyArea(XtDisplay(canvas), Data->pix, XtWindow(canvas), 
			Data->gc[2], xoff, yoff, WIDTH*WRES, HEIGHT*HRES, 
			xoff, yoff);
}

void 
curves()
{
	Arg wargs[3];
	int wide, high;
	XmString label;

	if (Cflag) {
		Cflag=0;
		if ((wide=(WIDTH*WRES)) > MAX_X)
			wide = MAX_X - 50;
		if ((high=(HEIGHT*HRES)) > MAX_Y)
			high = MAX_Y - 30;
		XtSetArg(wargs[0], XtNwidth, wide);
		XtSetArg(wargs[1], XtNheight, high);
		XtSetValues(framework, wargs,2);
		label = XmStringCreate("CURVES ON", 
					XmSTRING_DEFAULT_CHARSET);
		XtSetArg(wargs[0], XmNlabelString, label);
		XtSetValues(button[3], wargs,1);
		XtSetArg(wargs[0], XmNtopPosition, 1);
		XtSetArg(wargs[1], XmNrightPosition, 99);
		XtSetValues(canvas, wargs, 2);
		XtUnmanageChild(fish_return);
		XtUnmanageChild(shark_return);
		XtUnmanageChild(population);
	}
	else {
		Cflag=1;
		if ((wide=(2*WIDTH*WRES)) > MAX_X)
			wide = MAX_X - 50;
		if ((high=(3*HEIGHT*HRES)) > MAX_Y)
			high = MAX_Y - 30;
		XtSetArg(wargs[0], XtNwidth, wide);
		XtSetArg(wargs[1], XtNheight, high);
		XtSetValues(framework, wargs,2);
		label = XmStringCreate("CURVES OFF", 
					XmSTRING_DEFAULT_CHARSET);
		XtSetArg(wargs[0], XmNlabelString, label);
		XtSetValues(button[3], wargs,1);
		XtSetArg(wargs[0], XmNtopPosition, 47);
		XtSetArg(wargs[1], XmNrightPosition, 59);
		XtSetValues(canvas, wargs, 2);
		XtSetArg(wargs[0], XmNbottomPosition, 45);
		XtSetArg(wargs[1], XmNleftPosition, 1);
		XtSetValues(population, wargs, 2);
		XtSetArg(wargs[1], XmNleftPosition, 61);
		XtSetValues(fish_return, wargs, 2);
		XtSetArg(wargs[0], XmNbottomPosition, 90);
		XtSetValues(shark_return, wargs, 2);
		XtManageChild(fish_return);
		XtManageChild(shark_return);
		XtManageChild(population);
		XFillRectangle(XtDisplay(fish_return), XtWindow(fish_return), 
				Egc, 0, 0, Fret->width,  Fret->height);
		XFillRectangle(XtDisplay(shark_return), XtWindow(shark_return), 
				Egc, 0, 0, Sret->width,  Sret->height);
		XFillRectangle(XtDisplay(population), XtWindow(population), 
				Egc, 0, 0, Pop->width,  Pop->height);
		XCopyArea(XtDisplay(fish_return), Fret->pix, XtWindow(fish_return), 
			Data->gc[2], 0, 0, Fret->width, Fret->height, 0, 0);
		XCopyArea(XtDisplay(shark_return), Sret->pix, XtWindow(shark_return), 
			Data->gc[2], 0, 0, Sret->width, Sret->height, 0, 0);
		XCopyArea(XtDisplay(population), Pop->pix, XtWindow(population), 
			Data->gc[2], 0, 0, Pop->width, Pop->height, 0, 0);
	}
}

get_color_index(n)
int n;
{
	char *value, option[20];
	XColor used, exact;

	sprintf(option, "Color%d", n);
	value = XGetDefault(dpy, "Wator", option);
	if (!value) value = color_names[n];
	XAllocNamedColor(dpy, DefaultColormap(dpy,0),value,&used,&exact);
	colors[n] = (mono) ? WhitePixel(dpy,0) : used.pixel;
	return(colors[n]);
}

