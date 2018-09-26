#ifndef MPLCPP_FIGURECANVASQTTEST_H
#define MPLCPP_FIGURECANVASQTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

using MantidQt::Widgets::MplCpp::Figure;
using MantidQt::Widgets::MplCpp::FigureCanvasQt;

class FigureCanvasQtTest : public CxxTest::TestSuite {
public:
  static FigureCanvasQtTest *createSuite() { return new FigureCanvasQtTest; }
  static void destroySuite(FigureCanvasQtTest *suite) { delete suite; }

public:
  void testConstructionYieldsExpectedSubplot() {
    FigureCanvasQt canvas{111};
    auto geometry = canvas.gca().pyobj().attr("get_geometry")();
    TS_ASSERT_EQUALS(1, geometry[0]);
    TS_ASSERT_EQUALS(1, geometry[1]);
    TS_ASSERT_EQUALS(1, geometry[2]);
  }

  void testConstructionCapturesGivenAxesObject() {
    using namespace MantidQt::Widgets::MplCpp;
    Figure fig;
    fig.addSubPlot(221);
    FigureCanvasQt canvas{std::move(fig)};
    auto geometry = canvas.gca().pyobj().attr("get_geometry")();
    TS_ASSERT_EQUALS(2, geometry[0]);
    TS_ASSERT_EQUALS(2, geometry[1]);
    TS_ASSERT_EQUALS(1, geometry[2]);
  }
};

#endif // MPLCPP_FIGURECANVASQTTEST_H
