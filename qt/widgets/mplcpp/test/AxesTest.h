#ifndef MPLCPP_AXESTEST_H
#define MPLCPP_AXESTEST_H

#include "MantidQtWidgets/MplCpp/Axes.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::Widgets::MplCpp;

class AxesTest : public CxxTest::TestSuite {
public:
  static AxesTest *createSuite() { return new AxesTest; }
  static void destroySuite(AxesTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testConstructWithPyObjectAxes() {
    TS_ASSERT_THROWS_NOTHING(Axes axes(pyAxes()));
  }

  void testSetXLabel() {
    Axes axes(pyAxes());
    axes.setXLabel("X");
    TS_ASSERT_EQUALS("X", axes.pyobj().attr("get_xlabel")());
  }

  void testSetYLabel() {
    Axes axes(pyAxes());
    axes.setYLabel("Y");
    TS_ASSERT_EQUALS("Y", axes.pyobj().attr("get_ylabel")());
  }

  void testSetTitle() {
    Axes axes(pyAxes());
    axes.setTitle("Title");
    TS_ASSERT_EQUALS("Title", axes.pyobj().attr("get_title")());
  }

  void testPlotWithValidDataReturnsLine2D() {
    Axes axes(pyAxes());
    std::vector<double> xsrc{1, 2, 3}, ysrc{1, 2, 3};
    auto line = axes.plot(xsrc, ysrc);
    auto linex = line.pyobj().attr("get_xdata")(true);
//    for(size_t i = 0; i < xsrc.size(); ++i) {
//      TSM_ASSERT_EQUALS("Mismatch in X data", linex[i], xsrc[i]);
//    }
  }

  // ----------------- failure tests ---------------------
  void testPlotThrowsWithEmptyData() {
    Axes axes(pyAxes());
    TS_ASSERT_THROWS(axes.plot({}, {}), std::invalid_argument);
    TS_ASSERT_THROWS(axes.plot({1}, {}), std::invalid_argument);
    TS_ASSERT_THROWS(axes.plot({}, {1}), std::invalid_argument);
  }

private:
  Python::Object pyAxes() {
    // An Axes requires a figure and rectangle definition
    // to be constructible
    const auto figureModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.figure"))};
    const auto figure{figureModule.attr("Figure")()};
    const auto rect{Python::NewRef(Py_BuildValue("(iiii)", 0, 0, 1, 1))};
    const auto axesModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.axes"))};

    return axesModule.attr("Axes")(figure, rect);
  }
};

#endif // MPLCPP_AXESTEST_H
