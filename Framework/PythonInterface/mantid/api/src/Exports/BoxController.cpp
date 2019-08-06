// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/BoxController.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::BoxController;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(BoxController)

void export_BoxController() {
  register_ptr_to_python<boost::shared_ptr<BoxController>>();

  class_<BoxController, boost::noncopyable>("BoxController", no_init)
      .def("getNDims", &BoxController::getNDims, arg("self"),
           "Get # of dimensions")
      .def("getSplitThreshold", &BoxController::getSplitThreshold, arg("self"),
           "Return the splitting threshold, in # of events")
      .def("getSplitInto", &BoxController::getSplitInto,
           (arg("self"), arg("dim")),
           "Return into how many to split along a dimension")
      .def("getMaxDepth", &BoxController::getMaxDepth, arg("self"),
           "Return the max recursion depth allowed for grid box splitting.")
      .def("getTotalNumMDBoxes", &BoxController::getTotalNumMDBoxes,
           arg("self"),
           "Return the total number of MD Boxes, irrespective of depth")
      .def("getTotalNumMDGridBoxes", &BoxController::getTotalNumMDGridBoxes,
           arg("self"),
           "Return the total number of MDGridBox'es, irrespective of depth")
      .def("getAverageDepth", &BoxController::getAverageDepth, arg("self"),
           "Return the average recursion depth of gridding.")
      .def("isFileBacked", &BoxController::isFileBacked, arg("self"),
           "Return True if the MDEventWorkspace is backed by a file ")
      .def("getFilename", &BoxController::getFilename, arg("self"),
           "Return  the full path to the file open as the file-based back or "
           "empty string if no file back-end is initiated")
      .def("useWriteBuffer", &BoxController::useWriteBuffer, arg("self"),
           "Return true if the MRU should be used");
}
