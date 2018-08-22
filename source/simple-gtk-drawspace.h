#ifndef _SIMPLE_GTK_DRAWSPACE_
#define _SIMPLE_GTK_DRAWSPACE_

#include <gtk/gtk.h>
#include <pthread.h>
#include <semaphore.h>
#include <queue>
#include <string>
using namespace std;

class SimpleGTKDrawspace
{

    typedef void (*DrawFunction) (SimpleGTKDrawspace* );
    typedef void (*KeyFunction) (string);
public:
	void init(unsigned int in_sizeX, unsigned int in_sizeY, DrawFunction in_drawFunction, KeyFunction in_keyFunction);
	static gboolean key_event(GtkWidget* widget, GdkEventKey* event, gpointer in_this);
    string GetPressedKey();

	void squareBrush(double in_x, double in_y, double in_size);
	void squareBrushFilled(double in_x, double in_y, double in_size);
	void moveTo(double in_x, double in_y);
	void lineTo(double in_x, double in_y);
	void line(double in_x0, double in_y0, double in_x1, double in_y1);
	void circle(double in_x, double in_y, double in_radius);
	void arc(double in_x, double in_y, double in_radius, double in_beginAngle, double in_endAngle);
	void clear();
	void clear(double in_r, double in_g, double in_b);

	void setFontSize(double in_size);
	void printText(double in_x, double in_y, const char* in_text);

	void setColor(double in_red, double in_green, double in_blue);
	void setLineWidth(double in_width);
	void setAntialiasing(unsigned int in_grade);

	void saveToPNG(const char* in_filename);

	void pauseRendering();
	void resumeRendering();
	void waitForRender();

	SimpleGTKDrawspace(int* inout_argc, char** inout_argv[]);
	SimpleGTKDrawspace();
	~SimpleGTKDrawspace();

private:
	pthread_t drawThread;
	GtkWidget *window;
	GtkWidget *verticalBox;
	GtkWidget *horizontalBox;
	GtkWidget *frame;
	GtkWidget *drawingArea;

	GtkWidget *startButton;
	GtkWidget *pauseButton;
	GtkWidget *saveButton;

	GtkWidget *savePictureDialog;

	cairo_surface_t *drawSurface;
	cairo_t *drawCairo;
	pthread_mutex_t drawSurfaceMutex;

	sem_t waitForRenderSem;

	DrawFunction drawFunction;

	bool initialised;
	bool drawingInProcess; // user's drawing function is exicuting now
	bool renderingIsPaused; // user called pauseRendering
	bool waitingForRedraw; // user called waitForRedraw

	bool drawedBetweenFrames; // Need we redraw at all?

	static void closeWindowCallback(void* in_this);

	// GTK callbacks
	static gboolean drawCallback(GtkWidget *widget, cairo_t *cr, gpointer in_this);
	static gboolean ConfigureEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer in_this);
	static gboolean motionNotifyEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer in_this);
	static void startButtonCallback(GtkWidget *widget, gpointer in_this);
	static void saveButtonCallback(GtkWidget *widget, gpointer in_this);

	static void* userDrawThreadFunc(void* in_this);


	static gboolean timerRedrawCallback(gpointer in_this);

	int* pargc;
	char*** pargv;

	double currentX, currentY;
	string pressedKey;
};

#endif
