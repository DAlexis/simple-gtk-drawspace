#include "simple-gtk-drawspace.h"

#include <signal.h>
#include <math.h>

#ifdef SGDS_DEBUG
	#define DBG_OUT(a)	g_print(a)
#else
	#define DBG_OUT(a)
#endif

SimpleGTKDrawspace::SimpleGTKDrawspace(int* inout_argc, char** inout_argv[]) :
    m_drawSurface(0),
    m_drawCairo(0),
    m_drawFunction(0)
{
    m_pargc = inout_argc;
    m_pargv = inout_argv;
}

SimpleGTKDrawspace::~SimpleGTKDrawspace()
{
    if (m_draw_thread) m_draw_thread->join();
}

//////////////////////////////////
// Gtk events' callbacks
void SimpleGTKDrawspace::gtk_callback_close_window(void* in_this)
{
    SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;

    // And now it is time to segmentation fault! =)
    if (this_class->m_drawCairo) cairo_destroy (this_class->m_drawCairo);
    if (this_class->m_drawSurface) cairo_surface_destroy (this_class->m_drawSurface);

    gtk_main_quit();
}

gboolean SimpleGTKDrawspace::gtk_callback_draw(GtkWidget *widget, cairo_t *cr, gpointer in_this)
{
	DBG_OUT("d");
    SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;

	bool thisFrameIsWaited = false;
    if (this_class->m_waitingForRedraw) thisFrameIsWaited = true;

    //cairo_surface_write_to_png (this_class->drawSurface, "hello.png");
    if (!this_class->m_renderingIsPaused) {
        std::unique_lock<std::mutex> lck(this_class->m_draw_surface_mutex);
        cairo_set_source_surface (cr, this_class->m_drawSurface, 0, 0);
        cairo_paint (cr);
	}

	if (thisFrameIsWaited) {
        this_class->m_renering_done.set_value();
        //sem_post(&(this_class->waitForRenderSem));
        this_class->m_waitingForRedraw = false;
	}
	return FALSE;
}

gboolean SimpleGTKDrawspace::gtk_callback_configure_event(GtkWidget *widget, GdkEventMotion *event, gpointer in_this)
{
	DBG_OUT("Configure event.\n");
    SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;

    if (this_class->m_drawCairo) cairo_destroy (this_class->m_drawCairo);
    if (this_class->m_drawSurface) cairo_surface_destroy (this_class->m_drawSurface);

	// TODO test another way to create
    this_class->m_drawSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
    //this_class->drawSurface = gdk_window_create_similar_surface (gtk_widget_get_window (widget), CAIRO_CONTENT_COLOR, gtk_widget_get_allocated_width (widget), gtk_widget_get_allocated_height (widget));

	/* Initialize the surface to white */
    this_class->m_drawCairo = cairo_create(this_class->m_drawSurface);

    this_class->clear();

    //sem_post(&(this_class->readyToDraw));
	/* We've handled the configure event, no need for further processing. */
	return TRUE;
}

gboolean SimpleGTKDrawspace::gtk_callback_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer in_this)
{
    //SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;
	/* paranoia check, in case we haven't gotten a configure event */
/*	if (this_class->surface == NULL)
		return FALSE;
		*/
    //sem_post(&(this_class->readyToDraw));
	return TRUE;
}

void SimpleGTKDrawspace::gtk_callback_start_button(GtkWidget *widget, gpointer in_this)
{
    SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;
	DBG_OUT("Start pressed.\n");
    this_class->m_draw_thread.reset(new std::thread([this_class](){ this_class->run_user_draw(); }));
    //pthread_create(&(this_class->drawThread), NULL, this_class->userDrawThreadFunc, (void*) (this_class));
}

void SimpleGTKDrawspace::gtk_callback_save_button(GtkWidget *widget, gpointer in_this)
{
    SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;
	DBG_OUT("Save pressed.\n");

	// Creating file chooser dialog
    this_class->m_savePictureDialog = gtk_file_chooser_dialog_new("Save picture as...", GTK_WINDOW (this_class->m_window), GTK_FILE_CHOOSER_ACTION_SAVE,
													GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (this_class->m_savePictureDialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (this_class->m_savePictureDialog), "Untitled");

    if (gtk_dialog_run (GTK_DIALOG (this_class->m_savePictureDialog)) == GTK_RESPONSE_ACCEPT) {
		DBG_OUT("Acceptance responded.\n");
		char *filename;
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (this_class->m_savePictureDialog));
        cairo_surface_write_to_png (this_class->m_drawSurface, filename);
		g_free(filename);
	}

    gtk_widget_destroy (this_class->m_savePictureDialog);
}

