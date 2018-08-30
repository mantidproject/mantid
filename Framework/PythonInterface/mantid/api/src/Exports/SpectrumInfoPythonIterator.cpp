#include "MantidPythonInterface/api/SpectrumInfoPythonIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>

using Mantid::PythonInterface::SpectrumInfoPythonIterator;
using namespace boost::python;

// Export SpectrumInfoPythonIterator
void export_SpectrumInfoPythonIterator() {

  // Export to Python
  class_<SpectrumInfoPythonIterator>("SpectrumInfoPythonIterator", no_init)
      .def("__iter__", objects::identity_function())
      .def(
#if PY_VERSION_HEX >= 0x03000000
          "__next__"
#else
          "next"
#endif
          ,
          &SpectrumInfoPythonIterator::next,
          return_value_policy<reference_existing_object>());
}
