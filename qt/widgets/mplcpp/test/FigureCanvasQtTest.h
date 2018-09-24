#ifndef MPLCPP_FIGURECANVASQTTEST_H
#define MPLCPP_FIGURECANVASQTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

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
    auto fig = Python::NewRef(PyImport_ImportModule("matplotlib.figure"))
                   .attr("Figure")();
    auto axes{fig.attr("add_subplot")(221)};
    FigureCanvasQt canvas{std::move(axes)};
    auto geometry = canvas.gca().pyobj().attr("get_geometry")();
    TS_ASSERT_EQUALS(2, geometry[0]);
    TS_ASSERT_EQUALS(2, geometry[1]);
    TS_ASSERT_EQUALS(1, geometry[2]);
  }
};

#endif // MPLCPP_FIGURECANVASQTTEST_H
