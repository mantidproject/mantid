// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Axis.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/ssize_t.hpp> //For Py_ssize_t. We can get rid of this when RHEL5 goes

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using namespace Mantid::API;
using Mantid::specnum_t;
using Mantid::Kernel::Unit_sptr;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Axis)

namespace {

//------------------------------- Overload macros ---------------------------
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

// Overloads for operator() function which has 1 optional argument
// cppcheck-suppress unknownMacro
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Axis_getValue, Axis::getValue, 1, 2)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
/**
 * Extract the axis values as a sequence. A numpy array is used if the
 * data is numerical or a simple python list is used if the data is a string
 * type
 * @param self A reference to the object that called this method
 * @return A PyObject representing the array
 */
PyObject *extractAxisValues(Axis &self) {
  const auto nvalues = static_cast<npy_intp>(self.length());
  npy_intp arrayDims[1] = {nvalues};

  // Pick the correct element type base on the Axis type
  PyObject *array;
  bool numeric(true);
  if (self.isNumeric() || self.isSpectra()) {
    array = PyArray_SimpleNew(1, arrayDims, NPY_DOUBLE);
  } else if (self.isText()) {
    array = PyList_New(static_cast<Py_ssize_t>(nvalues));
    numeric = false;
  } else {
    throw std::invalid_argument("Unknown axis type. Cannot extract to Numpy");
  }

  // Fill the array
  for (npy_intp i = 0; i < nvalues; ++i) {
    if (numeric) {
      PyObject *value = PyFloat_FromDouble(self.getValue(static_cast<size_t>(i)));
      void *pos = PyArray_GETPTR1((PyArrayObject *)array, i);
      PyArray_SETITEM(reinterpret_cast<PyArrayObject *>(array), reinterpret_cast<char *>(pos), value);
    } else {
      const std::string s = self.label(static_cast<size_t>(i));
      PyObject *value = to_python_value<const std::string &>()(s);
      PyList_SetItem(array, (Py_ssize_t)i, value);
    }
  }
  return array;
}
} // namespace

void export_Axis() {
  register_ptr_to_python<Axis *>();

  // Class
  class_<Axis, boost::noncopyable>("MantidAxis", no_init)
      .def("length", &Axis::length, arg("self"), "Returns the length of the axis")
      .def("title", (const std::string &(Axis::*)() const) & Axis::title, arg("self"),
           return_value_policy<copy_const_reference>(), "Get the axis title")
      .def("isSpectra", &Axis::isSpectra, arg("self"), "Returns true if this is a SpectraAxis")
      .def("isNumeric", &Axis::isNumeric, arg("self"), "Returns true if this is a NumericAxis")
      .def("isText", &Axis::isText, arg("self"), "Returns true if this is a TextAxis")
      .def("label", &Axis::label, (arg("self"), arg("index")), "Return the axis label")
      .def("getUnit", (const Unit_sptr &(Axis::*)() const) & Axis::unit, arg("self"),
           return_value_policy<copy_const_reference>(), "Returns the unit object for the axis")
      .def("getValue", &Axis::getValue,
           Axis_getValue((arg("self"), arg("index"), arg("vertical_index")),
                         "Returns the value at the given point on the Axis. "
                         "The vertical axis index [default=0]"))
      .def("indexOfValue", &Axis::indexOfValue,
           ((arg("self"), arg("value")), return_value_policy<copy_const_reference>(),
            "Returns the index of the given value on the Axis. "))
      .def("extractValues", &extractAxisValues, arg("self"), "Return a numpy array of the axis values")
      .def("indexOfValue", &Axis::indexOfValue, (arg("value")),
           "Returns the index of the closest to the given value on the axis")
      .def("setUnit", &Axis::setUnit, (arg("self"), arg("unit_name")), return_value_policy<copy_const_reference>(),
           "Set the unit for this axis by name.")
      .def("setValue", &Axis::setValue, (arg("self"), arg("index"), arg("value")), "Set a value at the given index")
      .def("getMin", &Axis::getMin, arg("self"), "Get min value specified on the axis")
      .def("getMax", &Axis::getMax, arg("self"), "Get max value specified on the axis")
      //------------------------------------ Special methods
      //------------------------------------
      .def("__len__", &Axis::length, arg("self"));
}

// --------------------------------------------------------------------------------------------
// SpectraAxis
// --------------------------------------------------------------------------------------------
/**
 * Creates a SpectraAxis referencing a given workspace
 * @param ws A pointer to the parent workspace
 * @return pointer to the axis object
 */
Axis *createSpectraAxis(const MatrixWorkspace *const ws) { return new SpectraAxis(ws); }

void export_SpectraAxis() {
  /// Exported so that Boost.Python can give back a SpectraAxis class when an
  /// Axis* is returned
  class_<SpectraAxis, bases<Axis>, boost::noncopyable>("SpectraAxis", no_init)
      .def("create", &createSpectraAxis, arg("workspace"), return_internal_reference<>(),
           "Creates a new SpectraAxis referencing the given workspace")
      .staticmethod("create");
}

// --------------------------------------------------------------------------------------------
// NumericAxis
// --------------------------------------------------------------------------------------------
/**
 * Creates a NumericAxis
 * @param length The length of the new axis
 * @return pointer to the axis object
 */
Axis *createNumericAxis(int length) { return new NumericAxis(length); }

void export_NumericAxis() {
  /// Exported so that Boost.Python can give back a NumericAxis class when an
  /// Axis* is returned
  class_<NumericAxis, bases<Axis>, boost::noncopyable>("NumericAxis", no_init)
      .def("create", &createNumericAxis, arg("length"), return_internal_reference<>(),
           "Creates a new NumericAxis of a specified length")
      .staticmethod("create");
}

// --------------------------------------------------------------------------------------------
// BinEdgeAxis
// --------------------------------------------------------------------------------------------

/**
 * Creates a BinEdgeAxis
 * @param length The length of the new axis
 * @return pointer to the axis object
 */
Axis *createBinEdgeAxis(int length) { return new BinEdgeAxis(length); }

void export_BinEdgeAxis() {
  /// Exported so that Boost.Python can give back a BinEdgeAxis class when an
  /// Axis* is returned
  class_<BinEdgeAxis, bases<NumericAxis>, boost::noncopyable>("BinEdgeAxis", no_init)
      .def("create", &createBinEdgeAxis, arg("length"), return_internal_reference<>(),
           "Creates a new BinEdgeAxis of a specified length")
      .staticmethod("create");
}

// --------------------------------------------------------------------------------------------
// TextAxis
// --------------------------------------------------------------------------------------------

/**
 * Creates a TextAxis
 * @param length The length of the new axis
 * @return pointer to the axis object
 */
Axis *createTextAxis(int length) { return new TextAxis(length); }

void export_TextAxis() {
  class_<TextAxis, bases<Axis>, boost::noncopyable>("TextAxis", no_init)
      .def("setLabel", &TextAxis::setLabel, (arg("self"), arg("index"), arg("label")),
           "Set the label at the given entry")
      .def("label", &TextAxis::label, (arg("self"), arg("index")), "Return the label at the given position")
      .def("create", &createTextAxis, arg("length"), return_internal_reference<>(),
           "Creates a new TextAxis of a specified length")
      .staticmethod("create");
}
