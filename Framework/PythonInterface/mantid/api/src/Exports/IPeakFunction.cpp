// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IPeakFunction.h"
#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::IFunction1D;
using Mantid::API::IPeakFunction;
using Mantid::PythonInterface::IPeakFunctionAdapter;
using namespace boost::python;

void export_IPeakFunction() {

  register_ptr_to_python<std::shared_ptr<IPeakFunction>>();
  class_<IPeakFunction, bases<IFunction1D>, std::shared_ptr<IPeakFunctionAdapter>, boost::noncopyable>(
      "IPeakFunction", "Base class for peak Fit functions")
      // cppcheck-suppress cstyleCast - cppcheck complains but already using static_cast
      .def("functionLocal", static_cast<object(IPeakFunctionAdapter::*)(const object &) const>(&IPeakFunctionAdapter::functionLocal),
           (arg("self"), arg("vec_x")),
           "Calculate the values of the function for the given x values. The "
           "output should be stored in the out array")
      .def("fwhm", &IPeakFunction::fwhm, arg("self"), "Returns the fwhm of the peak function.")
      .def("setFwhm", &IPeakFunction::setFwhm, (arg("self"), arg("new_fwhm")), "Sets FWHM of peak.")
      .def("intensity", &IPeakFunction::intensity, arg("self"), "Returns the integral intensity of the peak function.")
      .def("intensityError", &IPeakFunction::intensityError, arg("self"),
           "Returns the integral intensity error of the peak function due to uncertainties in uncorrelated fit "
           "parameters.")
      .def("setIntensity", &IPeakFunction::setIntensity, (arg("self"), arg("new_intensity")),
           "Changes the integral intensity of the peak function by setting its "
           "height.")
      .def("setHeight", &IPeakFunction::setHeight, arg("self"), "Sets height of the peak function.")
      .def("setCentre", &IPeakFunction::setCentre, arg("self"), "Sets the centre of the peak function.")
      .def("getWidthParameterName", &IPeakFunction::getWidthParameterName, arg("self"),
           "Gets the parameter name that determines the peak width.")
      .def("getCentreParameterName", &IPeakFunction::getCentreParameterName, arg("self"),
           "Gets the parameter name that determiness the peak centre.");
}
