#ifndef _SIMPLE_GTK_DRAWSPACE_
#define _SIMPLE_GTK_DRAWSPACE_

#include <gtk/gtk.h>
#include <queue>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <future>

class SimpleGTKDrawspace
{
public:
    using KeyPressCallback = std::function<void(const std::string&)>;
    using DrawFunction = std::function<void(SimpleGTKDrawspace*)>;

    void run(unsigned int in_sizeX, unsigned int in_sizeY, DrawFunction in_drawFunction, KeyPressCallback in_keyFunction = nullptr);

    std::string get_pressed_key();

    void square_brush(double in_x, double in_y, double in_size);
    void square_brush_filled(double in_x, double in_y, double in_size);
    void move_to(double in_x, double in_y);
    void line_to(double in_x, double in_y);
	void line(double in_x0, double in_y0, double in_x1, double in_y1);
	void circle(double in_x, double in_y, double in_radius);
	void arc(double in_x, double in_y, double in_radius, double in_beginAngle, double in_endAngle);
	void clear();
	void clear(double in_r, double in_g, double in_b);

    void set_font_size(double in_size);
    void print_text(double in_x, double in_y, const char* in_text);

    void set_color(double in_red, double in_green, double in_blue);
    void set_line_width(double in_width);
    void set_antialiasing(unsigned int in_grade);

    void save_to_png(const char* in_filename);

    void pause_rendering();
    void resume_rendering();
    void wait_for_render();

    SimpleGTKDrawspace(int* inout_argc = nullptr, char** inout_argv[] = nullptr);
	~SimpleGTKDrawspace();

private:
    void run_user_draw();
    static gboolean gtk_callback_key_event(GtkWidget* widget, GdkEventKey* event, gpointer in_this);
    static void gtk_callback_close_window(void* in_this);

    // GTK callbacks
    static gboolean gtk_callback_draw(GtkWidget *widget, cairo_t *cr, gpointer in_this);
    static gboolean gtk_callback_configure_event(GtkWidget *widget, GdkEventMotion *event, gpointer in_this);
    static gboolean gtk_callback_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer in_this);
    static void gtk_callback_start_button(GtkWidget *widget, gpointer in_this);
    static void gtk_callback_save_button(GtkWidget *widget, gpointer in_this);
    static gboolean gtk_callback_timer_redraw(gpointer in_this);

    std::unique_ptr<std::thread> m_draw_thread;
    pthread_t drawThread;
    GtkWidget *m_window = nullptr;
    GtkWidget *m_verticalBox = nullptr;
    GtkWidget *m_horizontalBox = nullptr;
    GtkWidget *m_frame = nullptr;
    GtkWidget *m_drawingArea = nullptr;

    GtkWidget *m_startButton = nullptr;
    GtkWidget *m_pauseButton = nullptr;
    GtkWidget *m_saveButton = nullptr;

    GtkWidget *m_savePictureDialog = nullptr;

    cairo_surface_t *m_drawSurface = nullptr;
    cairo_t *m_drawCairo = nullptr;
    std::mutex m_draw_surface_mutex;

    std::mutex m_wait_for_render_mutex;
    std::promise<void> m_renering_done;

    DrawFunction m_drawFunction;
    KeyPressCallback m_key_function;

    bool m_renderingIsPaused; // user called pauseRendering
    bool m_waitingForRedraw; // user called waitForRedraw

    bool m_drawedBetweenFrames; // Need we redraw at all?

    int* m_pargc;
    char*** m_pargv;

    double m_current_x, m_current_y;
    std::string m_pressed_key;
};

#endif
