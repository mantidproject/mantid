#include "MantidAPI/IPeakFunction.h"
#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"
#include <boost/python/class.hpp>

using Mantid::API::IFunction1D;
using Mantid::API::IPeakFunction;
using Mantid::PythonInterface::IPeakFunctionAdapter;
using namespace boost::python;

void export_IPeakFunction()
{
  class_<IPeakFunction, bases<IFunction1D>, boost::shared_ptr<IPeakFunctionAdapter>,
         boost::noncopyable>("IPeakFunction")
   .def("functionLocal", (object (IPeakFunctionAdapter::*)(const object &)const)&IPeakFunction::functionLocal,
        "Calculate the values of the function for the given x values. The output should be stored in the out array")
    ;
}

