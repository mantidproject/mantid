// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FunctionDomain1D.h"

#include "MantidPythonInterface/core/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/module.hpp>

using Mantid::API::FunctionDomain1D;
using Mantid::API::FunctionDomain1DVector;
using namespace Mantid::PythonInterface;
using namespace Mantid::PythonInterface::Converters;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(FunctionDomain1DVector)

FunctionDomain1DVector *createFunctionDomain1DVector(const boost::python::object &xvalues) {
  if (NDArray::check(xvalues)) {
    return new FunctionDomain1DVector(NDArrayToVector<double>(NDArray(xvalues))());
  } else {
    return new FunctionDomain1DVector(PySequenceToVector<double>(NDArray(xvalues))());
  }
}

void export_FunctionDomain1DVector() {

  class_<FunctionDomain1DVector, bases<FunctionDomain1D>, boost::noncopyable>("FunctionDomain1DVector", no_init)
      .def("__init__", make_constructor(&createFunctionDomain1DVector, default_call_policies(), (arg("xvalues"))));
}
