#include "MantidAPI/Axis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/ssize_t.hpp> //For Py_ssize_t. We can get rid of this when RHEL5 goes

// See http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>


using Mantid::API::Axis;
using Mantid::API::NumericAxis;
using Mantid::API::TextAxis;
using Mantid::API::SpectraAxis;
using Mantid::Kernel::Unit_sptr;
using Mantid::specid_t;
using namespace boost::python;

namespace
{
  namespace bpl = boost::python;

  //------------------------------- Overload macros ---------------------------
  // Overloads for operator() function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Axis_getValue, Axis::getValue, 1, 2);

  /**
   * Extract the axis values as a sequence. A numpy array is used if the
   * data is numerical or a simple python list is used if the data is a string type
   * @param self A reference to the object that called this method
   * @return A PyObject representing the array
   */
  PyObject * extractAxisValues(Axis &self)
  {
    const npy_intp nvalues = static_cast<npy_intp>(self.length());
    npy_intp arrayDims[1] = { nvalues };

    // Pick the correct element type base on the Axis type
    PyObject *array;
    bool numeric(true);
    if( self.isNumeric() || self.isSpectra() )
    {
      array = PyArray_SimpleNew(1, arrayDims, NPY_DOUBLE);
    }
    else if( self.isText() )
    {
      array = PyList_New((Py_ssize_t)nvalues);
      numeric = false;
    }
    else
    {
      throw std::invalid_argument("Unknown axis type. Cannot extract to Numpy");
    }

    // Fill the array
    for(npy_intp i = 0; i < nvalues; ++i)
    {
      if( numeric )
      {
        PyObject *value = PyFloat_FromDouble(self.getValue(static_cast<size_t>(i)));
        void *pos = PyArray_GETPTR1((PyArrayObject *)array, i);
        PyArray_SETITEM((PyArrayObject *)array, pos, value);
      }
      else
      {
        const std::string s = self.label(static_cast<size_t>(i));
        PyObject *value = PyString_FromString(s.c_str());
        PyList_SetItem(array, (Py_ssize_t)i, value);
      }
    }
    return array;
  }
}


void export_Axis()
{
  register_ptr_to_python<Axis*>();

  // Class
  class_< Axis, boost::noncopyable >("MantidAxis", no_init)
    .def("length", &Axis::length, "Returns the length of the axis")
    .def("title", (const std::string & (Axis::*)() const ) &Axis::title, return_value_policy<copy_const_reference>(),
         "Get the axis title")
    .def("isSpectra", &Axis::isSpectra, "Returns true if this is a SpectraAxis")
    .def("isNumeric", &Axis::isNumeric, "Returns true if this is a NumericAxis")
    .def("isText", &Axis::isText, "Returns true if this is a TextAxis")
    .def("label", &Axis::label, "Return the axis label")
    .def("getUnit", (const Unit_sptr & (Axis::*)() const) &Axis::unit, return_value_policy<copy_const_reference>(),
         "Returns the unit object for the axis")
    .def("getValue", &Axis::getValue,
        Axis_getValue(args("index", "vertical_index"), "Returns the value at the given point on the Axis. The vertical axis index [default=0]"))
    .def("extractValues", &extractAxisValues, "Return a numpy array of the axis values")
    .def("setUnit", & Axis::setUnit, "Set the unit for this axis")
    .def("setValue", &Axis::setValue, "Set a value at the given index")
    //------------------------------------ Special methods ------------------------------------
    .def("__len__", &Axis::length)
    ;
}


void export_NumericAxis()
{
  /// Exported so that Boost.Python can give back a NumericAxis class when an Axis* is returned
  class_< NumericAxis, bases<Axis>, boost::noncopyable >("NumericAxis", no_init)
   ;
}

void export_SpectraAxis()
{
  class_< SpectraAxis, bases<Axis>, boost::noncopyable >("SpectraAxis", no_init)
    .def("spectraNo", (const specid_t &(SpectraAxis::*)(const size_t &) const)&SpectraAxis::spectraNo,
          return_value_policy<copy_const_reference>(), "Returns the spectrum no at the given index")
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

