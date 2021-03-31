// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IWorkspaceProperty.h"
#include <boost/python/class.hpp>

void export_IWorkspaceProperty() {
  using namespace boost::python;
  using Mantid::API::IWorkspaceProperty;

  class_<IWorkspaceProperty, boost::noncopyable>("IWorkspaceProperty", no_init)
      .def("isOptional", &IWorkspaceProperty::isOptional, arg("self"), "Is the input workspace property optional")
      .def("isLocking", &IWorkspaceProperty::isLocking, arg("self"),
           "Will the workspace be locked when starting an algorithm");
}
