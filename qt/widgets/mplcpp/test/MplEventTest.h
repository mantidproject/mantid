#ifndef MPLEVENTTEST_H
#define MPLEVENTTEST_H

#include "cxxtest/TestSuite.h"

#include "MantidQtWidgets/MplCpp/MplEvent.h"

using namespace MantidQt::Widgets::MplCpp;

class MplEventTest : public CxxTest::TestSuite {
public:
  static MplEventTest *createSuite() { return new MplEventTest; }
  static void destroySuite(MplEventTest *suite) { delete suite; }

  //
  // MplMouseEvent
  //
  void test_MplMouseEvent_Default_Construction_Produces_Null_Points() {
    MplMouseEvent evt;
    TSM_ASSERT("Default position should be null", evt.pos().isNull());
    TSM_ASSERT("Default data position should be null", evt.dataPos().isNull());
  }

  void test_MplMouseEvent_Construction_Produces_Expected_Points() {
    QPoint pos(-1, 5);
    QPointF dataPos(-2.0f, 10.0f);
    MplMouseEvent evt(pos, dataPos, Qt::LeftButton);
    TS_ASSERT_EQUALS(pos, evt.pos());
    TS_ASSERT_EQUALS(dataPos, evt.dataPos());
    TS_ASSERT_EQUALS(Qt::LeftButton, evt.button());
  }
};

#endif // MPLEVENTTEST_H
