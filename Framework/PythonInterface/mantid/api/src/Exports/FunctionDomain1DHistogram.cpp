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
using Mantid::API::FunctionDomain1DHistogram;
using namespace Mantid::PythonInterface;
using namespace Mantid::PythonInterface::Converters;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(FunctionDomain1Histogram)

FunctionDomain1DHistogram *createFunctionDomain1DHistogram(const boost::python::object &bins) {
  if (NDArray::check(bins)) {
    return new FunctionDomain1DHistogram(NDArrayToVector<double>(NDArray(bins))());
  } else {
    return new FunctionDomain1DHistogram(PySequenceToVector<double>(NDArray(bins))());
  }
}

void export_FunctionDomain1DHistogram() {

  class_<FunctionDomain1DHistogram, bases<FunctionDomain1D>, boost::noncopyable>("FunctionDomain1DHistogram", no_init)
      .def("__init__", make_constructor(&createFunctionDomain1DHistogram, default_call_policies(), (arg("bins"))));
}
