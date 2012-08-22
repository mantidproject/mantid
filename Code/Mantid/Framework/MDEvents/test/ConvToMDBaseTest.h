#ifndef MDEVENTS_CONV2_MDBASE_TEST_H_
#define MDEVENTS_CONV2_MDBASE_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/ConvToMDBase.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

// TEST HELPER
class ConvToMDBaseTestHelper : public ConvToMDBase
{
  size_t conversionChunk(size_t job_ID){return 0;}
public:
  void runConversion(API::Progress *){};
  int getNumThreads(){return m_NumThreads;}

};

// The test
class ConvToMDBaseTest : public CxxTest::TestSuite
{
    // Matrix workspace description;
     MDWSDescription WSD;
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
private: 
  ConvToMDBaseTest()
  {
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
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
  }

};


#endif