project(sgds)

find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_library(${PROJECT_NAME} simple-gtk-drawspace.h simple-gtk-drawspace.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} ${GTK3_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${GTK3_LIBRARIES} Threads::Threads)

