// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(MDFrame)

void export_MDFrame() {

  using namespace Mantid::Geometry;

  register_ptr_to_python<boost::shared_ptr<MDFrame>>();

  class_<MDFrame, boost::noncopyable>("MDFrame", no_init)
      .def("getUnitLabel", &MDFrame::getUnitLabel, arg("self"))
      .def("name", &MDFrame::name, arg("self"));
}
