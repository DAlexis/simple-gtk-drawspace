#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <string>
#include <functional>

class MainLoop
{
public:
    using FrameFunction = std::function<void(double t, double dt)>;

    MainLoop(FrameFunction frame_func, double dt);

    void run();
    void stop();

private:
    FrameFunction m_frame_func;
    double m_wanted_dt;
    bool m_need_stop = false;

};

#endif
