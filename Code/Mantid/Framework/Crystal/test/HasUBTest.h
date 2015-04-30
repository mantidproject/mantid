#ifndef MANTID_CRYSTAL_HASUBTEST_H_
#define MANTID_CRYSTAL_HASUBTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/HasUB.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class HasUBTest: public CxxTest::TestSuite
{

private:

  // Helper method to create a matrix workspace.
  std::string createMatrixWorkspace(const bool withOrientedLattice = true)
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1, 2);
    if (withOrientedLattice)
    {
      OrientedLattice *latt = new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);
      ws->mutableSample().setOrientedLattice(latt);
      delete latt;
    }
    const std::string wsName = "TestWorkspace";
    AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return wsName;
  }

  // Helper method to create a MDHW
  std::string createMDHistoWorkspace(const uint16_t nExperimentInfosToAdd = 2)
  {
    const std::string wsName = "TestWorkspace";
    auto ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 10, 10, 1, wsName);
    OrientedLattice *latt =new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);
    ws->getExperimentInfo(0)->mutableSample().setOrientedLattice(latt);

    for (uint16_t i = 1; i < nExperimentInfosToAdd; ++i)
    {
      ExperimentInfo_sptr experimentInfo = boost::make_shared<ExperimentInfo>();
      ws->addExperimentInfo(experimentInfo);
      ws->getExperimentInfo(i)->mutableSample().setOrientedLattice(latt);
    }

    delete latt;
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
    return wsName;
  }

  // Execute the algorithm
  bool doExecute(const std::string& wsName)
  {
    // Create and run the algorithm
    HasUB alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", wsName));
    alg.execute();
    TS_ASSERT( alg.isExecuted());

    return alg.getProperty("HasUB");
  }

  // Execute the algorithm
  bool doExecuteMultiInfo(const std::string& wsName)
  {
    // Create and run the algorithm
    HasUB alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", wsName));
    alg.execute();
    TS_ASSERT( alg.isExecuted());

    return alg.getProperty("HasUB");
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HasUBTest *createSuite()
  {
    return new HasUBTest();
  }
  static void destroySuite(HasUBTest *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    HasUB alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_safely_continue_if_noOrientedLattice()
  {
    // Name of the output workspace.
    const bool createOrientedLattice = false;
    const std::string wsName = createMatrixWorkspace(createOrientedLattice);
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    TSM_ASSERT("No oriented lattice to begin with", !ws->sample().hasOrientedLattice());

    bool hasUB = true;
    TSM_ASSERT_THROWS_NOTHING("Should safely handle this.", hasUB = doExecute(wsName));
    TS_ASSERT(!hasUB);

    // Clean up.
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_dry_run_with_intput_workspace_not_expeirmentinfo()
  {
    using Mantid::DataObjects::TableWorkspace;
    Workspace_sptr inws = boost::make_shared<TableWorkspace>();
    const std::string wsName = "tablews";
    AnalysisDataService::Instance().addOrReplace(wsName, inws);

    bool hasUB = true;
    TSM_ASSERT_THROWS_NOTHING("Should safely handle this.", hasUB = doExecute(wsName));
    TS_ASSERT(!hasUB);

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_check_MatrixWorkspace()
  {
    // Name of the output workspace.
    const std::string wsName = createMatrixWorkspace();
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    TSM_ASSERT("OrientedLattice should be present to begin with", ws->sample().hasOrientedLattice());

    bool hasUB = doExecute(wsName);

    TSM_ASSERT("OutputFlag should indicate possible removal", hasUB);

    // Clean up.
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_check_OrientedLatticeMDHW()
  {
    // Name of the output workspace.
    const std::string wsName = createMDHistoWorkspace();

    bool hasUB = doExecuteMultiInfo(wsName);

    TSM_ASSERT("OutputFlag should indicate potential removal", hasUB);
    // Clean up.
    AnalysisDataService::Instance().remove(wsName);
  }

};

#endif /* MANTID_CRYSTAL_HASUBTEST_H_ */
