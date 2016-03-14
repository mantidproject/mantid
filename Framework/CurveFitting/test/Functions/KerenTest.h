#ifndef MANTID_CURVEFITTING_KERENTEST_H_
#define MANTID_CURVEFITTING_KERENTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Keren.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::CurveFitting::Functions::Keren;

class KerenTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static KerenTest *createSuite() { return new KerenTest(); }
  static void destroySuite( KerenTest *suite ) { delete suite; }

  void test_name() {
    Keren function;
    TS_ASSERT_EQUALS("Keren", function.name());
  }

  void test_category() {
    Keren function;
    TS_ASSERT_EQUALS("Muon", function.category());
  }

private:
    void getMockData(Mantid::MantidVec &y, Mantid::MantidVec &e) {
    // Mock data got from an Excel spreadsheet with
    // Lambda = 0.16, Omega = 0.4, Beta = 1.2 &  A = 1.5

    y[0] = 1.5;
    y[1] = 1.141313628;
    y[2] = 0.591838582;
    y[3] = 0.217069719;
    y[4] = 0.143355934;
    y[5] = 0.256915274;
    y[6] = 0.365739273;
    y[7] = 0.360727646;
    y[8] = 0.260023319;
    y[9] = 0.146136639;
    y[10] = 0.080853314;
    y[11] = 0.068393706;
    y[12] = 0.075537727;
    y[13] = 0.071800717;
    y[14] = 0.051659705;
    y[15] = 0.028746883;
    y[16] = 0.017073081;
    y[17] = 0.018710399;
    y[18] = 0.025298535;
    y[19] = 0.027436201;

    for (int i = 0; i <= 20; i++) {
      e[i] = 0.01;
    }
  }

};


#endif /* MANTID_CURVEFITTING_KERENTEST_H_ */