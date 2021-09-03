#include "engine.hpp"
#include <chrono>
#include <thread>

MainLoop::MainLoop(FrameFunction frame_func, double dt) :
    m_frame_func(frame_func), m_wanted_dt(dt)
{
}

void MainLoop::run()
{
    using namespace std::chrono;
    steady_clock::time_point start_time_point = steady_clock::now();
    steady_clock::time_point last_time_point = start_time_point;

    while (!m_need_stop)
    {
        steady_clock::time_point current_time_point = steady_clock::now();
        double t = duration_cast<duration<double>>(current_time_point - start_time_point).count();
        double dt = duration_cast<duration<double>>(current_time_point - last_time_point).count();
        last_time_point = current_time_point;
        m_frame_func(t, dt);
        double actual_frame_time = duration_cast<duration<double>>(steady_clock::now() - last_time_point).count();
        if (actual_frame_time < m_wanted_dt)
        {
            std::this_thread::sleep_for(duration<double>(m_wanted_dt - actual_frame_time));
        }
    }
}

//void MainLoop::on_key_pressed(const std::string& key)
//{
//}
