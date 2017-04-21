#ifndef MANTID_MANTIDWIDGETS_TRACKEDACTIONEST_H_
#define MANTID_MANTIDWIDGETS_TRACKEDACTIONEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtMantidWidgets/TrackedAction.h"
#include <QCoreApplication>

using MantidQt::MantidWidgets::TrackedAction;

class TrackedActionTest : public CxxTest::TestSuite {
  // inner class
  class TestableTrackedAction : public TrackedAction {
  public:
    TestableTrackedAction(QObject *parent)
        : TrackedAction(parent), m_lastName(){};
    TestableTrackedAction(const QString &text, QObject *parent)
        : TrackedAction(text, parent), m_lastName(){};
    TestableTrackedAction(const QIcon &icon, const QString &text,
                          QObject *parent)
        : TrackedAction(icon, text, parent), m_lastName(){};

    std::string getLastUsedName() const { return m_lastName; };

  protected:
    void registerUsage(const std::string &name) override { m_lastName = name; };

  private:
    std::string m_lastName;
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

    std::string appNamePrefix =
        QCoreApplication::applicationName().toStdString() + "->";

    TS_ASSERT_EQUALS(action.getTrackingName(),
                     appNamePrefix + "TestName"); // default state
    action.setTrackingName("TestName2");
    TS_ASSERT_EQUALS(action.getTrackingName(), "TestName2"); // altered state
  }

  void testTrackingCallLogic() {
    QObject parent;
    TestableTrackedAction action(QString::fromStdString("TestName"), &parent);

    // tracking should be on by default
    TS_ASSERT_EQUALS(action.getIsTracking(), true); // default state
    TS_ASSERT_EQUALS(action.getLastUsedName(), ""); // default state

    action.setTrackingName("ShouldTrack");
    action.trigger();
    TS_ASSERT_EQUALS(action.getLastUsedName(),
                     "ShouldTrack"); // tracking occurred state
    action.setIsTracking(false);
    action.setTrackingName("ShouldNotTrack");
    action.trigger();
    TS_ASSERT_DIFFERS(action.getLastUsedName(),
                      "ShouldNotTrack"); // Should not have tracked
  }
};

#endif /* MANTID_MANTIDWIDGETS_TRACKEDACTIONEST_H_ */