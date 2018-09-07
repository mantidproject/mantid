#ifndef MPLCPP_FIGURETEST_H
#define MPLCPP_FIGURETEST_H

#include "MantidQtWidgets/MplCpp/Figure.h"
#include <cxxtest/TestSuite.h>

using MantidQt::Widgets::MplCpp::Figure;

class FigureTest : public CxxTest::TestSuite {
public:
  static FigureTest *createSuite() { return new FigureTest; }
  static void destroySuite(FigureTest *suite) { delete suite; }

public:
  void testConstructionYieldsNewFigure() {
    TS_ASSERT_THROWS_NOTHING(Figure fig);
  }
};

#endif // MPLCPP_FIGURETEST_H
