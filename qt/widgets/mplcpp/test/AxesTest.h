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

  void testPlotGivesLineWithExpectedData() {
    Axes axes(pyAxes());
    std::vector<double> xsrc{1, 2, 3}, ysrc{1, 2, 3};
    auto line = axes.plot(xsrc, ysrc);
    auto linex = line.pyobj().attr("get_xdata")(true);
    auto liney = line.pyobj().attr("get_ydata")(true);
    for (size_t i = 0; i < xsrc.size(); ++i) {
      TSM_ASSERT_EQUALS("Mismatch in X data", linex[i], xsrc[i]);
      TSM_ASSERT_EQUALS("Mismatch in Y data", liney[i], ysrc[i]);
    }
  }

  void testPlotWithNoFormatUsesDefault() {
    Axes axes(pyAxes());
    auto line = axes.plot({1, 2, 3}, {1, 2, 3});
    TS_ASSERT_EQUALS('b', line.pyobj().attr("get_color")());
    TS_ASSERT_EQUALS('-', line.pyobj().attr("get_linestyle")());
  }

  void testPlotUsesFormatStringIfProvided() {
    Axes axes(pyAxes());
    const std::string format{"ro"};
    auto line = axes.plot({1, 2, 3}, {1, 2, 3}, format.c_str());
    TS_ASSERT_EQUALS(format[0], line.pyobj().attr("get_color")());
    TS_ASSERT_EQUALS(format[1], line.pyobj().attr("get_marker")());
  }

  void testSetXScaleWithKnownScaleType() {
    Axes axes(pyAxes());
    axes.setXScale("symlog");
  }

  void testSetYScaleWithKnownScaleType() {
    Axes axes(pyAxes());
    axes.setYScale("symlog");
  }

  // ----------------- failure tests ---------------------
  void testPlotThrowsWithEmptyData() {
    Axes axes(pyAxes());
    TS_ASSERT_THROWS(axes.plot({}, {}), std::invalid_argument);
    TS_ASSERT_THROWS(axes.plot({1}, {}), std::invalid_argument);
    TS_ASSERT_THROWS(axes.plot({}, {1}), std::invalid_argument);
  }

  void testSetXScaleWithUnknownScaleTypeThrows() {
    Axes axes(pyAxes());
    TS_ASSERT_THROWS(axes.setXScale("notascaletype"), std::invalid_argument);
  }

  void testSetYScaleWithUnknownScaleTypeThrows() {
    Axes axes(pyAxes());
    TS_ASSERT_THROWS(axes.setYScale("notascaletype"), std::invalid_argument);
  }

private:
  Python::Object pyAxes() {
    // An Axes requires a figure and rectangle definition
    // to be constructible
    const Python::Object figureModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.figure"))};
    const Python::Object figure{figureModule.attr("Figure")()};
    const Python::Object rect{
        Python::NewRef(Py_BuildValue("(iiii)", 0, 0, 1, 1))};
    const Python::Object axesModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.axes"))};
    return axesModule.attr("Axes")(figure, rect);
  }
};

#endif // MPLCPP_AXESTEST_H
