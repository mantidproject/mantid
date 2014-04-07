#ifndef MANTID_API_PLOTAXISLABELTEST_H_
#define MANTID_API_AXISLABELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtAPI/PlotAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class PlotAxisTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotAxisTest *createSuite() { return new PlotAxisTest(); }
  static void destroySuite( PlotAxisTest *suite ) { delete suite; }

  void test_NoUnit_On_Indexed_Axis_Prints_Default()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->replaceAxis(1, new Mantid::API::NumericAxis(1));

    TS_ASSERT_EQUALS("X axis", PlotAxis(ws, 0).title());
    TS_ASSERT_EQUALS("Y axis", PlotAxis(ws, 1).title());
  }

  void test_Empty_Unit_And_Empty_Axis_Title_On_Indexed_Axis_Prints_Default()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->getAxis(0)->setUnit("Empty");
    ws->replaceAxis(1, new Mantid::API::NumericAxis(1));
    ws->getAxis(1)->setUnit("Empty");

    TS_ASSERT_EQUALS("X axis", PlotAxis(ws, 0).title());
    TS_ASSERT_EQUALS("Y axis", PlotAxis(ws, 1).title());
  }

  void test_Empty_Unit_And_Non_Empty_Title_On_Indexed_Axis_Prints_Title()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->getAxis(0)->setUnit("Empty");
    auto * ax0 = ws->getAxis(0);
    ax0->setUnit("Empty");
    ax0->title() = "Custom title 1";
    ws->replaceAxis(1, new Mantid::API::NumericAxis(1));
    auto * ax1 = ws->getAxis(1);
    ax1->setUnit("Empty");
    ax1->title() = "Custom title 2";

    TS_ASSERT_EQUALS("Custom title 1", PlotAxis(ws, 0).title());
    TS_ASSERT_EQUALS("Custom title 2", PlotAxis(ws, 1).title());
  }

  void test_Axis_With_Unit_Has_Label_In_Parentheses()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->getAxis(0)->setUnit("TOF");
    ws->replaceAxis(1, new Mantid::API::NumericAxis(1));
    ws->getAxis(1)->setUnit("TOF");

    QString expected = QString::fromUtf8("Time-of-flight (\u03bcs)");
    TS_ASSERT_EQUALS(expected, PlotAxis(ws, 0).title());
    TS_ASSERT_EQUALS(expected, PlotAxis(ws, 1).title());
  }

  void test_SpectraAxis_Gives_Standard_Text()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->replaceAxis(0, new Mantid::API::SpectraAxis(ws.get()));

    TS_ASSERT_EQUALS("Spectrum Number", PlotAxis(ws, 0).title());
    TS_ASSERT_EQUALS("Spectrum Number", PlotAxis(ws, 1).title());
  }

  //---------------------- Failure cases -------------------------------

  void test_Index_Greater_Than_1_Or_Less_Than_Zero_Throws_Invalid_Argument()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    TS_ASSERT_THROWS(PlotAxis(ws, 2), std::invalid_argument);
    TS_ASSERT_THROWS(PlotAxis(ws, -1), std::invalid_argument);
  }
};


#endif /* MANTID_API_AXISLABELTEST_H_ */
