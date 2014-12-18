#ifndef MANTID_API_PLOTAXISLABELTEST_H_
#define MANTID_API_PLOTAXISLABELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtAPI/PlotAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class PlotAxisTest : public CxxTest::TestSuite
{
private:
  class EmptyUtf8Label : public Mantid::Kernel::Unit
  {
public:
    EmptyUtf8Label() : Mantid::Kernel::Unit()
    {
    }

    // Empty overrides of virtual methods
    const std::string unitID() const {return "aUnit";}
    const std::string caption() const {return "Caption";}
    const Mantid::Kernel::UnitLabel label () const {return Mantid::Kernel::UnitLabel("unittext", L"","");}
    void init() {}
    virtual double singleToTOF(const double ) const { return 0; }
    virtual double singleFromTOF(const double ) const { return 0; }
    virtual double conversionTOFMax()const{return std::numeric_limits<double>::quiet_NaN();}
    virtual double conversionTOFMin()const{return std::numeric_limits<double>::quiet_NaN();}

    virtual Unit * clone() const { return new EmptyUtf8Label();}
  };



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

    TS_ASSERT_EQUALS("X axis", PlotAxis(*ws, 0).title());
    TS_ASSERT_EQUALS("Y axis", PlotAxis(*ws, 1).title());
  }

  void test_Empty_Unit_And_Empty_Axis_Title_On_Indexed_Axis_Prints_Default()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->getAxis(0)->setUnit("Empty");
    ws->replaceAxis(1, new Mantid::API::NumericAxis(1));
    ws->getAxis(1)->setUnit("Empty");

    TS_ASSERT_EQUALS("X axis", PlotAxis(*ws, 0).title());
    TS_ASSERT_EQUALS("Y axis", PlotAxis(*ws, 1).title());
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

    TS_ASSERT_EQUALS("Custom title 1", PlotAxis(*ws, 0).title());
    TS_ASSERT_EQUALS("Custom title 2", PlotAxis(*ws, 1).title());
  }

  void test_Axis_With_Unit_Has_Label_In_Parentheses()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->getAxis(0)->setUnit("TOF");
    ws->replaceAxis(1, new Mantid::API::NumericAxis(1));
    ws->getAxis(1)->setUnit("TOF");

    QString expected = QString::fromWCharArray(L"Time-of-flight (\u03bcs)");
    TS_ASSERT_EQUALS(expected, PlotAxis(*ws, 0).title());
    TS_ASSERT_EQUALS(expected, PlotAxis(*ws, 1).title());
  }

  void test_Axis_With_Unit_But_Empty_Utf8_Lable_Uses_Ascii_In_Parentheses()
  {
    using MantidQt::API::PlotAxis;
    using Mantid::Kernel::Units::Degrees;

    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->getAxis(0)->unit() = boost::make_shared<EmptyUtf8Label>();

    TS_ASSERT_EQUALS("Caption (unittext)", PlotAxis(*ws, 0).title());
  }

  void test_SpectraAxis_Gives_Standard_Text()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->replaceAxis(0, new Mantid::API::SpectraAxis(ws.get()));

    TS_ASSERT_EQUALS("Spectrum", PlotAxis(*ws, 0).title());
    TS_ASSERT_EQUALS("Spectrum", PlotAxis(*ws, 1).title());
  }

  void test_Passing_Workspace_Not_Plotting_As_Distribution_Creates_UnitLess_Title_For_Y_Data()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->setYUnit("Counts");

    TS_ASSERT_EQUALS("Counts", PlotAxis(false, *ws).title());
  }

  void test_Passing_Workspace_And_Plotting_As_Distribution_Creates_UnitLess_Title_For_Y_Data()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);
    ws->setYUnit("Counts");

    QString expected = QString::fromWCharArray(L"Counts (\u03bcs\u207b\u00b9)");
    TS_ASSERT_EQUALS("Counts", PlotAxis(true, *ws).title());
  }

  void test_title_from_just_dimension()
  {
    using MantidQt::API::PlotAxis;
    using Mantid::Geometry::MDHistoDimension;
    using Mantid::Kernel::UnitLabel;

      MDHistoDimension dim("tof", "dimx", UnitLabel("us",L"\u03bcs","\\mu s"), 0.0f, 1.0f, 10);
    QString expected = QString::fromWCharArray(L"tof (\u03bcs)");
    TS_ASSERT_EQUALS(expected, PlotAxis(dim).title());
  }

  //---------------------- Failure cases -------------------------------

  void test_Index_Greater_Than_numDims_Or_Less_Than_Zero_Throws_Invalid_Argument()
  {
    using MantidQt::API::PlotAxis;
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    TS_ASSERT_THROWS(PlotAxis(*ws, 2), std::invalid_argument);
    TS_ASSERT_THROWS(PlotAxis(*ws, -1), std::invalid_argument);
  }
};


#endif /* MANTID_API_PLOTAXISLABELTEST_H_ */
