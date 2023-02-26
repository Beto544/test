//modified by: Hunberto Pascual
//date: 1/31/22
//
//author: Gordon Griesel
//date: Spring 2022
//purpose: get openGL working on your personal computer
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>


//some structures

class Global {
    public:
	int xres, yres;
	int n;
	Global(); 

} g;

const int MAX_PARTICLES = 1000;

class Box {
    public:
	float w, h;
	float pos[4];
	float vel[2];
	unsigned char color[3];
	void set_color(unsigned char col[3]) {
	    memcpy(color, col,sizeof(unsigned char) *3);
	}
	Box(){
	    w = 80.0f;
	    h = 20.0f;
	    pos[0] = g.xres/2;
	    pos[1] = g.yres/2;
            pos[2] = g.yres/2;  //;
            pos[3] = g.yres/2; //
	    vel[0] = 0.0;
	    vel[1] = 0.0;
	}
	Box(int wid, int hgt, int x, int y, float v0, float v1) {
	    w = wid;
	    h = hgt;
	    pos[0] = x;
	    pos[1] = y;
        pos[2] = x;
        pos[3] = y;
	    vel[0] = v0;
	    vel[1] = v1;
	}

} box, particle[MAX_PARTICLES];


class X11_wrapper {
    private:
	Display *dpy;
	Window win;
	GLXContext glc;
    public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);



//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
    init_opengl();
    //Main loop
    int done = 0;
    while (!done) {
	//Process external events.
	while (x11.getXPending()) {
	    XEvent e = x11.getXNextEvent();
	    x11.check_resize(&e);
	    x11.check_mouse(&e);
	    done = x11.check_keys(&e);
	}
	physics();
	render();
	x11.swapBuffers();
	usleep(200);
    }
    return 0;
}

Global::Global()
{
    xres = 400;
    yres = 200;
    n = 0;
}

X11_wrapper::~X11_wrapper()
{
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    int w = g.xres, h = g.yres;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
	cout << "\n\tcannot connect to X server\n" << endl;
	exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if (vi == NULL) {
	cout << "\n\tno appropriate visual found\n" << endl;
	exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask =
	ExposureMask | KeyPressMask | KeyReleaseMask |
	ButtonPress | ButtonReleaseMask |
	PointerMotionMask |
	StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
	    InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    set_title();
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "3350 Lab2");
}

bool X11_wrapper::getXPending()
{
    //See if there are pending events.
    return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
    //Get a pending event.
    XEvent e;
    XNextEvent(dpy, &e);
    return e;
}

void X11_wrapper::swapBuffers()
{
    glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
    //window has been resized.
    g.xres = width;
    g.yres = height;
    //
    glViewport(0, 0, (GLint)width, (GLint)height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
    //The ConfigureNotify is sent by the
    //server if the window is resized.
    if (e->type != ConfigureNotify)
	return;
    XConfigureEvent xce = e->xconfigure;
    if (xce.width != g.xres || xce.height != g.yres) {
	//Window size did change.
	reshape_window(xce.width, xce.height);
    }
}
//-----------------------------------------------------------------------------


#define rnd() ((float)rand() / (float)RAND_MAX)

void make_particle(int x, int y){
    if (g.n < MAX_PARTICLES) {
	particle[g.n].w = 4;
	particle[g.n].h = 4;
	particle[g.n].pos[0] = x;
	particle[g.n].pos[1] = y;
	particle[g.n].vel[0] = ((float)rand() / (float)RAND_MAX) * 0.2 - 0.1;
	particle[g.n].vel[0] = ((float)rand() / (float)RAND_MAX) * 0.2 - 0.1;
	particle[g.n].vel[1] = 0.0;

	++g.n;
	return;
    }
}



void X11_wrapper::check_mouse(XEvent *e)
{
    static int savex = 0;
    static int savey = 0;

    //Weed out non-mouse events
    if (e->type != ButtonRelease &&
	    e->type != ButtonPress &&
	    e->type != MotionNotify) {
	//This is not a mouse event that we care about.
	return;
    }
    //
    if (e->type == ButtonRelease) {
	return;
    }
    if (e->type == ButtonPress) {
	if (e->xbutton.button==1) {
	    //Left button was pressed.
	    make_particle(e->xbutton.x, g.yres - e->xbutton.y);
	    return;
	}
	if (e->xbutton.button==3) {
	    //Right button was pressed.
	    return;
	}
    }
    if (e->type == MotionNotify) {
	//The mouse moved!
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
	    savex = e->xbutton.x;
	    savey = e->xbutton.y;
	    //Code placed here will execute whenever the mouse moves.
	    for (int i=0; i < 5; i++){
		make_particle(e->xbutton.x, g.yres - e->xbutton.y);
	    }

	    if (g.n < MAX_PARTICLES) {
		particle[g.n].w = 4;
		particle[g.n].h = 4;
		particle[g.n].pos[0] = e->xbutton.x;
		particle[g.n].pos[1] = g.yres - e->xbutton.y;
		particle[g.n].vel[0] = particle[g.n].vel[1] = 0.0;
		++g.n;
	    }

	}
    }
}

