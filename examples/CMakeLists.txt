cmake_minimum_required(VERSION 3.7)

set (CMAKE_CXX_STANDARD 20)

project(CppExamples VERSION 1.0.0 LANGUAGES CXX)

add_compile_options(-DEIGEN_DONT_ALIGN_STATICALLY)

find_package(Boost 1.75.0 REQUIRED COMPONENTS system)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)


add_executable(CustomView CustomView.cpp)
target_link_libraries(CustomView Eigen3::Eigen)

add_executable(TypeDeduction TypeDeduction.cpp)

add_executable(CoroExample1 coroutine_examples/example1.cpp)
add_executable(CoroExample2 coroutine_examples/example2.cpp)
add_executable(CoroExample3 coroutine_examples/example3.cpp)
add_executable(CoroExample4 coroutine_examples/example4.cpp)
add_executable(CoroExample5 coroutine_examples/example5.cpp)
add_executable(CoroExample6 coroutine_examples/example6.cpp)
add_executable(CoroExample7 coroutine_examples/example7.cpp)
add_executable(CoroExample8 coroutine_examples/example8.cpp)
add_executable(CoroExample9 coroutine_examples/example9.cpp)
add_executable(CoroExampleBatch coroutine_examples/batch_coro.cpp)
