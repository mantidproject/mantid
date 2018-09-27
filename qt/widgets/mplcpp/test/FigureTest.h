#ifndef MPLCPP_FIGURETEST_H
#define MPLCPP_FIGURETEST_H

#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::Figure;
using MantidQt::Widgets::MplCpp::Normalize;
using MantidQt::Widgets::MplCpp::ScalarMappable;

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

  void testAddAxes() {
    Figure fig{false};
    TS_ASSERT_THROWS_NOTHING(fig.addAxes(0.1, 0.1, 0.9, 0.9));
  }

  void testSubPlot() {
    Figure fig{false};
    TS_ASSERT_THROWS_NOTHING(fig.addSubPlot(111));
  }

  void testColorbar() {
    Figure fig{false};
    auto cax = fig.addAxes(0.1, 0.1, 0.9, 0.9);
    ScalarMappable mappable(Normalize(-1, 1), "jet");

    TS_ASSERT_THROWS_NOTHING(fig.colorbar(mappable, cax));
  }
};

#endif // FigureTEST_H
