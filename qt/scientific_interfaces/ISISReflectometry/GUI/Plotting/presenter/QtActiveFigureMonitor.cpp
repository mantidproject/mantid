// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtActiveFigureMonitor.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

QtActiveFigureMonitor::QtActiveFigureMonitor() {
  m_timer.setInterval(1000);
  QObject::connect(&m_timer, &QTimer::timeout, [this]() {
    if (m_callback) {
      m_callback();
    }
  });
}

void QtActiveFigureMonitor::subscribe(std::function<void()> callback) { m_callback = std::move(callback); }

void QtActiveFigureMonitor::start() { m_timer.start(); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
