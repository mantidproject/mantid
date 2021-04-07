// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidPythonInterface/api/AnalysisDataServiceObserverAdapter.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

namespace {
void observeAll(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeAll(turnOn);
}

void observeRename(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeRename(turnOn);
}

void observeAdd(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeAdd(turnOn);
}

void observeReplace(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeReplace(turnOn);
}

void observeDelete(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeDelete(turnOn);
}

void observeClear(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeClear(turnOn);
}

void observeGroup(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeGroup(turnOn);
}

void observeUnGroup(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeUnGroup(turnOn);
}

void observeGroupUpdate(AnalysisDataServiceObserver &self, bool turnOn) {
  ReleaseGlobalInterpreterLock releaseGil;
  self.observeGroupUpdate(turnOn);
}
} // namespace

void export_AnalysisDataServiceObserver() {
  boost::python::class_<AnalysisDataServiceObserver, bases<>, AnalysisDataServiceObserverAdapter, boost::noncopyable>(
      "AnalysisDataServiceObserver", "Observes AnalysisDataService notifications: all only")
      .def("observeAll", &observeAll, (arg("self"), arg("on")), "Observe AnalysisDataService for any changes")
      .def("observeAdd", &observeAdd, (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being added")
      .def("observeReplace", &observeReplace, (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being replaced")
      .def("observeDelete", &observeDelete, (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being deleted")
      .def("observeClear", &observeClear, (arg("self"), arg("on")), "Observe AnalysisDataService for it being cleared")
      .def("observeRename", &observeRename, (arg("self"), arg("on")),
           "Observe AnalysisDataService for a workspace being renamed")
      .def("observeGroup", &observeGroup, (arg("self"), arg("on")),
           "Observe AnalysisDataService for a group being added/made in the ADS")
      .def("observeUnGroup", &observeUnGroup, (arg("self"), arg("on")),
           "Observe AnalysisDataService for a group being removed from the ADS")
      .def("observeGroupUpdate", &observeGroupUpdate, (arg("self"), arg("on")),
           "Observe AnalysisDataService for a group being updated by being "
           "added to or removed from");
}