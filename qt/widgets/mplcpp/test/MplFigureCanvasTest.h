#ifndef MPLFIGURECANVASTEST_H
#define MPLFIGURECANVASTEST_H

#include "cxxtest/TestSuite.h"

#include "MantidQtWidgets/MplCpp/MplFigureCanvas.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"

using namespace MantidQt::Widgets::MplCpp;

class MplFigureCanvasTest : public CxxTest::TestSuite {
public:
  static MplFigureCanvasTest *createSuite() { return new MplFigureCanvasTest; }
  static void destroySuite(MplFigureCanvasTest *suite) { delete suite; }

  //---------------------------------------------------------------------------
  // Success
  //---------------------------------------------------------------------------
  void test_Default_Construction_Yields_Single_Subplot() {
    MplFigureCanvas canvas;
    TSM_ASSERT_EQUALS("Default canvas should have a single subplot",
                      SubPlotSpec(1, 1), canvas.geometry());
    TSM_ASSERT_EQUALS("Default canvas should have 0 lines", 0, canvas.nlines());
  }

  void test_Construction_With_SubPlot_Layout_Respects_It() {
    MplFigureCanvas canvas(231);
    TSM_ASSERT_EQUALS("Canvas should respect subplot layout request",
                      SubPlotSpec(2, 3), canvas.geometry());
    TSM_ASSERT_EQUALS("Default canvas should have 0 lines", 0, canvas.nlines());
  }

  void test_Expected_Limits_Returned_Given_Data() {
    MplFigureCanvas canvas;
    std::vector<double> dataX{1, 2, 3, 4, 5}, dataY{2, 3, 4, 5, 6};
    canvas.plotLine(dataX, dataY, "r-");

    auto xlimits = canvas.limits(Axes::Scale::X);
    TS_ASSERT_EQUALS(std::make_tuple(1.0, 5.0), xlimits);
    auto ylimits = canvas.limits(Axes::Scale::Y);
    TS_ASSERT_EQUALS(std::make_tuple(2.0, 6.0), ylimits);
  }

  void test_Adding_A_Line_Increase_Line_Count_By_One() {
    MplFigureCanvas canvas;
    std::vector<double> data{1, 2, 3, 4, 5};
    canvas.plotLine(data, data, "r-");
    TSM_ASSERT_EQUALS("plotLine should increase line count by one", 1,
                      canvas.nlines());
  }

  void test_Removing_A_Line_Decreases_Line_Count_By_One() {
    MplFigureCanvas canvas;
    std::vector<double> data{1, 2, 3, 4, 5};
    canvas.plotLine(data, data, "r-");
    canvas.removeLine(0);
    TSM_ASSERT_EQUALS("removeLine should decrease line count by one", 0,
                      canvas.nlines());
  }

  void test_Clear_Removes_All_Lines() {
    MplFigureCanvas canvas;
    std::vector<double> data{1, 2, 3, 4, 5};
    canvas.plotLine(data, data, "r-");
    canvas.plotLine(data, data, "bo");
    canvas.clearLines();
    TSM_ASSERT_EQUALS("clear should remove all lines", 0, canvas.nlines());
  }

  void test_Setting_Axis_And_Figure_Titles() {
    MplFigureCanvas canvas;
    canvas.setLabel(Axes::Label::X, "new x label");
    TS_ASSERT_EQUALS("new x label", canvas.label(Axes::Label::X));
    canvas.setLabel(Axes::Label::Y, "new y label");
    TS_ASSERT_EQUALS("new y label", canvas.label(Axes::Label::Y));
    canvas.setLabel(Axes::Label::Title, "new title");
    TS_ASSERT_EQUALS("new title", canvas.label(Axes::Label::Title));
  }

  void test_Setting_X_Scale_Does_Not_Change_Y() {
    MplFigureCanvas canvas;
    TS_ASSERT_THROWS_NOTHING(canvas.setScale(Axes::Scale::X, "log"));
    TS_ASSERT_EQUALS("log", canvas.scaleType(Axes::Scale::X).toStdString());
    TS_ASSERT_EQUALS("linear", canvas.scaleType(Axes::Scale::Y).toStdString());
  }

  void test_Setting_Y_Scale_Does_Not_Change_X() {
    MplFigureCanvas canvas;
    TS_ASSERT_THROWS_NOTHING(canvas.setScale(Axes::Scale::Y, "log"));
    TS_ASSERT_EQUALS("log", canvas.scaleType(Axes::Scale::Y).toStdString());
    TS_ASSERT_EQUALS("linear", canvas.scaleType(Axes::Scale::X).toStdString());
  }

  void test_Setting_Both_Scales() {
    MplFigureCanvas canvas;
    TS_ASSERT_THROWS_NOTHING(canvas.setScale(Axes::Scale::Both, "log"));
    TS_ASSERT_EQUALS("log", canvas.scaleType(Axes::Scale::Y).toStdString());
    TS_ASSERT_EQUALS("log", canvas.scaleType(Axes::Scale::X).toStdString());
  }

  void test_toDataCoordinates_Gives_Data_Point_Inside_Axes() {
    MplFigureCanvas canvas;
    std::vector<double> data{1, 2, 3, 4, 5};
    canvas.plotLine(data, data, "r-");

    // Middle canvas should be roughly the middle of the data
    QPoint pixelPos(static_cast<int>(0.5 * canvas.canvasWidget()->width()),
                    static_cast<int>(0.5 * canvas.canvasWidget()->height()));
    QPointF dataCoords;
    TS_ASSERT_THROWS_NOTHING(dataCoords = canvas.toDataCoordinates(pixelPos));
    TS_ASSERT_DELTA(2.9f, dataCoords.x(), 0.1f);
    TS_ASSERT_DELTA(3.0f, dataCoords.y(), 0.1f);
  }

  //---------------------------------------------------------------------------
  // Failure
  //---------------------------------------------------------------------------

  void test_PlotLine_With_Different_Length_Arrays_Throws() {
    MplFigureCanvas canvas;
    std::vector<double> arr1{1, 2, 3}, arr2{1, 2, 3, 4};
    TSM_ASSERT_THROWS("plotLine should throw if len(x) < len(y)",
                      canvas.plotLine(arr1, arr2, "r-"), PythonError);
    TSM_ASSERT_THROWS("plotLine should throw if len(x) > len(y)",
                      canvas.plotLine(arr2, arr1, "r-"), PythonError);
  }

  void test_addSubPlot_Throws_With_Invalid_Configuration() {
    MplFigureCanvas canvas;
    TS_ASSERT_THROWS(canvas.addSubPlot(-111), PythonError);
    TS_ASSERT_THROWS(canvas.addSubPlot(1000), PythonError);
  }
};

#endif // MPLFIGURECANVASTEST_H
