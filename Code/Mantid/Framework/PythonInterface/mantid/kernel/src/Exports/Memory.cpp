#include "MantidKernel/Memory.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::MemoryStats;
using namespace boost::python;

void export_MemoryStats() {

  class_<MemoryStats>("MemoryStats", init<>("Construct MemoryStats object."))
      .def("update", &MemoryStats::update, args("self"))
      .def("totalMem", &MemoryStats::totalMem, args("self"))
      .def("availMem", &MemoryStats::availMem, args("self"))
      .def("residentMem", &MemoryStats::residentMem, args("self"))
      .def("virtualMem", &MemoryStats::virtualMem, args("self"))
      .def("reservedMem", &MemoryStats::reservedMem, args("self"))
      .def("getFreeRatio", &MemoryStats::getFreeRatio, args("self"))
      .def("getCurrentRSS", &MemoryStats::getCurrentRSS, args("self"))
      .def("getPeakRSS", &MemoryStats::getPeakRSS, args("self"));
}
