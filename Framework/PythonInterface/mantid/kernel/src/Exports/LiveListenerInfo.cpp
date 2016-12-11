#include "MantidKernel/LiveListenerInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidPythonInterface/kernel/StlExportDefinitions.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::Kernel::LiveListenerInfo;
using namespace boost::python;

void export_LiveListenerInfo() {
  using namespace Mantid::PythonInterface;
  std_vector_exporter<LiveListenerInfo>::wrap("std_vector_LiveListenerInfo");

  class_<LiveListenerInfo>("LiveListenerInfo", no_init)
      .def("name", &LiveListenerInfo::name, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the name of this LiveListener connection")

      .def("address", &LiveListenerInfo::address, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the address of this LiveListener connection")

      .def("listener", &LiveListenerInfo::listener, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Returns the name of the specific LiveListener class used");
}
