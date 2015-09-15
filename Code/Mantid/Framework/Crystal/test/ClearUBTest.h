#ifndef MANTID_CRYSTAL_CLEARUBTEST_H_
#define MANTID_CRYSTAL_CLEARUBTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidCrystal/ClearUB.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/TableWorkspace.h"

using Mantid::Crystal::ClearUB;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class ClearUBTest : public CxxTest::TestSuite
{

private:

  // Single return type
  struct SingleReturnType
  {
    ExperimentInfo_sptr ExperimentInfo;
    bool DidClear;
  };

  // Multiple return type
  struct MultipleReturnType
  {
    MultipleExperimentInfos_sptr ExperimentInfos;
    bool DidClear;
  };

  // Helper method to create a matrix workspace.
  std::string createMatrixWorkspace(const bool withOrientedLattice = true)
  {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1, 2);
    if(withOrientedLattice)
    {
      OrientedLattice latt(1.0,2.0,3.0, 90, 90, 90);
      ws->mutableSample().setOrientedLattice(&latt);
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
    OrientedLattice latt(1.0,2.0,3.0, 90, 90,90);
    ws->getExperimentInfo(0)->mutableSample().setOrientedLattice(&latt);

    for(uint16_t i = 1; i < nExperimentInfosToAdd; ++i)
    {
      ExperimentInfo_sptr experimentInfo = boost::make_shared<ExperimentInfo>();
      ws->addExperimentInfo(experimentInfo);
      ws->getExperimentInfo(i)->mutableSample().setOrientedLattice(&latt);
    }

    AnalysisDataService::Instance().addOrReplace(wsName, ws);
    return wsName;
  }

  // Execute the algorithm
  SingleReturnType doExecute(const std::string& wsName)
  {
    // Create and run the algorithm
    ClearUB alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", wsName));
    alg.execute();
    TS_ASSERT( alg.isExecuted());

    //Check results
    auto expInfo = AnalysisDataService::Instance().retrieveWS<ExperimentInfo>(wsName);
    SingleReturnType output;
    output.ExperimentInfo = expInfo;
    output.DidClear = alg.getProperty("DoesClear");
    return output;
  }

  // Execute the algorithm
  MultipleReturnType doExecuteMultiInfo(const std::string& wsName)
  {
    // Create and run the algorithm
    ClearUB alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", wsName));
    alg.execute();
    TS_ASSERT( alg.isExecuted());

    //Check results
    auto expInfos = AnalysisDataService::Instance().retrieveWS<MultipleExperimentInfos>(wsName);
    MultipleReturnType output;
    output.ExperimentInfos = expInfos;
    output.DidClear = alg.getProperty("DoesClear");
    return output;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClearUBTest *createSuite() { return new ClearUBTest(); }
  static void destroySuite( ClearUBTest *suite ) { delete suite; }


  void test_Init()
  {
    ClearUB alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_removeOrientedLattice()
  {
    // Name of the output workspace.
    const std::string wsName = createMatrixWorkspace();
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    TSM_ASSERT("OrientedLattice should be present!", ws->sample().hasOrientedLattice());
  
    auto output = doExecute(wsName);
    auto expInfo = output.ExperimentInfo;
    
    TS_ASSERT( expInfo)
    TSM_ASSERT("OrientedLattice should be gone!", !expInfo->sample().hasOrientedLattice());
    TSM_ASSERT("OutputFlag should indicate removal", output.DidClear);

    // Clean up.
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_removeOrientedLatticeMDHW()
  {
    // Name of the output workspace.
    const std::string wsName = createMDHistoWorkspace();

    auto output = doExecuteMultiInfo(wsName);
    auto expInfo = output.ExperimentInfos;

    TS_ASSERT( expInfo)

    // Check that every experiment info has been cleared.
    const uint16_t nInfos = expInfo->getNumExperimentInfo();
    for (uint16_t i = 0; i < nInfos; ++i)
    {
      TSM_ASSERT("OrientedLattice should be gone!", !expInfo->getExperimentInfo(i)->sample().hasOrientedLattice());
    }
    TSM_ASSERT("OutputFlag should indicate removal", output.DidClear);
    // Clean up.
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_safely_continue_if_noOrientedLattice()
  {
    // Name of the output workspace.
    const bool createOrientedLattice = false;
    const std::string wsName = createMatrixWorkspace(createOrientedLattice);
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    TSM_ASSERT("No oriented lattice to begin with", !ws->sample().hasOrientedLattice());

    auto output = doExecute(wsName);
    auto expInfo = output.ExperimentInfo;

    TS_ASSERT( expInfo)
    TSM_ASSERT("OrientedLattice should be gone!", !expInfo->sample().hasOrientedLattice());

    // Clean up.
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_throw_if_not_experimentinfo_workspace()
  {
    using Mantid::DataObjects::TableWorkspace;
    Workspace_sptr inws = boost::make_shared<TableWorkspace>();
    const std::string wsName = "tablews";
    AnalysisDataService::Instance().addOrReplace(wsName, inws);

    TSM_ASSERT_THROWS("Input workspace type is not derived from ExperimentInfo or MultipleExperimentInfo, so should throw.", doExecute(wsName), std::invalid_argument&);
    AnalysisDataService::Instance().remove(wsName);
  }

};


#endif /* MANTID_CRYSTAL_CLEARUBTEST_H_ */
