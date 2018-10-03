// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IPeakFunction.h"
#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"
#include <boost/python/class.hpp>

using Mantid::API::IFunction1D;
using Mantid::API::IPeakFunction;
using Mantid::PythonInterface::IPeakFunctionAdapter;
using namespace boost::python;

void export_IPeakFunction() {
  class_<IPeakFunction, bases<IFunction1D>,
         boost::shared_ptr<IPeakFunctionAdapter>, boost::noncopyable>(
      "IPeakFunction", "Base class for peak Fit functions")
      .def("functionLocal",
           (object(IPeakFunctionAdapter::*)(const object &) const) &
               IPeakFunction::functionLocal,
           (arg("self"), arg("vec_x")),
           "Calculate the values of the function for the given x values. The "
           "output should be stored in the out array")
      .def("intensity", &IPeakFunction::intensity, arg("self"),
           "Returns the integral intensity of the peak function.")
      .def("setIntensity", &IPeakFunction::setIntensity,
           (arg("self"), arg("new_intensity")),
           "Changes the integral intensity of the peak function by setting its "
           "height.");
}
