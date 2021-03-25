// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/IPreview.h"
#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/register_ptr_to_python.hpp>

GET_POINTER_SPECIALIZATION(IPreview)

using namespace boost::python;
using Mantid::API::IPreview;
using Mantid::API::Workspace_sptr;

namespace {
Workspace_sptr (IPreview::*view)(Workspace_sptr) const = &IPreview::view;
} // namespace

void export_IPreview() {

  register_ptr_to_python<IPreview *>();

  enum_<IPreview::PreviewType>("PreviewType")
      .value("IVIEW", IPreview::PreviewType::IVIEW)
      .value("PLOT1D", IPreview::PreviewType::PLOT1D)
      .value("PLOT2D", IPreview::PreviewType::PLOT2D)
      .value("SVIEW", IPreview::PreviewType::SVIEW)
      .export_values();

  class_<IPreview, boost::noncopyable>("IPreview", no_init)
      .def("name", &IPreview::name, arg("self"), "Get the name of the preview.")
      .def("facility", &IPreview::facility, arg("self"),
           "Get the facility of the preview")
      .def("technique", &IPreview::technique, arg("self"),
           "Get the technique of the preview.")
      .def("type", &IPreview::type, arg("self"), "Get the type of the preview.")
      .def("acquisition", &IPreview::acquisition, arg("self"),
           "Get the acquisition mode of the preview")
      .def("view", view, (arg("self"), arg("ws")),
           "Perform the preview operation on the workspace.");
}
