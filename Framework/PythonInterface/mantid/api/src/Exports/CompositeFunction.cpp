// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CompositeFunction.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::CompositeFunction;
using Mantid::API::IFunction;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(CompositeFunction)

namespace {

using getParameterType1 = double (CompositeFunction::*)(size_t) const;
using getParameterType2 =
    double (CompositeFunction::*)(const std::string &) const;

using setParameterType2 = void (CompositeFunction::*)(const std::string &,
                                                      const double &, bool);
GNU_DIAG_OFF("unused-local-typedef")
GNU_DIAG_OFF("conversion")

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType2_Overloads,
                                       setParameter, 2, 3)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
} // namespace

void export_CompositeFunction() {

  register_ptr_to_python<boost::shared_ptr<CompositeFunction>>();

  class_<CompositeFunction, bases<IFunction>, boost::noncopyable>(
      "CompositeFunction", "Composite Fit functions")
      .def("nFunctions", &CompositeFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("__len__", &CompositeFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("getFunction", &CompositeFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("__getitem__", &CompositeFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("__setitem__", &CompositeFunction::replaceFunction,
           (arg("self"), arg("i"), arg("f")),
           "Put function in place of the i-th function.")
      .def("add", &CompositeFunction::addFunction,
           (arg("self"), arg("function")), "Add a member function.")
      .def("getParameterValue",
           (getParameterType1)&CompositeFunction::getParameter,
           (arg("self"), arg("i")), "Get value of parameter of given index.")
      .def("getParameterValue",
           (getParameterType2)&CompositeFunction::getParameter,
           (arg("self"), arg("name")), "Get value of parameter of given name.")
      .def("__getitem__", (getParameterType2)&CompositeFunction::getParameter,
           (arg("self"), arg("name")), "Get value of parameter of given name.")
      .def("__setitem__", (setParameterType2)&CompositeFunction::setParameter,
           setParameterType2_Overloads(
               (arg("self"), arg("name"), arg("value"), arg("explicitlySet")),
               "Get value of parameter of given name."))
      .def("__delitem__", &CompositeFunction::removeFunction,
           (arg("self"), arg("index")));
}
