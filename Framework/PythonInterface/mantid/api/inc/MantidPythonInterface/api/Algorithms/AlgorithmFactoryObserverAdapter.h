// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_ALGORITHMFACTORYOBSERVERADAPTER_H_
#define MANTID_PYTHONINTERFACE_ALGORITHMFACTORYOBSERVERADAPTER_H_

#include "MantidAPI/AlgorithmFactoryObserver.h"
#include <boost/python/wrapper.hpp>

namespace Mantid {
namespace PythonInterface {

/**
A wrapper class helping to export AnalysisDataServiceObserver to python.
It provides access from the C++ side to methods defined in python
on subclasses of AnalysisDataServiceObserver.
This allows the virtual methods to be overriden by python subclasses.
 */

class DLLExport AlgorithmFactoryObserverAdapter
    : public API::AlgorithmFactoryObserver {
public:
  explicit AlgorithmFactoryObserverAdapter(PyObject *self);
  AlgorithmFactoryObserverAdapter(const AlgorithmFactoryObserverAdapter &) =
      delete;
  AlgorithmFactoryObserverAdapter &
  operator=(const AlgorithmFactoryObserverAdapter &) = delete;

  void updateHandle() override;

private:
  /// Return the PyObject that owns this wrapper, i.e. self
  inline PyObject *getSelf() const { return m_self; }
  /// Value of "self" used by python to refer to an instance of this class
  PyObject *m_self;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /*MANTID_PYTHONINTERFACE_ALGORITHMFACTORYOBSERVERADAPTER_H_*/