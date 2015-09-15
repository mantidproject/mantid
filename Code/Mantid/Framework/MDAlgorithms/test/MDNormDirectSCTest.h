#ifndef MANTID_MDALGORITHMS_MDNORMDIRECTSCTEST_H_
#define MANTID_MDALGORITHMS_MDNORMDIRECTSCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/MDNormDirectSC.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MDAlgorithms::MDNormDirectSC;
using namespace Mantid::API;

class MDNormDirectSCTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDNormDirectSCTest *createSuite() { return new MDNormDirectSCTest(); }
  static void destroySuite( MDNormDirectSCTest *suite ) { delete suite; }


  void test_Init()
  {
    MDNormDirectSC alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_properties()
  {
    std::string mdWsName = "__temp_InputMDWorkspaceName";
    createMDWorkspace(mdWsName);
    std::string saWsName = "__temp_InputSAWorkspaceName";
    createSolidAngleWorkspace(saWsName);

    MDNormDirectSC alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", mdWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("SolidAngleWorkspace", saWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "OutWSName") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputNormalizationWorkspace", "OutNormWSName") );

    AnalysisDataService::Instance().clear();
  }
  
private:

  void createMDWorkspace(const std::string& wsName)
  {
    const int ndims = 2;
    std::string bins = "2,2";
    std::string extents = "0,1,0,1";
    std::vector<std::string> names(ndims);
    names[0] = "A"; names[1] = "B";
    std::vector<std::string> units(ndims);
    units[0] = "a"; units[1] = "b";

    Mantid::MDAlgorithms::CreateMDWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Dimensions", ndims) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Extents", extents) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Names", names) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Units", units) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", wsName) );
    alg.execute();

  }

  void createSolidAngleWorkspace(const std::string& wsName)
  {
    auto sa = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument( 2, 10 );
    AnalysisDataService::Instance().addOrReplace( wsName, sa );
  }

};


#endif /* MANTID_MDALGORITHMS_MDNORMDIRECTSCTEST_H_ */
