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
          return_value_policy<copy_const_reference>());
  /*
   Return value policy for next is to copy the const reference. Copy by value is
   essential for python 2.0 compatibility because items (SpectrumInfoItem) will
   outlive their iterators if declared as part of for loops. There is no good
   way to deal with this other than to force a copy so that internals of the
   item are not also corrupted. Future developers may wish to choose a separte
   policy for python 3.0 where this is not a concern, and const ref returns
   would be faster.
  */
}
