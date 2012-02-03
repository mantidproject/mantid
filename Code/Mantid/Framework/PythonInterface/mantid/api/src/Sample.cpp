#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::Sample;
using Mantid::Geometry::OrientedLattice;
using namespace boost::python;

void export_Sample()
{
  register_ptr_to_python<Sample*>();
  register_ptr_to_python<boost::shared_ptr<Sample> >();


  class_< Sample, boost::noncopyable >("Sample", no_init)
    .def("getName", &Sample::getName, return_value_policy<copy_const_reference>(), "Returns the string name of the sample")
    .def("getOrientedLattice", (const OrientedLattice & (Sample::*)() const)&Sample::getOrientedLattice,
          return_value_policy<reference_existing_object>(), "Get the oriented lattice for this sample")
    .def("hasOrientedLattice", &Sample::hasOrientedLattice, "Returns True if this sample has an oriented lattice, false otherwise")
    .def("size", &Sample::size, "Return the number of samples contained within this sample")
     // Required for ISIS SANS reduction until the full sample geometry is defined on loading
    .def("getGeometryFlag", &Sample::getGeometryFlag, "Return the geometry flag.")
    .def("getThickness", &Sample::getThickness, "Return the thickness in mm")
    .def("getHeight", &Sample::getHeight, "Return the height in mm")
    .def("getWidth", &Sample::getWidth, "Return the width in mm")
    // -------------------------Operators -------------------------------------
    .def("__len__", &Sample::size)
    .def("__getitem__", &Sample::operator[], return_internal_reference<>())
   ;
}

