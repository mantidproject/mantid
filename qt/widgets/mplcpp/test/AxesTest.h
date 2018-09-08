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
  void testConstructWithAxesIsSuccessful() {
    // An Axes requires a figure and rectangle definition
    // to be constructible
    const auto figureModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.figure"))};
    const auto figure{figureModule.attr("Figure")()};
    const auto rect{Python::NewRef(Py_BuildValue("(iiii)", 0, 0, 1, 1))};
    const auto axesModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.axes"))};

    Python::Object pyaxes = axesModule.attr("Axes")(figure, rect);
    TS_ASSERT_THROWS_NOTHING(Axes axes(pyaxes));
  }
};

#endif // MPLCPP_AXESTEST_H
