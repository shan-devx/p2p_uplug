cmake_minimum_required(VERSION 3.14)
project(p2p_uplug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(p2p_uplug
  src/payload.cpp
  src/transport.cpp
  bindings/python_api.cpp
)

target_include_directories(p2p_uplug PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  Boost:boost
)