int X11_wrapper::check_keys(XEvent *e)
{
    if (e->type != KeyPress && e->type != KeyRelease)
	return 0;
    int key = XLookupKeysym(&e->xkey, 0);
    if (e->type == KeyPress) {
	switch (key) {
	    case XK_1:
		//Key 1 was pressed
		break;
	    case XK_Escape:
		//Escape key was pressed
		return 1;
	}
    }
    return 0;
}

void init_opengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, g.xres, g.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set 2D mode (no perspective)
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    //Set the screen background color
    glClearColor(0.1, 0.1, 0.1, 1.0);
    // set box color
    unsigned char c[3] = {100, 200, 100};
    box.set_color(c);
}
const float GRAVITY = 0.05;

void physics()
{
    for (int i =0; i< g.n; i++) {
	particle[i].pos[0] -= particle[i].vel[0];
	particle[i].pos[1] += particle[i].vel[1];
	particle[i].vel[1] -= GRAVITY;
	// check if particle went off screen...
	if (particle[i].pos[1] < 0.0){
	    //optimizing this code below
	    // particle[i] = particle[g.n -1];	    //g.n = g.n -1;
	    // optimized
	    particle[i] = particle[--g.n];
	}
	if (particle[i].pos[1] < box.pos[1]+box.h && 
		particle[i].pos[0] > box.pos[0]-box.w &&
		particle[1].pos[0] < box.pos[0]+box.w){
	    particle[i].vel[1] = -particle[i].vel[1] * 0.3;
	    particle[i].vel[0] += 0.01;
	}


    }
}
void render()
{
    //
    glClear(GL_COLOR_BUFFER_BIT);
    //Draw box.
    glPushMatrix();
    glColor3ubv(box.color);
    glTranslatef(box.pos[0], box.pos[1], 0.0f);
    //glTranslatef(box.pos[2], box.pos[3], 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(-box.w, -box.h);
    glVertex2f(-box.w,  box.h);
    glVertex2f( box.w,  box.h);
    glVertex2f( box.w, -box.h);
    glEnd();
    glPopMatrix();

    //Draw particle.
    for (int i =0; i< g.n; i++) {
	glPushMatrix();
	glColor3ub(150, 160, 220);
	glTranslatef(particle[i].pos[0], particle[i].pos[1], 0.0f);
	glBegin(GL_QUADS);
	glVertex2f(- particle[i].w, -particle[i].h);
	glVertex2f(- particle[i].w, particle[i].h);
	glVertex2f(  particle[i].w, particle[i].h);
	glVertex2f(  particle[i].w, -particle[i].h);
	glEnd();
	glPopMatrix();

    }

}

