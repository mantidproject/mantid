// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_FIGURETEST_H
#define MPLCPP_FIGURETEST_H

#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::Figure;
using MantidQt::Widgets::MplCpp::Normalize;
using MantidQt::Widgets::MplCpp::ScalarMappable;
using namespace MantidQt::Widgets::Common;

class FigureTest : public CxxTest::TestSuite {
public:
  static FigureTest *createSuite() { return new FigureTest; }
  static void destroySuite(FigureTest *suite) { delete suite; }

public:
  void testDefaultFigureHasTightLayout() {
    Figure fig;
    TS_ASSERT_EQUALS(true, fig.pyobj().attr("get_tight_layout")());
  }

  void testConstructFigureWithNoTightLayout() {
    Figure fig{false};
    TS_ASSERT_EQUALS(false, fig.pyobj().attr("get_tight_layout")());
  }

  void testGcaReturnsAxesIfNotAdded() {
    Figure fig{false};
    TS_ASSERT_THROWS_NOTHING(fig.gca());
  }

  void testAddAxes() {
    Figure fig{false};
    TS_ASSERT_THROWS_NOTHING(fig.addAxes(0.1, 0.1, 0.9, 0.9));
  }

  void testSetFaceColorFromString() {
    Figure fig{false};
    fig.setFaceColor("r");
    const auto color = fig.faceColor();
    TS_ASSERT_EQUALS(255, color.red());
    TS_ASSERT_EQUALS(0, color.green());
    TS_ASSERT_EQUALS(0, color.blue());
  }

  void testSetFaceColorFromQColor() {
    Figure fig{false};
    fig.setFaceColor(QColor("red"));
    const auto color = fig.faceColor();
    TS_ASSERT_EQUALS(255, color.red());
    TS_ASSERT_EQUALS(0, color.green());
    TS_ASSERT_EQUALS(0, color.blue());
  }

  void testSubPlot() {
    Figure fig{false};
    TS_ASSERT_THROWS_NOTHING(fig.addSubPlot(111));
  }

  void testSubPlotAcceptsProjection() {
    Python::Object module{
        Python::NewRef(PyImport_ImportModule("mantid.plots"))};
    Figure fig{false};
    TS_ASSERT_THROWS_NOTHING(fig.addSubPlot(111, "mantid"));
  }

  void testColorbar() {
    Figure fig{false};
    const auto cax = fig.addAxes(0.1, 0.1, 0.9, 0.9);
    ScalarMappable mappable(Normalize(-1, 1), "jet");

    TS_ASSERT_THROWS_NOTHING(fig.colorbar(mappable, cax));
  }

  // -------------------------- Failure tests ---------------------------
  void testFigureConstructedWithNonFigureThrows() {
    TS_ASSERT_THROWS(Figure fig(Python::NewRef(Py_BuildValue("(i)", 1))),
                     const std::invalid_argument &);
  }
};

#endif // FigureTEST_H
