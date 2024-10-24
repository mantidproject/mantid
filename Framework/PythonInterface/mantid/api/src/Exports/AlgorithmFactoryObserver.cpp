// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmFactoryObserver.h"
#include "MantidPythonInterface/api/Algorithms/AlgorithmFactoryObserverAdapter.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>

using namespace Mantid::API;
using namespace Mantid::PythonInterface;
using namespace boost::python;

void export_AlgorithmFactoryObserver() {
  boost::python::class_<AlgorithmFactoryObserver, bases<>, AlgorithmFactoryObserverAdapter, boost::noncopyable>(
      "AlgorithmFactoryObserver", "Observes AlgorithmFactory notifications: all only")
      .def("observeUpdate", &AlgorithmFactoryObserverAdapter::observeUpdate, (arg("self"), arg("on")));
}
