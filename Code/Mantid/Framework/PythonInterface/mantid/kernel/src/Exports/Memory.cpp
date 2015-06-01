#include "MantidKernel/Memory.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::MemoryStats;
using namespace boost::python;

void export_MemoryStats() {

  class_<MemoryStats>("MemoryStats", init<>("Construct MemoryStats object."))
      .def("update", &MemoryStats::update)
      .def("totalMem", &MemoryStats::totalMem)
      .def("availMem", &MemoryStats::availMem)
      .def("residentMem", &MemoryStats::residentMem)
      .def("virtualMem", &MemoryStats::virtualMem)
      .def("reservedMem", &MemoryStats::reservedMem)
      .def("getFreeRatio", &MemoryStats::getFreeRatio);
}
