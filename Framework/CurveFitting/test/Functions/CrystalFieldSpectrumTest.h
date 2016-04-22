#ifndef CRYSTALFIELDSPECTRUMTEST_H_
#define CRYSTALFIELDSPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Algorithms/Fit.h"

#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;
using namespace Mantid::API;
//using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;

//typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
//typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;


class CrystalFieldSpectrumTest : public CxxTest::TestSuite {
public:
  void testFunction() {
    CrystalFieldSpectrum fun;
  }
};

#endif /*CRYSTALFIELDSPECTRUMTEST_H_*/
