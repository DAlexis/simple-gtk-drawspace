#include "simple-gtk-drawspace.h"

#include <math.h>

void loop(SimpleGTKDrawspace* window)
{
    window->pause_rendering();
    static double x = 100;
    static double y = 100;
    window->resume_rendering();
}

int main(int argc, char* argv[])
{
	SimpleGTKDrawspace window(&argc, &argv);
    window.run(500, 500, draw, nullptr);
	return 0;
	
}
