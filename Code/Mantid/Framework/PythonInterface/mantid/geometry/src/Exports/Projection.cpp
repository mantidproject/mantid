#include "MantidGeometry/Crystal/Projection.h"
#include <boost/bind.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_non_const_reference.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

void export_Projection()
{
  class_<Projection>("Projection", init<>("Default constructor creates a two dimensional projection"))
    .def(init<size_t>("Constructs an n-dimensional projection", args("num_dimensions")))
    .def(init<VMD,VMD>("Constructs a 2 dimensional projection", args("u","v")))
    .def(init<VMD,VMD,VMD>("Constructs a 3 dimensional projection", args("u","v","w")))
    .def(init<VMD,VMD,VMD,VMD>("Constructs a 4 dimensional projection", args("u","v","w","x")))
    .def(init<VMD,VMD,VMD,VMD,VMD>("Constructs a 5 dimensional projection", args("u","v","w","x","y")))
    .def(init<VMD,VMD,VMD,VMD,VMD,VMD>("Constructs a 6 dimensional projection", args("u","v","w","x","y","z")))
    .def("getNumDims", &Projection::getNumDims, "Returns the number of dimensions in the projection")
    .def("getOffset", &Projection::getOffset, "Returns the offset for the given dimension", args("dimension"))
    .def("getAxis", &Projection::getAxis, "Returns the axis for the given dimension", args("dimension"))
    .def("setOffset", &Projection::setOffset, "Sets the offset for the given dimension", args("dimension", "offset"))
    .def("setAxis", &Projection::setAxis, "Sets the axis for the given dimension", args("dimension", "axis"))
    .def("U", &Projection::U, "Returns a reference to the U axis", return_internal_reference<>())
    .def("V", &Projection::V, "Returns a reference to the V axis", return_internal_reference<>())
    .def("W", &Projection::W, "Returns a reference to the W axis", return_internal_reference<>())
    ;
}
