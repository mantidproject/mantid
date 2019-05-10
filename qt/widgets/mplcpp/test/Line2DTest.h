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
  void testConstructionWithVectorData() {
    std::vector<double> xdata{{2, 3, 4}}, ydata{{4, 5, 6}};
    const auto expectedX{xdata}, expectedY{ydata};
    Line2D line(rawMplLine2D(), std::move(xdata), std::move(ydata));
    TS_ASSERT_EQUALS(expectedX, line.rawData().xaxis)
    TS_ASSERT_EQUALS(expectedY, line.rawData().yaxis)
  }

  void testConstructionWithDataStruct() {
    const std::vector<double> xdata{{2, 3, 4}}, ydata{{4, 5, 6}};
    Line2D::Data lineData{xdata, ydata};
    Line2D line(rawMplLine2D(), std::move(lineData));
    TS_ASSERT_EQUALS(xdata, line.rawData().xaxis)
    TS_ASSERT_EQUALS(ydata, line.rawData().yaxis)
  }

  void testGetColorReturnsExpectedColor() {
    Line2D line(rawMplLine2D(), {1, 2}, {1, 2});
    line.pyobj().attr("set_color")("r");

    auto color = line.getColor();
    TS_ASSERT_EQUALS(255, color.red());
    TS_ASSERT_EQUALS(0, color.green());
    TS_ASSERT_EQUALS(0, color.blue());
  }

  void testSetDataUpdatesDataWithVectors() {
    Line2D line(rawMplLine2D(), {1, 2}, {1, 2});
    std::vector<double> newx{{2, 3, 4}}, newy{{4, 5, 6}};
    const auto expectedX{newx}, expectedY{newy};
    line.setData(std::move(newx), std::move(newy));
    assertDataMatches(line, expectedX, expectedY);
  }

  void testSetDataUpdatesDataWithDataStruct() {
    Line2D line(rawMplLine2D(), {1, 2}, {1, 2});
    const std::vector<double> newx{{2, 3, 4}}, newy{{4, 5, 6}};
    line.setData(Line2D::Data{newx, newy});
    assertDataMatches(line, newx, newy);
  }

  // ---------------------- failure tests --------------------
  void testConstructionWithNonLine2DObjectThrowsInvalidArgument() {
    Python::Object obj{Python::NewRef(Py_BuildValue("(i)", 1))};
    TS_ASSERT_THROWS(Line2D line(obj, {}, {}), const std::invalid_argument &);
  }

private:
  void assertDataMatches(const Line2D &line,
                         const std::vector<double> &expectedX,
                         const std::vector<double> &expectedY) {
    // check owned vector mathces
    TS_ASSERT_EQUALS(expectedX, line.rawData().xaxis)
    TS_ASSERT_EQUALS(expectedY, line.rawData().yaxis)

    // check viewed data from the python instance
    auto linex = line.pyobj().attr("get_xdata")();
    auto liney = line.pyobj().attr("get_ydata")();
    for (size_t i = 0; i < expectedX.size(); ++i) {
      TS_ASSERT_EQUALS(expectedX[i], linex[i]);
      TS_ASSERT_EQUALS(expectedY[i], liney[i]);
    }
  }

  // Creates a raw matplotlib Line2D instance with some fake data
  Python::Object rawMplLine2D() {
    // A Line2D requires x and y data sequences
    const Python::Object data{Python::NewRef(Py_BuildValue("(f, f)", 0., 1.))};
    const Python::Object linesModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.lines"))};
    return linesModule.attr("Line2D")(data, data);
  }
};

#endif // MPLCPP_LINE2DTEST_H
