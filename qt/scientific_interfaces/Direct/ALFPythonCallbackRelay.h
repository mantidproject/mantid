// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <functional>

namespace MantidQt::CustomInterfaces {

/** Generic relay object for invoking named C++ callbacks from Python.
 *
 * Python can find this object using `findChild(QObject, "ALFPythonCallbackRelay")`
 * and trigger callbacks through QMetaObject::invokeMethod.
 */
class MANTIDQT_DIRECT_DLL ALFPythonCallbackRelay : public QObject {
  Q_OBJECT
public:
  explicit ALFPythonCallbackRelay(QObject *parent = nullptr) : QObject(parent) {
    setObjectName("ALFPythonCallbackRelay");
  }

  void setCallback(QString const &name, std::function<void()> callback) {
    m_callbacks.insert(name, std::move(callback));
  }

  // cppcheck-suppress unknownMacro
public slots:
  void notify(QString const &name) {
    auto callback = m_callbacks.constFind(name);
    if (callback != m_callbacks.constEnd() && *callback)
      (*callback)();
  }

private:
  QHash<QString, std::function<void()>> m_callbacks;
};

} // namespace MantidQt::CustomInterfaces
