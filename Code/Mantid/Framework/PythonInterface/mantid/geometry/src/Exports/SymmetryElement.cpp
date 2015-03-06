#include "MantidGeometry/Crystal/SymmetryElement.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace {
    Mantid::Kernel::V3D getAxis(SymmetryElement & self)
    {
        try {
            SymmetryElementWithAxis &axisElement = dynamic_cast<SymmetryElementWithAxis &>(self);
            return Mantid::Kernel::V3D(axisElement.getAxis());
        } catch(std::bad_cast) {
            return Mantid::Kernel::V3D(0, 0, 0);
        }
    }
}

void export_SymmetryElement()
{
  register_ptr_to_python<boost::shared_ptr<SymmetryElement> >();  
  scope symmetryElementScope = class_<SymmetryElement, boost::noncopyable>("SymmetryElement", no_init);
  class_<SymmetryElement, boost::noncopyable>("SymmetryElement", no_init)
      .def("hmSymbol", &SymmetryElement::hmSymbol)
      .def("getAxis", &getAxis);
}
