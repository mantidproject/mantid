#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/def.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>
#include <boost/python/reference_existing_object.hpp>

#include "MantidGeometry/Instrument/DetectorInfoPythonIterator.h"

using namespace boost::python;
using Mantid::Geometry::DetectorInfoPythonIterator;

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
      return_value_policy<copy_const_reference>());
   
  //def("detectorInfo", detectorInfo, return_value_policy<reference_existing_object>());
}