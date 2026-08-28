// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <QObject>
#include <functional>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Notifies presenter state when the active plot figure changes.
class MANTIDQT_ISISREFLECTOMETRY_DLL IActiveFigureMonitor {
public:
  virtual ~IActiveFigureMonitor() = default;
  /// Subscribe to notifications that the active figure changed.
  virtual void subscribe(std::function<void()> callback) = 0;
};

/// Qt-backed active-figure monitor for the Reflectometry plotting presenter.
class MANTIDQT_ISISREFLECTOMETRY_DLL QtActiveFigureMonitor : public QObject, public IActiveFigureMonitor {
  Q_OBJECT
public:
  /// Create a monitor for Workbench active-figure notifications.
  QtActiveFigureMonitor();

  void subscribe(std::function<void()> callback) override;

  // cppcheck-suppress unknownMacro
private slots:
  void notifyActiveFigureChanged();

private:
  std::function<void()> m_callback;
  bool m_isSubscribed{false};
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
