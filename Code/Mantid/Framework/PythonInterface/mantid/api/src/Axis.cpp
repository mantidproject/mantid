#include "MantidAPI/Axis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/copy_const_reference.hpp>

using Mantid::API::Axis;
using Mantid::API::NumericAxis;
using Mantid::API::TextAxis;
using Mantid::API::SpectraAxis;
using Mantid::Kernel::Unit_sptr;
using Mantid::specid_t;
using namespace boost::python;

void export_Axis()
{
  register_ptr_to_python<Axis*>();

  // Class
  class_< Axis, boost::noncopyable >("MantidAxis", no_init)
    .def("length", &Axis::length, "Returns the length of the axis")
    .def("title", (std::string & (Axis::*)() ) &Axis::title, return_internal_reference<>(),
         "Get/set the axis title. Can be used as ax.title = ")
    .def("isSpectra", &Axis::isSpectra, "Returns true if this is a SpectraAxis")
    .def("isNumeric", &Axis::isNumeric, "Returns true if this is a NumericAxis")
    .def("isText", &Axis::isText, "Returns true if this is a TextAxis")
    .def("label", &Axis::label, "Return the axis label")
    .def("getUnit", (const Unit_sptr & (Axis::*)() const) &Axis::unit, return_value_policy<copy_const_reference>(),
         "Returns the unit object for the axis")
    .def("setUnit", & Axis::setUnit, "Set the unit for this axis")
    ;
}

void export_NumericAxis()
{
  class_< NumericAxis, bases<Axis>, boost::noncopyable >("NumericAxis", no_init)
    .def("setValue", &NumericAxis::setValue, "Set a value at the given index")
    ;
}

void export_SpectraAxis()
{
  class_< SpectraAxis, bases<Axis>, boost::noncopyable >("SpectraAxis", no_init)
    .def("spectraNo", (const specid_t &(SpectraAxis::*)(const size_t &) const)&SpectraAxis::spectraNo,
          return_value_policy<copy_const_reference>(), "Returns the spectrum no at the given index")
    .def("setValue", & SpectraAxis::setValue, "Set a value at the given index")
    .def("populateOneToOne", & SpectraAxis::populateOneToOne, "Populate the list with 1:1 values")
    ;
}

void export_TextAxis()
{
  class_< TextAxis, bases<Axis>, boost::noncopyable >("TextAxis", no_init)
    .def("setLabel", & TextAxis::setLabel, "Set the label at the given entry")
    .def("label", & TextAxis::label, "Return the label at the given position")
    ;
}

