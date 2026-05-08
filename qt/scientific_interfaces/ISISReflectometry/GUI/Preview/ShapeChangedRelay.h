// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include "Common/DllConfig.h"
#include <QObject>
#include <functional>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** ShapeChangedRelay is a minimal QObject whose sole purpose is to provide
 *  a Qt meta-object slot that Python can invoke via QMetaObject::invokeMethod.
 *  An instance is created as a child of the Python instrument-view widget so
 *  that Python can locate it with findChild(QObject, "ShapeChangedRelay") and
 *  call notify() to propagate the shape-changed event back into C++.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL ShapeChangedRelay : public QObject {
  Q_OBJECT
public:
  explicit ShapeChangedRelay(QObject *parent = nullptr) : QObject(parent) {}

  void setCallback(std::function<void()> callback) { m_callback = std::move(callback); }

  // cppcheck-suppress unknownMacro
public slots:
  void notify() {
    if (m_callback)
      m_callback();
  }

private:
  std::function<void()> m_callback;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
