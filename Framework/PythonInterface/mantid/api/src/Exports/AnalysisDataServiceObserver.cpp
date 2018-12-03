// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_AnalysisDataServiceObserver() {

  boost::python::register_ptr_to_python<
      boost::shared_ptr<AnalysisDataServiceObserver>>();

  boost::python::class_<AnalysisDataServiceObserver, bases<>,
         boost::shared_ptr<AnalysisDataServiceObserver>, boost::noncopyable>(
      "AnalysisDataServiceObserver",
      "Observes AnalysisDataService notifications: all only")
      .def("observeAll", &AnalysisDataServiceObserver::observeAll, (arg("self"), arg("on")),
           "Observe AnalysisDataService for any changes")
      .def("anyChangeHandle", &AnalysisDataServiceObserver::anyChangeHandle, arg("self"),
           "Override this to change the effect of what happens when a change occurs in the ADS");
}