// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_ANALYSISDATASERVICEOBSERVERADAPTER_H_
#define MANTID_PYTHONINTERFACE_ANALYSISDATASERVICEOBSERVERADAPTER_H_

#include "MantidAPI/AnalysisDataServiceObserver.h"
#include <boost/python/wrapper.hpp>

namespace Mantid {
namespace PythonInterface {

/**
A wrapper class helping to export AnalysisDataServiceObserver to python.
It provides access from the C++ side to methods defined in python
on subclasses of AnalysisDataServiceObserver.
This allows the virtual methods to be overriden by python subclasses.
 */

class DLLExport AnalysisDataServiceObserverAdapter
    : public API::AnalysisDataServiceObserver {
public:
  explicit AnalysisDataServiceObserverAdapter(PyObject *self);
  AnalysisDataServiceObserverAdapter(
      const AnalysisDataServiceObserverAdapter &) = delete;
  AnalysisDataServiceObserverAdapter &
  operator=(const AnalysisDataServiceObserverAdapter &) = delete;

  void anyChangeHandle() override;
  void addHandle(const std::string &wsName, const Workspace_sptr &ws) override;
  void replaceHandle(const std::string &wsName,
                     const Workspace_sptr &ws) override;
  void deleteHandle(const std::string &wsName,
                    const Workspace_sptr &ws) override;
  void clearHandle() override;
  void renameHandle(const std::string &wsName,
                    const std::string &newName) override;
  void groupHandle(const std::string &wsName,
                   const Workspace_sptr &ws) override;
  void unGroupHandle(const std::string &wsName,
                     const Workspace_sptr &ws) override;
  void groupUpdateHandle(const std::string &wsName,
                         const Workspace_sptr &ws) override;

private:
  /// Return the PyObject that owns this wrapper, i.e. self
  inline PyObject *getSelf() const { return m_self; }
  /// Value of "self" used by python to refer to an instance of this class
  PyObject *m_self;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /*MANTID_PYTHONINTERFACE_ANALYSISDATASERVICEOBSERVERADAPTER_H_*/