gboolean SimpleGTKDrawspace::gtk_callback_timer_redraw(gpointer in_this)
{
    SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;

    if (!this_class->m_drawedBetweenFrames || this_class->m_renderingIsPaused) return TRUE;

    gtk_widget_queue_draw(this_class->m_drawingArea);

    this_class->m_drawedBetweenFrames = false;

	return TRUE;
}

//////////////////////////////////
// Threads-specific functions
void SimpleGTKDrawspace::run_user_draw()
{
    m_current_x = 0;
    m_current_y = 0;

    m_drawFunction(this);
}

// Key press callback
gboolean SimpleGTKDrawspace::gtk_callback_key_event(GtkWidget *widget, GdkEventKey *event, gpointer in_this)
{
    SimpleGTKDrawspace* this_class = (SimpleGTKDrawspace*) in_this;
    std::string pressed_key = gdk_keyval_name(event->keyval);

    if (this_class->m_key_function)
    {
        this_class->m_key_function(pressed_key);
    } else {
        this_class->m_pressed_key = pressed_key;
    }

    return TRUE;
}

//////////////////////////////////
// Draw API functions
void SimpleGTKDrawspace::init(unsigned int in_sizeX, unsigned int in_sizeY, DrawFunction in_drawFunction, KeyPressCallback in_keyFunction)
{
    m_drawedBetweenFrames = true;
    m_renderingIsPaused = false;
    m_waitingForRedraw = false;
    m_key_function = in_keyFunction;

    gtk_init (m_pargc, m_pargv);

	// Creating window object
    m_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (m_window), "Drawing Area");
    g_signal_connect (m_window, "destroy", G_CALLBACK (gtk_callback_close_window), (gpointer) this);
    gtk_container_set_border_width (GTK_CONTAINER (m_window), 4);

	// Creating frame to put GtkDrawspace to it
    m_frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (m_frame), GTK_SHADOW_IN);

	// Creating start button
    m_startButton = gtk_button_new_with_label ("Start!");
    g_signal_connect (m_startButton, "clicked", G_CALLBACK (gtk_callback_start_button), (gpointer) this);

	// Creating start button
    m_saveButton = gtk_button_new_with_label ("Save to file...");
    g_signal_connect (m_saveButton, "clicked", G_CALLBACK (gtk_callback_save_button), (gpointer) this);

	// Creating and filling horizontal box
    m_horizontalBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    gtk_box_pack_start(GTK_BOX(m_horizontalBox), m_startButton, false, true, 0);
    gtk_box_pack_end(GTK_BOX(m_horizontalBox), m_saveButton, false, true, 0);

	// Creating and filling vertical box
    m_verticalBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    gtk_box_pack_start(GTK_BOX(m_verticalBox), m_frame, true, true, 0);
    gtk_box_pack_start(GTK_BOX(m_verticalBox), m_horizontalBox, false, true, 0);

	// Adding vertical box to a window
    gtk_container_add (GTK_CONTAINER (m_window), m_verticalBox);

	//gtk_container_add (GTK_CONTAINER (window), frame);


    m_drawingArea = gtk_drawing_area_new ();
    gtk_widget_set_size_request (m_drawingArea, in_sizeX, in_sizeY);

    gtk_container_add (GTK_CONTAINER (m_frame), m_drawingArea);

    g_signal_connect (m_drawingArea, "draw", G_CALLBACK (gtk_callback_draw), (gpointer) this);

    g_signal_connect (m_drawingArea, "configure-event", G_CALLBACK (gtk_callback_configure_event), (gpointer) this);
	//g_signal_connect (drawingArea, "motion-notify-event", G_CALLBACK (motionNotifyEventCallback), (gpointer) this);
    gtk_widget_add_events(m_window, GDK_KEY_PRESS_MASK);
    g_signal_connect(m_window, "key-press-event", G_CALLBACK(gtk_callback_key_event), (gpointer) this);

    gtk_widget_show_all (m_window);

    m_drawFunction = in_drawFunction;

	// Setting up font
    cairo_select_font_face(m_drawCairo, "mono",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(m_drawCairo, 11);

	DBG_OUT("Starting update timer...\n");
    g_timeout_add(20, (GSourceFunc) gtk_callback_timer_redraw, (gpointer) this);

	DBG_OUT("Going to gtk_main()\n");

	gtk_main();
}

void SimpleGTKDrawspace::saveToPNG(const char* in_filename)
{
    cairo_surface_write_to_png (m_drawSurface, in_filename);
}

