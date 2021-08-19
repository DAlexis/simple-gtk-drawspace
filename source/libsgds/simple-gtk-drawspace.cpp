#include "simple-gtk-drawspace.h"

#include <signal.h>
#include <math.h>

#ifdef SGDS_DEBUG
	#define DBG_OUT(a)	g_print(a)
#else
	#define DBG_OUT(a)
#endif

SimpleGTKDrawspace::SimpleGTKDrawspace(int* inout_argc, char** inout_argv[]) :
	drawSurface(0),
	drawCairo(0),
	drawFunction(0),
	initialised(false)
{
	pargc = inout_argc;
	pargv = inout_argv;
}

SimpleGTKDrawspace::SimpleGTKDrawspace() :
	drawSurface(0),
	drawCairo(0),
	drawFunction(0),
	initialised(false),
	pargc(0),
	pargv(0)
{
}

SimpleGTKDrawspace::~SimpleGTKDrawspace()
{
	pthread_mutex_destroy(&drawSurfaceMutex);
	sem_destroy(&waitForRenderSem);
}

//////////////////////////////////
// Gtk events' callbacks
void SimpleGTKDrawspace::closeWindowCallback(void* in_this)
{
	SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;


	// And now is time to segmentation fault! =)
	if (thisClass->drawCairo) cairo_destroy (thisClass->drawCairo);
	if (thisClass->drawSurface) cairo_surface_destroy (thisClass->drawSurface);

	gtk_main_quit();
}

gboolean SimpleGTKDrawspace::drawCallback(GtkWidget *widget, cairo_t *cr, gpointer in_this)
{
	DBG_OUT("d");
	SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;

	bool thisFrameIsWaited = false;
	if (thisClass->waitingForRedraw) thisFrameIsWaited = true;

	//cairo_surface_write_to_png (thisClass->drawSurface, "hello.png");
	if (!thisClass->renderingIsPaused) {
		pthread_mutex_lock(&(thisClass->drawSurfaceMutex));
			cairo_set_source_surface (cr, thisClass->drawSurface, 0, 0);
			cairo_paint (cr);
		pthread_mutex_unlock(&(thisClass->drawSurfaceMutex));
	}

	if (thisFrameIsWaited) {
		sem_post(&(thisClass->waitForRenderSem));
		thisClass->waitingForRedraw = false;
	}
	return FALSE;
}

gboolean SimpleGTKDrawspace::ConfigureEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer in_this)
{
	DBG_OUT("Configure event.\n");
	SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;

	if (thisClass->drawCairo) cairo_destroy (thisClass->drawCairo);
	if (thisClass->drawSurface) cairo_surface_destroy (thisClass->drawSurface);

	// TODO test another way to create
	thisClass->drawSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gtk_widget_get_allocated_width (widget), gtk_widget_get_allocated_height (widget));
	//thisClass->drawSurface = gdk_window_create_similar_surface (gtk_widget_get_window (widget), CAIRO_CONTENT_COLOR, gtk_widget_get_allocated_width (widget), gtk_widget_get_allocated_height (widget));

	/* Initialize the surface to white */
	thisClass->drawCairo = cairo_create(thisClass->drawSurface);

	thisClass->clear();

	//sem_post(&(thisClass->readyToDraw));
	/* We've handled the configure event, no need for further processing. */
	return TRUE;
}

gboolean SimpleGTKDrawspace::motionNotifyEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer in_this)
{
	//SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;
	/* paranoia check, in case we haven't gotten a configure event */
/*	if (thisClass->surface == NULL)
		return FALSE;
		*/
	//sem_post(&(thisClass->readyToDraw));
	return TRUE;
}

void SimpleGTKDrawspace::startButtonCallback(GtkWidget *widget, gpointer in_this)
{
	SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;
	DBG_OUT("Start pressed.\n");
	thisClass->drawingInProcess = true;
	pthread_create(&(thisClass->drawThread), NULL, thisClass->userDrawThreadFunc, (void*) (thisClass));
}

void SimpleGTKDrawspace::saveButtonCallback(GtkWidget *widget, gpointer in_this)
{
	SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;
	DBG_OUT("Save pressed.\n");

	// Creating file chooser dialog
	thisClass->savePictureDialog = gtk_file_chooser_dialog_new("Save picture as...", GTK_WINDOW (thisClass->window), GTK_FILE_CHOOSER_ACTION_SAVE,
													GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (thisClass->savePictureDialog), TRUE);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (thisClass->savePictureDialog), "Untitled");

	if (gtk_dialog_run (GTK_DIALOG (thisClass->savePictureDialog)) == GTK_RESPONSE_ACCEPT) {
		DBG_OUT("Acceptance responded.\n");
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (thisClass->savePictureDialog));
		cairo_surface_write_to_png (thisClass->drawSurface, filename);
		g_free(filename);
	}

	gtk_widget_destroy (thisClass->savePictureDialog);
}

