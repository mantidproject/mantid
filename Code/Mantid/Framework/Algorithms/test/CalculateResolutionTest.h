#ifndef CALCULATERESOLUTIONTEST_H_
#define CALCULATERESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateResolution.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidDataHandling/Load.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

class CalculateResolutionTest : public CxxTest::TestSuite
{
public:
  void testCalculateResolution()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename","INTER00013460.nxs");
    loader.setPropertyValue("OutputWorkspace","CalculateResolutionTestWS");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("CalculateResolutionTestWS");
    TS_ASSERT(ws);

    CalculateResolution alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", ws->name());
    alg.setProperty("Theta", 0.7);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    const double res = alg.getProperty("Resolution");
    TS_ASSERT_DELTA(res, 0.03403, 0.00001);
  }
};

#endif /*CALCULATERESOLUTIONTEST_H_*/
