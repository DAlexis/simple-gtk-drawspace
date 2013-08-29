#include "simple-gtk-drawspace.h"

#include <signal.h>

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
	
	//cairo_surface_write_to_png (thisClass->drawSurface, "hello.png");
	
	pthread_mutex_lock(&(thisClass->drawSurfaceMutex));
		cairo_set_source_surface (cr, thisClass->drawSurface, 0, 0);
		cairo_paint (cr);
	pthread_mutex_unlock(&(thisClass->drawSurfaceMutex));
	
	
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
	
	thisClass->clearSurface();
	
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
	gtk_widget_queue_draw(thisClass->drawingArea);
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

//////////////////////////////////
// Draw API functions
void SimpleGTKDrawspace::init(unsigned int in_sizeX, unsigned int in_sizeY, DrawFunction in_drawFunction)
{
	initialised = true;
	drawingInProcess = false;
	
	gtk_init (pargc, pargv);
	
	// Creating queue mutex
	pthread_mutexattr_t mutAttributes;
	pthread_mutexattr_init(&mutAttributes);
	pthread_mutex_init(&drawSurfaceMutex, &mutAttributes);
	pthread_mutexattr_destroy(&mutAttributes);
	
	
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
	
	gtk_widget_show_all (window);
	
	drawFunction = in_drawFunction;
	
	DBG_OUT("Starting update timer...\n");
	g_timeout_add(33, (GSourceFunc) timerRedrawCallback, (gpointer) this);
	
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
}

void SimpleGTKDrawspace::setColor(double in_red, double in_green, double in_blue)
{
	cairo_set_source_rgba (drawCairo, in_red, in_green, in_blue, 1);
}

void SimpleGTKDrawspace::setLineWidth(double in_width)
{
	cairo_set_line_width (drawCairo, in_width);
}

void SimpleGTKDrawspace::clearSurface()
{
	pthread_mutex_lock(&drawSurfaceMutex);
	cairo_set_source_rgba (drawCairo, 1, 1, 1, 1);
	cairo_paint (drawCairo);
	pthread_mutex_unlock(&drawSurfaceMutex);
}