gboolean SimpleGTKDrawspace::timerRedrawCallback(gpointer in_this)
{
	SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;

	if (!thisClass->drawedBetweenFrames || thisClass->renderingIsPaused) return TRUE;

	gtk_widget_queue_draw(thisClass->drawingArea);

	thisClass->drawedBetweenFrames = false;

	return TRUE;
}

//////////////////////////////////
// Threads-specific functions
void* SimpleGTKDrawspace::userDrawThreadFunc(void* in_this)
{
	SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;

	thisClass->currentX = 0;
	thisClass->currentY = 0;

	thisClass->drawFunction(thisClass);

	thisClass->drawingInProcess = false;
	return NULL;
}

// Key press callback
gboolean SimpleGTKDrawspace::key_event(GtkWidget *widget, GdkEventKey *event, gpointer in_this)
{
    SimpleGTKDrawspace* thisClass = (SimpleGTKDrawspace*) in_this;
    thisClass->pressedKey = gdk_keyval_name(event->keyval);
    return TRUE;
}

//////////////////////////////////
// Draw API functions
void SimpleGTKDrawspace::init(unsigned int in_sizeX, unsigned int in_sizeY, DrawFunction in_drawFunction, KeyFunction in_keyFunction)
{
	initialised = true;
	drawingInProcess = false;
	drawedBetweenFrames = true;
	renderingIsPaused = false;
	waitingForRedraw = false;

	gtk_init (pargc, pargv);

	// Creating queue mutex
	pthread_mutexattr_t mutAttributes;
	pthread_mutexattr_init(&mutAttributes);
	pthread_mutex_init(&drawSurfaceMutex, &mutAttributes);
	pthread_mutexattr_destroy(&mutAttributes);

	// Creating waiting for render semaphore
	sem_init(&waitForRenderSem, 0, 1);
	sem_wait(&waitForRenderSem);

	// Creating window object
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Drawing Area");
	g_signal_connect (window, "destroy", G_CALLBACK (closeWindowCallback), (gpointer) this);
	gtk_container_set_border_width (GTK_CONTAINER (window), 4);

	// Creating frame to put GtkDrawspace to it
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	// Creating start button
	startButton = gtk_button_new_with_label ("Start!");
	g_signal_connect (startButton, "clicked", G_CALLBACK (startButtonCallback), (gpointer) this);

	// Creating start button
	saveButton = gtk_button_new_with_label ("Save to file...");
	g_signal_connect (saveButton, "clicked", G_CALLBACK (saveButtonCallback), (gpointer) this);

	// Creating and filling horizontal box
	horizontalBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

	gtk_box_pack_start(GTK_BOX(horizontalBox), startButton, false, true, 0);
	gtk_box_pack_end(GTK_BOX(horizontalBox), saveButton, false, true, 0);

	// Creating and filling vertical box
	verticalBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

	gtk_box_pack_start(GTK_BOX(verticalBox), frame, true, true, 0);
	gtk_box_pack_start(GTK_BOX(verticalBox), horizontalBox, false, true, 0);

	// Adding vertical box to a window
	gtk_container_add (GTK_CONTAINER (window), verticalBox);

	//gtk_container_add (GTK_CONTAINER (window), frame);


	drawingArea = gtk_drawing_area_new ();
	gtk_widget_set_size_request (drawingArea, in_sizeX, in_sizeY);

	gtk_container_add (GTK_CONTAINER (frame), drawingArea);


	g_signal_connect (drawingArea, "draw", G_CALLBACK (drawCallback), (gpointer) this);

	g_signal_connect (drawingArea, "configure-event", G_CALLBACK (ConfigureEventCallback), (gpointer) this);
	//g_signal_connect (drawingArea, "motion-notify-event", G_CALLBACK (motionNotifyEventCallback), (gpointer) this);
	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
	g_signal_connect(window, "key-press-event", G_CALLBACK(key_event), (gpointer) this);

	gtk_widget_show_all (window);

	drawFunction = in_drawFunction;

	// Setting up font
	cairo_select_font_face(drawCairo, "mono",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(drawCairo, 11);

	DBG_OUT("Starting update timer...\n");
	g_timeout_add(20, (GSourceFunc) timerRedrawCallback, (gpointer) this);


	DBG_OUT("Going to gtk_main()\n");

	gtk_main();
}

void SimpleGTKDrawspace::saveToPNG(const char* in_filename)
{
	cairo_surface_write_to_png (drawSurface, in_filename);
}

void SimpleGTKDrawspace::squareBrush(double in_x, double in_y, double in_size)
{
	double sizeDiv2 = in_size / 2.0;

	pthread_mutex_lock(&drawSurfaceMutex);
	cairo_rectangle(drawCairo, in_x - sizeDiv2, in_y - sizeDiv2, in_size, in_size);
	// TODO: understand what does this function do
	cairo_stroke(drawCairo);
	pthread_mutex_unlock(&drawSurfaceMutex);
	drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::squareBrushFilled(double in_x, double in_y, double in_size)
{
	double sizeDiv2 = in_size / 2.0;

	pthread_mutex_lock(&drawSurfaceMutex);
	cairo_rectangle(drawCairo, in_x - sizeDiv2, in_y - sizeDiv2, in_size, in_size);
	cairo_fill(drawCairo);
	cairo_stroke(drawCairo);
	pthread_mutex_unlock(&drawSurfaceMutex);
	drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::moveTo(double in_x, double in_y)
{
	currentX = in_x; currentY = in_y;
}

void SimpleGTKDrawspace::lineTo(double in_x, double in_y)
{
	line(currentX, currentY, in_x, in_y);
	moveTo(in_x, in_y);
}

void SimpleGTKDrawspace::line(double in_x0, double in_y0, double in_x1, double in_y1)
{
	pthread_mutex_lock(&drawSurfaceMutex);
	cairo_move_to (drawCairo, in_x0, in_y0);
	cairo_line_to (drawCairo, in_x1, in_y1);
	cairo_stroke (drawCairo);
	pthread_mutex_unlock(&drawSurfaceMutex);
	drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::circle(double in_x, double in_y, double in_radius)
{
	pthread_mutex_lock(&drawSurfaceMutex);
	cairo_arc(drawCairo, in_x, in_y, in_radius, 0, 2 * M_PI);
	cairo_stroke(drawCairo);
	pthread_mutex_unlock(&drawSurfaceMutex);
	drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::arc(double in_x, double in_y, double in_radius, double in_beginAngle, double in_endAngle)
{
	pthread_mutex_lock(&drawSurfaceMutex);
	cairo_arc(drawCairo, in_x, in_y, in_radius, 2*M_PI - in_endAngle, 2*M_PI - in_beginAngle);
	cairo_stroke(drawCairo);
	pthread_mutex_unlock(&drawSurfaceMutex);
	drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::setColor(double in_red, double in_green, double in_blue)
{
	cairo_set_source_rgba (drawCairo, in_red, in_green, in_blue, 1);
}

void SimpleGTKDrawspace::setLineWidth(double in_width)
{
	cairo_set_line_width (drawCairo, in_width);
}

void SimpleGTKDrawspace::clear()
{
	clear(1.0, 1.0, 1.0);
}

void SimpleGTKDrawspace::clear(double in_r, double in_g, double in_b)
{
	pthread_mutex_lock(&drawSurfaceMutex);
	cairo_set_source_rgba (drawCairo, in_r, in_g, in_b, 1);
	cairo_paint (drawCairo);
	pthread_mutex_unlock(&drawSurfaceMutex);
	drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::setFontSize(double in_size)
{
	cairo_set_font_size(drawCairo, in_size);
}

void SimpleGTKDrawspace::setAntialiasing(unsigned int in_grade)
{
	switch(in_grade)
	{
		case 2:
			cairo_set_antialias(drawCairo, CAIRO_ANTIALIAS_GOOD);
		break;
		case 1:
			cairo_set_antialias(drawCairo, CAIRO_ANTIALIAS_DEFAULT);
		break;
		case 0:
		default:
			cairo_set_antialias(drawCairo, CAIRO_ANTIALIAS_NONE);
	}
}

void SimpleGTKDrawspace::printText(double in_x, double in_y, const char* in_text)
{
	cairo_move_to(drawCairo, in_x, in_y);
	cairo_show_text(drawCairo, in_text);
}

void SimpleGTKDrawspace::pauseRendering()
{
	renderingIsPaused = true;
}

void SimpleGTKDrawspace::resumeRendering()
{
	renderingIsPaused = false;
}

void SimpleGTKDrawspace::waitForRender()
{
	if (!drawedBetweenFrames) return;
	waitingForRedraw = true;
	sem_wait(&waitForRenderSem);
}

// This function should be used in the main drawing loop
// to provide real-time single-key keyboard controls
string SimpleGTKDrawspace::GetPressedKey()
{
    string retval = this->pressedKey;
    pressedKey = "";
    return retval;
}
