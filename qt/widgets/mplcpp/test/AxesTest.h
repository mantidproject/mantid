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
    const Python::Object figureModule(
        Python::Handle<>(PyImport_ImportModule("matplotlib.figure")));
    const Python::Object figure = figureModule.attr("Figure")();
    const Python::Object rect(
        Python::Handle<>(Py_BuildValue("(iiii)", 0, 0, 1, 1)));
    const Python::Object axesModule(
        Python::Handle<>(PyImport_ImportModule("matplotlib.axes")));

    Python::Object pyaxes = axesModule.attr("Axes")(figure, rect);
    TS_ASSERT_THROWS_NOTHING(Axes axes(pyaxes));
  }
};

#endif // MPLCPP_AXESTEST_H
