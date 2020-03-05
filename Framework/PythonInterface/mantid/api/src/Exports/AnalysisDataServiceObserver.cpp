// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidPythonInterface/api/AnalysisDataServiceObserverAdapter.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

void export_AnalysisDataServiceObserver() {
  boost::python::class_<AnalysisDataServiceObserver, bases<>,
                        AnalysisDataServiceObserverAdapter, boost::noncopyable>(
      "AnalysisDataServiceObserver",
      "Observes AnalysisDataService notifications: all only")
      .def("observeAll", &AnalysisDataServiceObserverAdapter::observeAll,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for any changes")
      .def("observeAdd", &AnalysisDataServiceObserverAdapter::observeAdd,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being added")
      .def("observeReplace",
           &AnalysisDataServiceObserverAdapter::observeReplace,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being replaced")
      .def("observeDelete", &AnalysisDataServiceObserverAdapter::observeDelete,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being deleted")
      .def("observeClear", &AnalysisDataServiceObserverAdapter::observeClear,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for it being cleared")
      .def("observeRename", &AnalysisDataServiceObserverAdapter::observeRename,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being renamed")
      .def(
          "observeGroup", &AnalysisDataServiceObserverAdapter::observeGroup,
          (arg("self"), arg("on")),
          "Observe AnalysisDataService for a group being added/made in the ADS")
      .def("observeUnGroup",
           &AnalysisDataServiceObserverAdapter::observeUnGroup,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for a group being removed from the ADS")
      .def("observeGroupUpdate",
           &AnalysisDataServiceObserverAdapter::observeGroupUpdate,
           (arg("self"), arg("on")),
           "Observe AnalysisDataService for a group being updated by being "
           "added to or removed from");
}