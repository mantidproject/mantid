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

#include <boost/mpl/vector.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/make_function.hpp>
#include <functional>

using namespace std::placeholders;
using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;
using ObserverMethod = void (AnalysisDataServiceObserver::*)(bool);
using ObserverMethodSignature = boost::mpl::vector<void, AnalysisDataServiceObserver &, bool>;

namespace {

/**
 * @brief Given an observer method, such as observeAll, callReleasingGIL will ensure the GIL
 * has been released before calling the observer method. This is to prevent a deadlock that
 * can occur if an observe method is called at the same time python is being executed for
 * an ADS observer handle, such as replaceHandle.
 *
 * @param self AnalysisDataServiceObserver; the ADS observer object to call the observe method on
 * @param on bool; whether to turn on or off the observer for the specific method
 * @param method ObserverMethod; the method to call with the on parameter.
 */
void callReleasingGIL(AnalysisDataServiceObserver &self, bool on, ObserverMethod method) {
  ReleaseGlobalInterpreterLock releaseGil;
  (self.*method)(on);
}
} // namespace

void export_AnalysisDataServiceObserver() {
  boost::python::class_<AnalysisDataServiceObserver, bases<>, AnalysisDataServiceObserverAdapter, boost::noncopyable>(
      "AnalysisDataServiceObserver", "Observes AnalysisDataService notifications: all only")
      .def("observeAll",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeAll),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for any changes")
      .def("observeAdd",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeAdd),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for a workspace being added")
      .def("observeReplace",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeReplace),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for a workspace being replaced")
      .def("observeDelete",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeDelete),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for a workspace being deleted")
      .def("observeClear",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeClear),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for it being cleared")
      .def("observeRename",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeRename),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for a workspace being renamed")
      .def("observeGroup",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeGroup),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for a group being added/made in the ADS")
      .def("observeUnGroup",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeUnGroup),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for a group being removed from the ADS")
      .def("observeGroupUpdate",
           make_function(std::bind(callReleasingGIL, _1, _2, &AnalysisDataServiceObserver::observeGroupUpdate),
                         default_call_policies(), (arg("self"), arg("on")), ObserverMethodSignature()),
           "Observe AnalysisDataService for a group being updated by being "
           "added to or removed from");
}
