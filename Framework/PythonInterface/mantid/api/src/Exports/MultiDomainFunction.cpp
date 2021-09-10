// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::CompositeFunction;
using Mantid::API::MultiDomainFunction;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(MultiDomainFunction)

void export_MultiDomainFunction() {

  register_ptr_to_python<std::shared_ptr<MultiDomainFunction>>();

  class_<MultiDomainFunction, bases<CompositeFunction>, boost::noncopyable>("MultiDomainFunction",
                                                                            "Multi-Domain Fit functions")
      .def("nFunctions", &MultiDomainFunction::nFunctions, arg("self"), "Get the number of member functions.")
      .def("__len__", &MultiDomainFunction::nFunctions, arg("self"), "Get the number of member functions.")
      .def("getFunction", &MultiDomainFunction::getFunction, (arg("self"), arg("i")), "Get the i-th function.")
      .def("__getitem__", &MultiDomainFunction::getFunction, (arg("self"), arg("i")), "Get the i-th function.")
      .def("add", &MultiDomainFunction::addFunction, (arg("self"), arg("function")), "Add a member function.")
      .def("setDomainIndex", &MultiDomainFunction::setDomainIndex, (arg("self"), arg("funIndex"), arg("domainIndex")),
           "Associate a function and a domain.");
}
