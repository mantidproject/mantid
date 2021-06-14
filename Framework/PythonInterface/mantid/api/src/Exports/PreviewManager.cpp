// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/PreviewManager.h"

#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::API::PreviewManager;
using Mantid::API::PreviewManagerImpl;
using namespace boost::python;

namespace {
PreviewManagerImpl &instance() { return PreviewManager::Instance(); }
} // namespace

void export_PreviewManager() {
  class_<PreviewManagerImpl, boost::noncopyable>("PreviewManager", no_init)

      .def("getPreview", &PreviewManagerImpl::getPreview,
           (arg("self"), arg("facility"), arg("technique"), arg("acquisition"), arg("name")),
           "Get the preview by the facility, technique, acquisition mode "
           "and name.",
           return_value_policy<reference_existing_object>())
      .def("getPreviews", &PreviewManagerImpl::getPreviews,
           (arg("self"), arg("facility"), arg("technique") = "", arg("acquisition") = ""),
           "Get the names of the previews available for the facility, "
           "technique and acquisition mode.")
      .def("Instance", instance, "Return a reference to the singleton instance",
           return_value_policy<reference_existing_object>())
      .staticmethod("Instance");
}
