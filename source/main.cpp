#include "simple-gtk-drawspace.h"

#include <math.h>

void draw(SimpleGTKDrawspace* window)
{
	g_print("- Drawing begin\n");
	
	window->setColor(0, 0, 1);
	g_print("- 1 test brushes\n");
	window->squareBrush(50, 50, 10);
	g_print("- 2 test brushes\n");
	window->squareBrush(200, 50, 10);
	
	g_print("- Test spiral\n");
	window->setColor(0, 1, 0);
	window->moveTo(250, 250);
	for (double alpha = 0; alpha < M_PI * 50; alpha += 0.3)
	{
		double x = 250 + 1 * cos(alpha) * alpha, y = 250 - 1 * sin(alpha) * alpha;
		window->lineTo(x, y);
		window->squareBrush(x, y, 1.0 + alpha*0.1);
		usleep(20000);
	}
	
	window->setColor(0.2, 0.4, 0.8);
	window->line(80, 80, 80, 100);
	
	g_print("- Drawing end\n");
}

int main(int argc, char* argv[])
{
	SimpleGTKDrawspace window(&argc, &argv);
	window.init(500, 500, draw);
	return 0;
	
}
