// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidPythonInterface/api/AnalysisDataServiceObserverAdapter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

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
           "Observe AnalysisDataService for any changes");
}