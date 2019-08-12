// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_ALGORITHMOBSERVERADAPTER_H_
#define MANTID_PYTHONINTERFACE_ALGORITHMOBSERVERADAPTER_H_

#include "MantidAPI/AlgorithmObserver.h"
#include <boost/python/wrapper.hpp>

namespace Mantid {
namespace PythonInterface {

/**

A wrapper class helping to export AlgorithmObserver to python.
It provides access from the C++ side to methods defined in python
on subclasses of AlgorithmObserver.
This allows the virtual methods to be overriden by python subclasses.
 */
class DLLExport AlgorithmObserverAdapter : public API::AlgorithmObserver {
public:
  explicit AlgorithmObserverAdapter(PyObject *self);
  AlgorithmObserverAdapter(const AlgorithmObserverAdapter &) = delete;
  AlgorithmObserverAdapter &
  operator=(const AlgorithmObserverAdapter &) = delete;
  void progressHandle(const API::IAlgorithm *alg, double p,
                      const std::string &msg) override;
  void startingHandle(API::IAlgorithm_sptr alg) override;
  void finishHandle(const API::IAlgorithm *alg) override;
  void errorHandle(const API::IAlgorithm *alg,
                   const std::string &what) override;

private:
  /// Return the PyObject that owns this wrapper, i.e. self
  inline PyObject *getSelf() const { return m_self; }
  /// Value of "self" used by python to refer to an instance of this class
  PyObject *m_self;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_ALGORITHMOBSERVERADAPTER_H_ */
