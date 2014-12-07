#ifndef MANTID_MDALGORITHMS_MDNORMSCDTEST_H_
#define MANTID_MDALGORITHMS_MDNORMSCDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/MDNormSCD.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MDAlgorithms::MDNormSCD;
using namespace Mantid::API;

class MDNormSCDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDNormSCDTest *createSuite() { return new MDNormSCDTest(); }
  static void destroySuite( MDNormSCDTest *suite ) { delete suite; }


  void test_Init()
  {
    MDNormSCD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_properties()
  {
    std::string mdWsName = "__temp_InputMDWorkspaceName";
    createMDWorkspace(mdWsName);
    std::string fluxGoodWsName = "__temp_InputGoodFluxWorkspaceName";
    createGoodFluxWorkspace(fluxGoodWsName);
    std::string fluxBadWsName = "__temp_InputBadFluxWorkspaceName";
    createBadFluxWorkspace(fluxBadWsName);
    std::string saWsName = "__temp_InputSAWorkspaceName";
    createBadFluxWorkspace(saWsName);

    MDNormSCD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", mdWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FluxWorkspace", fluxGoodWsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FluxWorkspace", fluxBadWsName) ); // it isn't bad any more
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

  void createGoodFluxWorkspace(const std::string& wsName)
  {
    auto flux = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument( 2, 10 );
    auto &x = flux->dataX(0);
    auto &y1 = flux->dataY(1);
    
    for(size_t i = 0; i < y1.size(); ++i)
    {
      y1[i] = 2 * x[i];
    }
    flux->setX(1,x);
    flux->getAxis(0)->setUnit("Momentum");

    AnalysisDataService::Instance().addOrReplace( wsName, flux );
  }

  void createBadFluxWorkspace(const std::string& wsName)
  {
    auto flux = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument( 2, 10 );
    auto &x = flux->dataX(0);
    auto &y1 = flux->dataY(1);
    
    for(size_t i = 0; i < y1.size(); ++i)
    {
      y1[i] = -2 * x[i];
    }
    flux->setX(1,x);
    flux->getAxis(0)->setUnit("Momentum");

    AnalysisDataService::Instance().addOrReplace( wsName, flux );
  }

  void createSolidAngleWorkspace(const std::string& wsName)
  {
    auto sa = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument( 2, 10 );
    AnalysisDataService::Instance().addOrReplace( wsName, sa );
  }

};


#endif /* MANTID_MDALGORITHMS_MDNORMSCDTEST_H_ */
