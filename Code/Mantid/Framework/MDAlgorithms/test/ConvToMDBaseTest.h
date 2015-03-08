#ifndef MDEVENTS_CONV2_MDBASE_TEST_H_
#define MDEVENTS_CONV2_MDBASE_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDAlgorithms/ConvToMDBase.h"
#include "MantidMDEvents/MDEventWSWrapper.h"
//#include "MantidMDEvents/MDTransfDEHelper.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

// TEST HELPER
class ConvToMDBaseTestHelper : public ConvToMDBase
{
  size_t conversionChunk(size_t ){return 0;}
public:
  void runConversion(API::Progress *){};
  int getNumThreads(){return m_NumThreads;}

};

// The test
class ConvToMDBaseTest : public CxxTest::TestSuite, public WorkspaceCreationHelper::MockAlgorithm
{
    // Matrix workspace description;
     MDWSDescription WSD;
     // matrix ws, sometimes can be obtained from description as a const pointer, but better not to do it for modifications
     Mantid::API::MatrixWorkspace_sptr ws2D;
     // the shared pointer to the expected taget Event ws; Not used here, just set up
     boost::shared_ptr<MDEventWSWrapper> outWSWrapper;
     // preprocessed detectors location (emulates static algorithm variable)
     MDEvents::ConvToMDPreprocDet DetLoc;
public:
static ConvToMDBaseTest *createSuite() {
    return new ConvToMDBaseTest();    
}
static void destroySuite(ConvToMDBaseTest  * suite) { delete suite; }    

void testConstructor()
{
  boost::scoped_ptr<ConvToMDBaseTestHelper> pConvToMDBase;
  TS_ASSERT_THROWS_NOTHING(pConvToMDBase.reset(new ConvToMDBaseTestHelper()));
  TSM_ASSERT_EQUALS("uninitiated num threads parameter should be equal -1",-1,pConvToMDBase->getNumThreads());

}
void testInitAndSetNumThreads()
{
  ConvToMDBaseTestHelper testClass;
  TS_ASSERT_THROWS_NOTHING(outWSWrapper->createEmptyMDWS(WSD));

  TSM_ASSERT_THROWS("Should throw if detectors prepositions are not initiated ",testClass.initialize(WSD,outWSWrapper),std::runtime_error);
  // should calculate the detectors info for WDS
  this->buildDetInfo(ws2D);

  TS_ASSERT_THROWS_NOTHING(testClass.initialize(WSD,outWSWrapper));
  TSM_ASSERT_EQUALS("uninitiated num threads parameter should be still equal -1",-1,testClass.getNumThreads());
  
  std::string QMode  = WSD.getQMode();
  std::string dEMode = WSD.getEModeStr();
  ws2D->mutableRun().addProperty("NUM_THREADS",0.);

  WSD.buildFromMatrixWS(ws2D,QMode,dEMode);
  this->buildDetInfo(ws2D);

  TS_ASSERT_THROWS_NOTHING(testClass.initialize(WSD,outWSWrapper));
  TSM_ASSERT_EQUALS("Initialized above num threads parameter should be equal to 0 (which would disable multithreading)",0,testClass.getNumThreads());
  ws2D->mutableRun().removeProperty("NUM_THREADS");

  // and this should let us run 2 thread program
  ws2D->mutableRun().addProperty("NUM_THREADS",2.);
  WSD.buildFromMatrixWS(ws2D,QMode,dEMode);
  this->buildDetInfo(ws2D);

  TS_ASSERT_THROWS_NOTHING(testClass.initialize(WSD,outWSWrapper));
  TSM_ASSERT_EQUALS("Initialized above num threads parameter should be equal to 2:",2,testClass.getNumThreads());

  // avoid side effects of this test to possible others;
  ws2D->mutableRun().removeProperty("NUM_THREADS");

}
private: 
  ConvToMDBaseTest()
  {
     ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);
     // ADD time series property
     ws2D->mutableRun().addProperty("H",10.,"Gs");
  
     std::vector<double> dimMin(4,-10);
     std::vector<double> dimMax(4, 20);
     std::vector<std::string> PropNamews;
     WSD.setMinMax(dimMin,dimMax);   
     WSD.buildFromMatrixWS(ws2D,"Q3D","Direct",PropNamews);

    outWSWrapper = boost::shared_ptr<MDEventWSWrapper>(new MDEventWSWrapper());
  }
  // helper function to build the detector info
  void buildDetInfo( Mantid::API::MatrixWorkspace_sptr spWS)
  {
     WSD.m_PreprDetTable = WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(spWS);
  }

};


#endif