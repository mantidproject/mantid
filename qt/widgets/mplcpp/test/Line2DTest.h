// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_LINE2DTEST_H
#define MPLCPP_LINE2DTEST_H

#include "MantidQtWidgets/MplCpp/Line2D.h"
#include <QRgb>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::Widgets::MplCpp;
using namespace MantidQt::Widgets::Common;

class Line2DTest : public CxxTest::TestSuite {
public:
  static Line2DTest *createSuite() { return new Line2DTest; }
  static void destroySuite(Line2DTest *suite) { delete suite; }

  // ---------------------- success tests --------------------
  void testConstructionRequiresMplLine2DObject() {
    TS_ASSERT_THROWS_NOTHING(Line2D line(pyLine2D(), {1, 2}, {1, 2}));
  }

  void testGetColorReturnsExpectedColor() {
    Line2D line(pyLine2D(), {1, 2}, {1, 2});
    line.pyobj().attr("set_color")("r");

    auto color = line.getColor();
    TS_ASSERT_EQUALS(255, color.red());
    TS_ASSERT_EQUALS(0, color.green());
    TS_ASSERT_EQUALS(0, color.blue());
  }

  // ---------------------- failure tests --------------------
  void testConstructionWithNonLine2DObjectThrowsInvalidArgument() {
    Python::Object obj{Python::NewRef(Py_BuildValue("(i)", 1))};
    TS_ASSERT_THROWS(Line2D line(obj, {}, {}), std::invalid_argument);
  }

private:
  Python::Object pyLine2D() {
    // A Line2D requires x and y data sequences
    const Python::Object data{Python::NewRef(Py_BuildValue("(f, f)", 0., 1.))};
    const Python::Object linesModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.lines"))};
    return linesModule.attr("Line2D")(data, data);
  }
};

#endif // MPLCPP_LINE2DTEST_H
