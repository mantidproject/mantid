#include "MantidPythonInterface/api/DetectorInfoPythonIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>

using Mantid::PythonInterface::DetectorInfoPythonIterator;
using namespace boost::python;

// Export DetectorInfoPythonIterator
void export_DetectorInfoPythonIterator() {

  // Export to Python
  class_<DetectorInfoPythonIterator>("DetectorInfoPythonIterator", no_init)
      .def("__iter__", objects::identity_function())
      .def(
#if PY_VERSION_HEX >= 0x03000000
          "__next__"
#else
          "next"
#endif
          ,
          &DetectorInfoPythonIterator::next,
          return_value_policy<reference_existing_object>());
}
