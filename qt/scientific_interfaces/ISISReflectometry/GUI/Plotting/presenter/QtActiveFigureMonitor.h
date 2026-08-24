// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <QTimer>
#include <functional>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Emits periodic refresh requests for presenter state that depends on the active plot figure.
class MANTIDQT_ISISREFLECTOMETRY_DLL IActiveFigureMonitor {
public:
  virtual ~IActiveFigureMonitor() = default;
  /// Set the callback to invoke when active-figure state should be refreshed.
  virtual void subscribe(std::function<void()> callback) = 0;
  /// Start periodic active-figure refresh notifications.
  virtual void start() = 0;
};

/// Qt-backed active-figure monitor for the Reflectometry plotting presenter.
class MANTIDQT_ISISREFLECTOMETRY_DLL QtActiveFigureMonitor : public IActiveFigureMonitor {
public:
  /// Create a monitor that refreshes once per second.
  QtActiveFigureMonitor();

  void subscribe(std::function<void()> callback) override;
  void start() override;

private:
  QTimer m_timer;
  std::function<void()> m_callback;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
