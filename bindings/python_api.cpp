#include <pybind11/pybind11.h>
#include <manager.hpp>

PYBIND11_MODULE(p2p_uplug, m){
  m.def("ping", &ping);
}
