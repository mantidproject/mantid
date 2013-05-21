#ifndef MANTID_ALGORITHMS_AVERAGEBYCOMPONENTTEST_H_
#define MANTID_ALGORITHMS_AVERAGEBYCOMPONENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/AverageByComponent.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"



using Mantid::Algorithms::AverageByComponent;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class AverageByComponentTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AverageByComponentTest *createSuite() { return new AverageByComponentTest(); }
  static void destroySuite( AverageByComponentTest *suite ) { delete suite; }


  void test_Init()
  {
    AverageByComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec_0()
  {
    std::string outputWSname("AverageByComponentTest_OutputWS_0");
    std::string inputWSname("AverageByComponentTest_InputWS_0");
    ABCtestWorkspace(inputWSname,false);

    AverageByComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LevelsUp",0) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    Workspace2D_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname));
    TS_ASSERT(result);
    if (!result) return;
    for(size_t i=0;i<result->getNumberHistograms();i++)
    {
        TS_ASSERT_DELTA(static_cast<double>(i)*2,result->readY(i)[0],1e-10);
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_exec_1()
  {
    std::string outputWSname("AverageByComponentTest_OutputWS_1");
    std::string inputWSname("AverageByComponentTest_InputWS_1");
    ABCtestWorkspace(inputWSname,false);

    AverageByComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LevelsUp",1) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    Workspace2D_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname));
    TS_ASSERT(result);
    if (!result) return;
    for(size_t i=0;i<result->getNumberHistograms()/2;i++)
    {
        TS_ASSERT_DELTA(result->readY(i*2)[0],result->readY(i*2+1)[0],1e-10);
        TS_ASSERT_DELTA(static_cast<double>(i)*4+1,result->readY(i*2+1)[0],1e-10);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_exec_2()
  {
    std::string outputWSname("AverageByComponentTest_OutputWS_2");
    std::string inputWSname("AverageByComponentTest_InputWS_2");
    ABCtestWorkspace(inputWSname,false);

    AverageByComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LevelsUp",2) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    Workspace2D_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname));
    TS_ASSERT(result);
    if (!result) return;
    for(size_t i=0;i<result->getNumberHistograms()/4;i++)
    {
        TS_ASSERT_DELTA(result->readY(i*4)[0],result->readY(i*4+1)[0],1e-10);
        TS_ASSERT_DELTA(result->readY(i*4)[0],result->readY(i*4+2)[0],1e-10);
        TS_ASSERT_DELTA(result->readY(i*4)[0],result->readY(i*4+3)[0],1e-10);
        TS_ASSERT_DELTA(static_cast<double>(i)*8+3,result->readY(i*4)[0],1e-10);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_exec_3()
  {
    std::string outputWSname("AverageByComponentTest_OutputWS_3");
    std::string inputWSname("AverageByComponentTest_InputWS_3");
    ABCtestWorkspace(inputWSname,false);

    AverageByComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LevelsUp",3) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    Workspace2D_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname));
    TS_ASSERT(result);
    if (!result) return;
    for(size_t i=0;i<result->getNumberHistograms();i++)
    {
        TS_ASSERT_DELTA(result->readY(i)[0],11.,1e-10);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_exec_15()
  {
    std::string outputWSname("AverageByComponentTest_OutputWS_15");
    std::string inputWSname("AverageByComponentTest_InputWS_15");
    ABCtestWorkspace(inputWSname,false);

    AverageByComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LevelsUp",15) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    Workspace2D_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname));
    TS_ASSERT(result);
    if (!result) return;
    for(size_t i=0;i<result->getNumberHistograms();i++)
    {
        TS_ASSERT_DELTA(result->readY(i)[0],11.,1e-10);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_exec_2_mask()
  {
    std::string outputWSname("AverageByComponentTest_OutputWS_2_mask");
    std::string inputWSname("AverageByComponentTest_InputWS_2_mask");
    ABCtestWorkspace(inputWSname,true);

    AverageByComponent alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LevelsUp",2) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    Workspace2D_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname));
    TS_ASSERT(result);
    if (!result) return;
    for(size_t i=0;i<result->getNumberHistograms()/4;i++)
    {
        TS_ASSERT_DELTA(result->readY(4*i+1)[0],8*i+4,1e-10);
        TS_ASSERT_DELTA(result->readY(4*i+2)[0],8*i+4,1e-10);
        TS_ASSERT_DELTA(result->readY(4*i+3)[0],8*i+4,1e-10);
        TS_ASSERT(result->getDetector(4*i)->isMasked());
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

private:


  void ABCtestWorkspace(std::string inputWSname,bool mask)
  {
    int nSpectra(12);
    Workspace2D_sptr ws2D=WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(nSpectra, 2);
    ws2D->setInstrument(ComponentCreationHelper::createTestInstrumentRectangular(3,2,0));

    Mantid::Geometry::ParameterMap& pmap = ws2D->instrumentParameters();
    for(int i=0;i<nSpectra;i++)
    {
        ws2D->getSpectrum(i)->setDetectorID(i+4);
        if (mask && (i%4==0))
        {
            Mantid::Geometry::IDetector_const_sptr det = ws2D->getDetector(i);
            pmap.addBool(det.get(),"masked",true);
        }
    }

    AnalysisDataService::Instance().add(inputWSname, ws2D);
  }


};


#endif /* MANTID_ALGORITHMS_AVERAGEBYCOMPONENTTEST_H_ */
