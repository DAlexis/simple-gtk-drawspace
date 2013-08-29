#include <gtk/gtk.h>
#include <pthread.h>
#include <semaphore.h>
#include <queue>

class SimpleGTKDrawspace
{
	typedef void (*DrawFunction) (SimpleGTKDrawspace* );
public:
	void init(unsigned int in_sizeX, unsigned int in_sizeY, DrawFunction in_drawFunction);
	
	void squareBrush(double in_x, double in_y, double in_size);
	void moveTo(double in_x, double in_y);
	void lineTo(double in_x, double in_y);
	void line(double in_x0, double in_y0, double in_x1, double in_y1);
	
	void setColor(double in_red, double in_green, double in_blue);
	void setLineWidth(double in_width);
	
	void saveToPNG(const char* in_filename);
	
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
	
	DrawFunction drawFunction;
	
	bool initialised;
	
	static void closeWindowCallback(void* in_this);
	
	// GTK callbacks
	static gboolean drawCallback(GtkWidget *widget, cairo_t *cr, gpointer in_this);
	static gboolean ConfigureEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer in_this);
	static gboolean motionNotifyEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer in_this);
	static void startButtonCallback(GtkWidget *widget, gpointer in_this);
	static void saveButtonCallback(GtkWidget *widget, gpointer in_this);
	
	static void* userDrawThreadFunc(void* in_this);
	bool drawingInProcess;
	
	static gboolean timerRedrawCallback(gpointer in_this);
	
	void clearSurface();
	
	int* pargc;
	char*** pargv;
	
	double currentX, currentY;
};
