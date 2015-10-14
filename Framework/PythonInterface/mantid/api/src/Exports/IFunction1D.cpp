#include "MantidAPI/IFunction1D.h"
#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"
#include <boost/python/class.hpp>

using Mantid::API::IFunction1D;
using Mantid::API::IFunction;
using Mantid::PythonInterface::IFunction1DAdapter;
using Mantid::PythonInterface::IFunctionAdapter;
using namespace boost::python;

void export_IFunction1D() {
  /**
   * The Python held type, boost::shared_ptr<IFunction1DAdapter>, allows
   * the class' virtual functions to be overridden in Python
   */
  class_<IFunction1D, bases<IFunction>, boost::shared_ptr<IFunction1DAdapter>,
         boost::noncopyable>("IFunction1D", "Base class for 1D Fit functions")
      .def("function1D",
           (object (IFunction1DAdapter::*)(const object &) const) &
               IFunction1DAdapter::function1D,
           "Calculate the values of the function for the given x values and "
           "returns them");
}
