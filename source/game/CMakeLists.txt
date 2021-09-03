project(game)

add_executable(${PROJECT_NAME} main.cpp engine.hpp engine.cpp)

target_link_libraries(${PROJECT_NAME} sgds)