void SimpleGTKDrawspace::squareBrush(double in_x, double in_y, double in_size)
{
	double sizeDiv2 = in_size / 2.0;
    std::unique_lock<std::mutex> lck(m_draw_surface_mutex);
    cairo_rectangle(m_drawCairo, in_x - sizeDiv2, in_y - sizeDiv2, in_size, in_size);
	// TODO: understand what does this function do
    cairo_stroke(m_drawCairo);
    m_drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::squareBrushFilled(double in_x, double in_y, double in_size)
{
	double sizeDiv2 = in_size / 2.0;

    std::unique_lock<std::mutex> lck(m_draw_surface_mutex);
    cairo_rectangle(m_drawCairo, in_x - sizeDiv2, in_y - sizeDiv2, in_size, in_size);
    cairo_fill(m_drawCairo);
    cairo_stroke(m_drawCairo);
    m_drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::moveTo(double in_x, double in_y)
{
    m_current_x = in_x; m_current_y = in_y;
}

void SimpleGTKDrawspace::lineTo(double in_x, double in_y)
{
    line(m_current_x, m_current_y, in_x, in_y);
	moveTo(in_x, in_y);
}

void SimpleGTKDrawspace::line(double in_x0, double in_y0, double in_x1, double in_y1)
{
    std::unique_lock<std::mutex> lck(m_draw_surface_mutex);
    cairo_move_to (m_drawCairo, in_x0, in_y0);
    cairo_line_to (m_drawCairo, in_x1, in_y1);
    cairo_stroke (m_drawCairo);
    m_drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::circle(double in_x, double in_y, double in_radius)
{
    std::unique_lock<std::mutex> lck(m_draw_surface_mutex);
    cairo_arc(m_drawCairo, in_x, in_y, in_radius, 0, 2 * M_PI);
    cairo_stroke(m_drawCairo);
    m_drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::arc(double in_x, double in_y, double in_radius, double in_beginAngle, double in_endAngle)
{
    std::unique_lock<std::mutex> lck(m_draw_surface_mutex);
    cairo_arc(m_drawCairo, in_x, in_y, in_radius, 2*M_PI - in_endAngle, 2*M_PI - in_beginAngle);
    cairo_stroke(m_drawCairo);
    m_drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::setColor(double in_red, double in_green, double in_blue)
{
    cairo_set_source_rgba (m_drawCairo, in_red, in_green, in_blue, 1);
}

void SimpleGTKDrawspace::setLineWidth(double in_width)
{
    cairo_set_line_width (m_drawCairo, in_width);
}

void SimpleGTKDrawspace::clear()
{
	clear(1.0, 1.0, 1.0);
}

void SimpleGTKDrawspace::clear(double in_r, double in_g, double in_b)
{
    std::unique_lock<std::mutex> lck(m_draw_surface_mutex);
    cairo_set_source_rgba (m_drawCairo, in_r, in_g, in_b, 1);
    cairo_paint (m_drawCairo);
    m_drawedBetweenFrames = true;
}

void SimpleGTKDrawspace::setFontSize(double in_size)
{
    cairo_set_font_size(m_drawCairo, in_size);
}

void SimpleGTKDrawspace::setAntialiasing(unsigned int in_grade)
{
	switch(in_grade)
	{
		case 2:
            cairo_set_antialias(m_drawCairo, CAIRO_ANTIALIAS_GOOD);
		break;
		case 1:
            cairo_set_antialias(m_drawCairo, CAIRO_ANTIALIAS_DEFAULT);
		break;
		case 0:
		default:
            cairo_set_antialias(m_drawCairo, CAIRO_ANTIALIAS_NONE);
	}
}

void SimpleGTKDrawspace::printText(double in_x, double in_y, const char* in_text)
{
    cairo_move_to(m_drawCairo, in_x, in_y);
    cairo_show_text(m_drawCairo, in_text);
}

void SimpleGTKDrawspace::pauseRendering()
{
    m_renderingIsPaused = true;
}

void SimpleGTKDrawspace::resumeRendering()
{
    m_renderingIsPaused = false;
}

void SimpleGTKDrawspace::waitForRender()
{
    if (!m_drawedBetweenFrames) return;
    m_waitingForRedraw = true;
    m_renering_done.get_future().wait();
    //sem_wait(&waitForRenderSem);
}

// This function should be used in the main drawing loop
// to provide real-time single-key keyboard controls
std::string SimpleGTKDrawspace::GetPressedKey()
{
    std::string retval = this->m_pressed_key;
    m_pressed_key = "";
    return retval;
}
