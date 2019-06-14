// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/AnalysisDataServiceObserverAdapter.h"
#include "MantidAPI/AnalysisDataServiceObserver.h"
#include "MantidPythonInterface/core/CallMethod.h"

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

void AnalysisDataServiceObserverAdapter::addHandle(const std::string &wsName,
                                                   const Workspace_sptr &ws) {
  try {
    return callMethod<void>(getSelf(), "addHandle", wsName, ws);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AnalysisDataServiceObserverAdapter::replaceHandle(
    const std::string &wsName, const Workspace_sptr &ws) {
  try {
    return callMethod<void>(getSelf(), "replaceHandle", wsName, ws);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AnalysisDataServiceObserverAdapter::deleteHandle(
    const std::string &wsName, const Workspace_sptr &ws) {
  try {
    return callMethod<void>(getSelf(), "deleteHandle", wsName, ws);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AnalysisDataServiceObserverAdapter::clearHandle() {
  try {
    return callMethod<void>(getSelf(), "clearHandle");
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AnalysisDataServiceObserverAdapter::renameHandle(
    const std::string &wsName, const std::string &newName) {
  try {
    return callMethod<void>(getSelf(), "renameHandle", wsName, newName);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AnalysisDataServiceObserverAdapter::groupHandle(const std::string &wsName,
                                                     const Workspace_sptr &ws) {
  try {
    return callMethod<void>(getSelf(), "groupHandle", wsName, ws);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AnalysisDataServiceObserverAdapter::unGroupHandle(
    const std::string &wsName, const Workspace_sptr &ws) {
  try {
    return callMethod<void>(getSelf(), "unGroupHandle", wsName, ws);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

void AnalysisDataServiceObserverAdapter::groupUpdateHandle(
    const std::string &wsName, const Workspace_sptr &ws) {
  try {
    return callMethod<void>(getSelf(), "groupUpdateHandle", wsName, ws);
  } catch (UndefinedAttributeError &) {
    return;
  }
}

} // namespace PythonInterface
} // namespace Mantid
