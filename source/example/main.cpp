#include "simple-gtk-drawspace.h"

#include <math.h>

void draw(SimpleGTKDrawspace* window)
{
	g_print("- Drawing begin\n");
	
	window->setColor(0, 0, 1);
	g_print("- Test brush\n");
	window->squareBrush(50, 50, 10);
	
	g_print("- Test filled brushes\n");
	window->squareBrushFilled(200, 50, 10);
	
	g_print("- Test circle and arc\n");
	window->circle(50, 200, 10);
	window->arc(50, 50, 50, -M_PI_2, 0);
	
	g_print("- Test text\n");
	window->printText(220, 450, "Hello!");
	
	g_print("- Test spiral\n");
	window->setColor(0.1, 0.5, 0);
	window->moveTo(250, 250);

	g_print("- Waiting for render...\n");
    window->waitForRender();

	g_print("- Pausing render\n");
	window->pauseRendering();
	
	for (double alpha = 0; alpha < M_PI * 50; alpha += 0.3)
	{
		double x = 250 + 1 * cos(alpha) * alpha, y = 250 - 1 * sin(alpha) * alpha;
		window->lineTo(x, y);
		window->squareBrush(x, y, 1.0 + alpha*0.1);
		usleep(5000);
	}
	g_print("- Resuming render\n");
	window->resumeRendering();
	
	g_print("- Animation\n");
    /*
	for (int i = 0; i <= 10; i++)
	{
		window->setAntialiasing(1);
		for (double x = 5; x < 495; x++)
		{
			window->setLineWidth(2);
			window->setColor(1, 0, 0.1);
			window->squareBrush(x, 480, 10+5*sin(x/490*2*M_PI));
			usleep(15000);
			window->setLineWidth(4);
			window->setColor(1, 1, 1);
			window->squareBrush(x, 480, 10+5*sin(x/490*2*M_PI));
		}
		window->setAntialiasing(0);
		for (double x = 495; x > 5; x--)
		{
			window->setLineWidth(2);
			window->setColor(1, 0, 0.1);
			window->squareBrush(x, 480, 10+5*sin(x/490*2*M_PI));
			usleep(15000);
			window->setLineWidth(2);
			window->setColor(1, 1, 1);
			window->squareBrush(x, 480, 10+5*sin(x/490*2*M_PI));
		}
	}
    */
	g_print("- Drawing end\n");
}

int main(int argc, char* argv[])
{
	SimpleGTKDrawspace window(&argc, &argv);
	window.init(500, 500, draw, nullptr);
	return 0;
	
}
