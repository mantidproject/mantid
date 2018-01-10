#include "MantidPythonInterface/api/Algorithms/AlgorithmObserverAdapter.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

#include <boost/python/object.hpp>
#include <iostream>

namespace Mantid {
namespace PythonInterface {
using Environment::callMethod;
using Environment::UndefinedAttributeError;

AlgorithmObserverAdapter::AlgorithmObserverAdapter(PyObject *self) : API::AlgorithmObserver(), m_self(self) {
}

void AlgorithmObserverAdapter::progressHandle(const API::IAlgorithm *alg, double p,
  const std::string &msg) {}

void AlgorithmObserverAdapter::startingHandle(API::IAlgorithm_sptr alg) {
  try {
    return callMethod<void>(getSelf(), "startingHandle", alg);
  } catch (UndefinedAttributeError &) {
    std::cerr << " on_starting " << alg->name() << std::endl;
    return;
  }
}

void AlgorithmObserverAdapter::startHandle(const API::IAlgorithm *alg) {}
void AlgorithmObserverAdapter::finishHandle(const API::IAlgorithm *alg) {}
void AlgorithmObserverAdapter::errorHandle(const API::IAlgorithm *alg, const std::string &what) {}

} // namespace PythonInterface
} // namespace Mantid
