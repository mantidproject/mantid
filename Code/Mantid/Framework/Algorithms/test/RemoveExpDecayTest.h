#ifndef MUONREMOVEEXPDECAYTEST_H_
#define MUONREMOVEEXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/RemoveExpDecay.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class RemoveExpDecayTest : public CxxTest::TestSuite
{
public:

  const std::string outputName = "MuonRemoveExpDecay_Output";

  void testInit()
  {
    MuonRemoveExpDecay alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized())
  }

  void testExecute()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    MuonRemoveExpDecay alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", outputName);
    alg.setPropertyValue("Spectra", "0");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted())

    AnalysisDataService::Instance().remove(outputName);
  }

  void testExecuteWhereSepctraNotSet()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    MuonRemoveExpDecay alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted())

    AnalysisDataService::Instance().remove(outputName);
  }

  void test_yUnitLabel()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    MuonRemoveExpDecay alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("OutputWorkspace", outputName);
    alg.execute();

    auto result = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputName);
    TS_ASSERT(result);
    TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");

    AnalysisDataService::Instance().remove(outputName);
  }
};

#endif /*MUONREMOVEEXPDECAYTEST_H_*/
