// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/AnalysisDataServiceObserverAdapter.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidPythonInterface/core/CallMethod.h"

#include <iostream>

namespace Mantid {
namespace PythonInterface {

AnalysisDataServiceObserverAdapter::AnalysisDataServiceObserverAdapter(
    PyObject *self)
    : API::AnalysisDataServiceObserver(), m_self(self) {}

void AnalysisDataServiceObserverAdapter::anyChangeHandle() {
  try {
    return callMethod<void>(getSelf(), "anyChangeHandle");
  } catch (UndefinedAttributeError &) {
    return;
  }
}

} // namespace PythonInterface
} // namespace Mantid
