cmake_minimum_required(VERSION 3.7)

set (CMAKE_CXX_STANDARD 20)

project(CppExamples VERSION 1.0.0 LANGUAGES CXX)

add_compile_options(-DEIGEN_DONT_ALIGN_STATICALLY)

find_package(Boost 1.75.0 REQUIRED COMPONENTS system)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)


add_executable(Example1 example1.cpp)
target_link_libraries(Example1 Eigen3::Eigen)
