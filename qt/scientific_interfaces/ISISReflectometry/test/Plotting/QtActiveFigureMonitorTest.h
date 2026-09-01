// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Plotting/presenter/QtActiveFigureMonitor.h"

#pragma push_macro("slots")
#undef slots

#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"

#pragma pop_macro("slots")

#include <QCoreApplication>
#include <cxxtest/TestSuite.h>

using MantidQt::CustomInterfaces::ISISReflectometry::QtActiveFigureMonitor;

class QtActiveFigureMonitorTest : public CxxTest::TestSuite {
public:
  static QtActiveFigureMonitorTest *createSuite() { return new QtActiveFigureMonitorTest(); }
  static void destroySuite(QtActiveFigureMonitorTest *suite) { delete suite; }

  void testSubscriberIsNotifiedWhenTheActiveFigureChanges() {
    auto notificationCount = 0;
    QtActiveFigureMonitor monitor;
    monitor.subscribe([&notificationCount]() { ++notificationCount; });

    emitActiveFigureChanged();
    QCoreApplication::processEvents();

    TS_ASSERT_EQUALS(notificationCount, 1);
  }

  void testResubscribingReplacesTheCallbackWithoutAddingAnotherConnection() {
    auto firstNotificationCount = 0;
    auto secondNotificationCount = 0;
    QtActiveFigureMonitor monitor;
    monitor.subscribe([&firstNotificationCount]() { ++firstNotificationCount; });
    monitor.subscribe([&secondNotificationCount]() { ++secondNotificationCount; });

    emitActiveFigureChanged();
    QCoreApplication::processEvents();

    TS_ASSERT_EQUALS(firstNotificationCount, 0);
    TS_ASSERT_EQUALS(secondNotificationCount, 1);
  }

  void testQueuedNotificationsAreDiscardedWhenTheMonitorIsDestroyed() {
    auto notificationCount = 0;
    {
      QtActiveFigureMonitor monitor;
      monitor.subscribe([&notificationCount]() { ++notificationCount; });
      emitActiveFigureChanged();
    }

    QCoreApplication::processEvents();

    TS_ASSERT_EQUALS(notificationCount, 0);
  }

private:
  void emitActiveFigureChanged() {
    using namespace MantidQt::Widgets::Common;

    Mantid::PythonInterface::GlobalInterpreterLock lock;
    auto const module = Python::Object{Python::NewRef(PyImport_ImportModule("workbench.plotting.globalfiguremanager"))};
    auto const observer = Python::Object{module.attr("GlobalFigureManager").attr("initialiseActiveFiguresObserver")()};
    observer.attr("active_figure_changed").attr("emit")();
  }
};
