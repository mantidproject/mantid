// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IFunction1D.h"
#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"
#include <boost/python/class.hpp>

using Mantid::API::IFunction;
using Mantid::API::IFunction1D;
using Mantid::PythonInterface::IFunction1DAdapter;
using namespace boost::python;

void export_IFunction1D() {
  /**
   * The Python held type, boost::shared_ptr<IFunction1DAdapter>, allows
   * the class' virtual functions to be overridden in Python
   */
  class_<IFunction1D, bases<IFunction>, boost::shared_ptr<IFunction1DAdapter>,
         boost::noncopyable>("IFunction1D", "Base class for 1D Fit functions")
      .def("function1D",
           (object(IFunction1DAdapter::*)(const object &) const) &
               IFunction1DAdapter::function1D,
           (arg("self"), arg("xvals")),
           "Calculate the values of the function for the given x values and "
           "returns them");
}
