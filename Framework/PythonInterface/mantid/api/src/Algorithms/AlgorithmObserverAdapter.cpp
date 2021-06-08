// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/Algorithms/AlgorithmObserverAdapter.h"
#include "MantidPythonInterface/core/CallMethod.h"

namespace Mantid {
namespace PythonInterface {

AlgorithmObserverAdapter::AlgorithmObserverAdapter(PyObject *self) : API::AlgorithmObserver(), m_self(self) {}

void AlgorithmObserverAdapter::progressHandle(const API::IAlgorithm *alg, double p, const std::string &msg,
                                              const double estimatedTime, const int progressPrecision) {
  UNUSED_ARG(alg)
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

void AlgorithmObserverAdapter::finishHandle(const API::IAlgorithm *alg) {
  UNUSED_ARG(alg)
  try {
    return callMethod<void>(getSelf(), "finishHandle");
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AlgorithmObserverAdapter::errorHandle(const API::IAlgorithm *alg, const std::string &what) {
  UNUSED_ARG(alg)
  try {
    return callMethod<void>(getSelf(), "errorHandle", what);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

} // namespace PythonInterface
} // namespace Mantid
