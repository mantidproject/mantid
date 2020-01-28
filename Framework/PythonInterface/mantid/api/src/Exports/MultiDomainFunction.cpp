// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MultiDomainFunction.h"
#include <boost/python/class.hpp>

using Mantid::API::CompositeFunction;
using Mantid::API::MultiDomainFunction;
using namespace boost::python;

void export_MultiDomainFunction() {
  class_<MultiDomainFunction, bases<CompositeFunction>, boost::noncopyable>(
      "MultiDomainFunction", "Multi-Domain Fit functions")
      .def("nFunctions", &MultiDomainFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("__len__", &MultiDomainFunction::nFunctions, arg("self"),
           "Get the number of member functions.")
      .def("getFunction", &MultiDomainFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("__getitem__", &MultiDomainFunction::getFunction,
           (arg("self"), arg("i")), "Get the i-th function.")
      .def("add", &MultiDomainFunction::addFunction,
           (arg("self"), arg("function")), "Add a member function.")
      .def("replaceFunction", &MultiDomainFunction::replaceFunction,
           (arg("self"), arg("function")), "Replace a member function.")
      .def("setDomainIndex", &MultiDomainFunction::setDomainIndex,
           (arg("self"), arg("funIndex"), arg("domainIndex")),
           "Associate a function and a domain.");
}
