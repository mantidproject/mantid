// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/Algorithms/AlgorithmObserverAdapter.h"
#include "MantidPythonInterface/core/CallMethod.h"

namespace Mantid::PythonInterface {

AlgorithmObserverAdapter::AlgorithmObserverAdapter(PyObject *self) : API::AlgorithmObserver(), m_self(self) {}

void AlgorithmObserverAdapter::progressHandle([[maybe_unused]] const API::IAlgorithm *alg, double p,
                                              const std::string &msg, const double estimatedTime,
                                              const int progressPrecision) {
  try {
    return callMethod<void>(getSelf(), "progressHandle", p, msg, estimatedTime, progressPrecision);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AlgorithmObserverAdapter::startingHandle(API::IAlgorithm_sptr alg) {
  try {
    return callMethod<void>(getSelf(), "startingHandle", alg);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AlgorithmObserverAdapter::finishHandle([[maybe_unused]] const API::IAlgorithm *alg) {
  try {
    return callMethod<void>(getSelf(), "finishHandle");
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AlgorithmObserverAdapter::errorHandle([[maybe_unused]] const API::IAlgorithm *alg, const std::string &what) {
  try {
    return callMethod<void>(getSelf(), "errorHandle", what);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

} // namespace Mantid::PythonInterface
