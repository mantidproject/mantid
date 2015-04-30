#ifndef MUONREMOVEEXPDECAYTEST_H_
#define MUONREMOVEEXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/RemoveExpDecay.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

const std::string outputName = "MuonRemoveExpDecay_Output";

class RemoveExpDecayTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveExpDecayTest *createSuite() { return new RemoveExpDecayTest(); }
  static void destroySuite( RemoveExpDecayTest *suite ) { delete suite; }

  RemoveExpDecayTest() {
    FrameworkManager::Instance();
  }


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
    TS_ASSERT(alg.isInitialized());
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", outputName);
    alg.setPropertyValue("Spectra", "0");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted())
  }

  void testExecuteWhereSepctraNotSet()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    MuonRemoveExpDecay alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted())
  }

  void test_yUnitLabel()
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1,1);

    MuonRemoveExpDecay alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("OutputWorkspace", outputName);
    alg.execute();

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");
    TS_ASSERT(result);
    TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");
  }
};

#endif /*MUONREMOVEEXPDECAYTEST_H_*/
