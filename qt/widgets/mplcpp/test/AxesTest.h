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
