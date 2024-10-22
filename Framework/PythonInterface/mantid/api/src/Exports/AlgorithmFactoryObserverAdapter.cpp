// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/Algorithms/AlgorithmFactoryObserverAdapter.h"
#include "MantidAPI/AlgorithmFactoryObserver.h"
#include "MantidPythonInterface/core/CallMethod.h"

namespace Mantid::PythonInterface {

AlgorithmFactoryObserverAdapter::AlgorithmFactoryObserverAdapter(PyObject *self)
    : API::AlgorithmFactoryObserver(), m_self(self) {}

void AlgorithmFactoryObserverAdapter::updateHandle() {
  try {
    return callMethod<void>(getSelf(), "updateHandle");
  } catch (UndefinedAttributeError &) {
    return;
  }
}

} // namespace Mantid::PythonInterface
