// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_AXESTEST_H
#define MPLCPP_AXESTEST_H

#include "MantidQtWidgets/MplCpp/Axes.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::Widgets::MplCpp;
using namespace MantidQt::Widgets::Common;

class AxesTest : public CxxTest::TestSuite {
public:
  static AxesTest *createSuite() { return new AxesTest; }
  static void destroySuite(AxesTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testConstructWithPyObjectAxes() {
    TS_ASSERT_THROWS_NOTHING(Axes axes(pyAxes()));
  }

  void testClear() {
    Axes axes(pyAxes());
    std::vector<double> xsrc{1, 2, 3}, ysrc{1, 2, 3};
    auto line = axes.plot(xsrc, ysrc);
    TS_ASSERT_EQUALS(1, line.pyobj().attr("get_xdata")()[0]);
    TS_ASSERT_EQUALS(1, Python::Len(axes.pyobj().attr("lines")));
    axes.clear();
    TS_ASSERT_EQUALS(0, Python::Len(axes.pyobj().attr("lines")));
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

  void testPlotSetsLabelIfProvided() {
    Axes axes(pyAxes());
    QString label{"mylabel"};
    auto line = axes.plot({1, 2, 3}, {1, 2, 3}, "b-", label);
    TS_ASSERT_EQUALS(label.toLatin1().constData(),
                     line.pyobj().attr("get_label")());
  }

  void testSetXScaleWithKnownScaleType() {
    Axes axes(pyAxes());
    axes.setXScale("symlog");
  }

  void testSetYScaleWithKnownScaleType() {
    Axes axes(pyAxes());
    axes.setYScale("symlog");
  }

  void testGetXLimReturnsXLimits() {
    Axes axes(pyAxes());
    axes.plot({5, 6, 7, 8}, {10, 11, 12, 13});
    axes.setXLim(4, 9);
    auto xlimits = axes.getXLim();
    TS_ASSERT_DELTA(4, std::get<0>(xlimits), 1e-5);
    TS_ASSERT_DELTA(9, std::get<1>(xlimits), 1e-5);
  }

  void testGetYLimReturnsYLimits() {
    Axes axes(pyAxes());
    axes.plot({5, 6, 7, 8}, {10, 11, 12, 13});
    axes.setYLim(9, 14);
    auto ylimits = axes.getYLim();
    TS_ASSERT_DELTA(9, std::get<0>(ylimits), 1e-5);
    TS_ASSERT_DELTA(14, std::get<1>(ylimits), 1e-5);
  }

  void testTextAddsTextAddGivenCoordinate() {
    Axes axes(pyAxes());
    auto artist = axes.text(0.5, 0.4, "test", "left");

    TS_ASSERT_EQUALS("test", artist.pyobj().attr("get_text")());
    TS_ASSERT_EQUALS(0.5, artist.pyobj().attr("get_position")()[0]);
    TS_ASSERT_EQUALS(0.4, artist.pyobj().attr("get_position")()[1]);
  }

  // ----------------- failure tests ---------------------
  void testPlotThrowsWithEmptyData() {
    Axes axes(pyAxes());
    TS_ASSERT_THROWS(axes.plot({}, {}), const std::invalid_argument &);
    TS_ASSERT_THROWS(axes.plot({1}, {}), const std::invalid_argument &);
    TS_ASSERT_THROWS(axes.plot({}, {1}), const std::invalid_argument &);
  }

  void testSetXScaleWithUnknownScaleTypeThrows() {
    Axes axes(pyAxes());
    TS_ASSERT_THROWS(axes.setXScale("notascaletype"),
                     const std::invalid_argument &);
  }

  void testSetYScaleWithUnknownScaleTypeThrows() {
    Axes axes(pyAxes());
    TS_ASSERT_THROWS(axes.setYScale("notascaletype"),
                     const std::invalid_argument &);
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
