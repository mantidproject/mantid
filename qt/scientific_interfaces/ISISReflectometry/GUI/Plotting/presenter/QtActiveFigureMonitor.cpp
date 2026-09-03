// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtActiveFigureMonitor.h"

#pragma push_macro("slots")
#undef slots

#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

#pragma pop_macro("slots")

#include <stdexcept>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
QObject *activeFigureObserver() {
  using namespace MantidQt::Widgets::Common;

  try {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    auto const module = Python::Object{Python::NewRef(PyImport_ImportModule("workbench.plotting.globalfiguremanager"))};
    auto const observer = Python::Object{module.attr("GlobalFigureManager").attr("initialiseActiveFiguresObserver")()};
    return Python::extract<QObject>(observer);
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}
} // namespace

QtActiveFigureMonitor::QtActiveFigureMonitor() = default;

void QtActiveFigureMonitor::subscribe(std::function<void()> callback) {
  m_callback = std::move(callback);
  if (m_isSubscribed) {
    return;
  }

  auto const connection = QObject::connect(activeFigureObserver(), SIGNAL(active_figure_changed()), this,
                                           SLOT(notifyActiveFigureChanged()), Qt::QueuedConnection);
  if (!connection) {
    throw std::runtime_error("Could not subscribe to Workbench active-figure changes.");
  }
  m_isSubscribed = true;
}

void QtActiveFigureMonitor::notifyActiveFigureChanged() {
  if (m_callback) {
    m_callback();
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
