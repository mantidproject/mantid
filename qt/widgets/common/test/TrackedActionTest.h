// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/Common/TrackedAction.h"
#include <QCoreApplication>

using MantidQt::MantidWidgets::TrackedAction;

class TrackedActionTest : public CxxTest::TestSuite {
  // inner class
  class TestableTrackedAction : public TrackedAction {
  public:
    TestableTrackedAction(QObject *parent) : TrackedAction(parent), m_lastName() {};
    TestableTrackedAction(const QString &text, QObject *parent) : TrackedAction(text, parent), m_lastName() {};
    TestableTrackedAction(const QIcon &icon, const QString &text, QObject *parent)
        : TrackedAction(icon, text, parent), m_lastName() {};

    std::vector<std::string> getLastUsedName() const { return m_lastName; };

  protected:
    void registerUsage(const std::vector<std::string> &name) override { m_lastName = name; };

  private:
    std::vector<std::string> m_lastName;
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TrackedActionTest *createSuite() { return new TrackedActionTest(); }
  static void destroySuite(TrackedActionTest *suite) { delete suite; }

  void testIsTrackingGetSetGet() {
    QObject parent;
    TestableTrackedAction action(&parent);

    TS_ASSERT_EQUALS(action.getIsTracking(), true); // default state
    action.setIsTracking(false);
    TS_ASSERT_EQUALS(action.getIsTracking(), false); // altered state
  }

  void testTrackingNameGetSetGet() {
    QObject parent;
    TestableTrackedAction action(QString::fromStdString("TestName"), &parent);

    std::string appNamePrefix = QCoreApplication::applicationName().toStdString();

    TS_ASSERT_EQUALS(action.getTrackingName().size(), 2);
    TS_ASSERT_EQUALS(action.getTrackingName()[0],
                     appNamePrefix);                           // default state
    TS_ASSERT_EQUALS(action.getTrackingName()[1], "TestName"); // default state

    action.setTrackingName({"TestName2"});

    TS_ASSERT_EQUALS(action.getTrackingName().size(), 1);
    TS_ASSERT_EQUALS(action.getTrackingName()[0],
                     "TestName2"); // altered state
  }

  void testTrackingCallLogic() {
    QObject parent;
    TestableTrackedAction action(QString::fromStdString("TestName"), &parent);

    // tracking should be on by default
    TS_ASSERT_EQUALS(action.getIsTracking(), true);           // default state
    TS_ASSERT_EQUALS(action.getLastUsedName().empty(), true); // default state

    action.setTrackingName({"ShouldTrack"});
    action.trigger();
    TS_ASSERT_EQUALS(action.getLastUsedName().size(), 1);
    TS_ASSERT_EQUALS(action.getLastUsedName()[0],
                     "ShouldTrack"); // tracking occurred state
    action.setIsTracking(false);
    action.setTrackingName({"ShouldNotTrack"});
    action.trigger();
    TS_ASSERT_EQUALS(action.getLastUsedName().size(), 1);
    TS_ASSERT_DIFFERS(action.getLastUsedName()[0],
                      "ShouldNotTrack"); // Should not have tracked
  }
};
