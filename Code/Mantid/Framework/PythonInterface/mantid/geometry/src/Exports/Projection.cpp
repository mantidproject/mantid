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
    .def("getUnit", &Projection::getUnit, "Returns the unit for the given dimension", args("dimension"))
    .def("setOffset", &Projection::setOffset, "Sets the offset for the given dimension", args("dimension", "offset"))
    .def("setAxis", &Projection::setAxis, "Sets the axis for the given dimension", args("dimension", "axis"))
    .def("setUnit", &Projection::setUnit, "Sets the unit for the given dimension", args("dimension", "unit"))
    .add_property("u",
        make_function(&Projection::U, return_internal_reference<>(), boost::mpl::vector2<VMD&, Projection&>()),
        make_function(boost::bind(&Projection::setAxis, _1, 0, _2), default_call_policies(), boost::mpl::vector3<void, Projection&, VMD>())
    )
    .add_property("v",
        make_function(&Projection::V, return_internal_reference<>(), boost::mpl::vector2<VMD&, Projection&>()),
        make_function(boost::bind(&Projection::setAxis, _1, 1, _2), default_call_policies(), boost::mpl::vector3<void, Projection&, VMD>())
    )
    .add_property("w",
        make_function(&Projection::W, return_internal_reference<>(), boost::mpl::vector2<VMD&, Projection&>()),
        make_function(boost::bind(&Projection::setAxis, _1, 2, _2), default_call_policies(), boost::mpl::vector3<void, Projection&, VMD>())
    )
    ;
}
