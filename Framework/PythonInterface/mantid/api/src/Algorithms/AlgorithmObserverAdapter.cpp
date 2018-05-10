#include "MantidPythonInterface/api/Algorithms/AlgorithmObserverAdapter.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

namespace Mantid {
namespace PythonInterface {
using Environment::UndefinedAttributeError;
using Environment::callMethod;

AlgorithmObserverAdapter::AlgorithmObserverAdapter(PyObject *self)
    : API::AlgorithmObserver(), m_self(self) {}

void AlgorithmObserverAdapter::progressHandle(const API::IAlgorithm *alg,
                                              double p,
                                              const std::string &msg) {
  UNUSED_ARG(alg)
  try {
    return callMethod<void>(getSelf(), "progressHandle", p, msg);
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

void AlgorithmObserverAdapter::errorHandle(const API::IAlgorithm *alg,
                                           const std::string &what) {
  UNUSED_ARG(alg)
  try {
    return callMethod<void>(getSelf(), "errorHandle", what);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

} // namespace PythonInterface
} // namespace Mantid
