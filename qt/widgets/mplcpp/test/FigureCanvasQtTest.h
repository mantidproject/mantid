#ifndef MPLCPP_FIGURECANVASQTTEST_H
#define MPLCPP_FIGURECANVASQTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

using MantidQt::Widgets::MplCpp::FigureCanvasQt;

class FigureCanvasQtTest : CxxTest::TestSuite {
public:
  static FigureCanvasQtTest *createSuite() { return new FigureCanvasQtTest; }
  static void destroySuite(FigureCanvasQtTest *suite) { delete suite; }

public:
  void testConstructionYieldsSingleSubPlot() {
    FigureCanvasQt canvas;
    auto geometry = canvas.axes().attr("get_geometry")();
    TS_ASSERT_EQUALS(1, geometry[0]);
    TS_ASSERT_EQUALS(1, geometry[1]);
    TS_ASSERT_EQUALS(1, geometry[2]);
  }
};

#endif // MPLCPP_FIGURECANVASQTTEST_H
