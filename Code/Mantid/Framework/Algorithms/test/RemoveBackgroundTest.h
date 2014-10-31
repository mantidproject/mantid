#ifndef REMOVE_BACKGROUD_TEST_H_
#define REMOVE_BACKGROUD_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/RemoveBackground.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CalculateFlatBackground.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
using namespace Mantid;

class RemoveBackgroundTest : public CxxTest::TestSuite
{
public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RemoveBackgroundTest *createSuite() { return new RemoveBackgroundTest(); }
  static void destroySuite( RemoveBackgroundTest *suite ) { delete suite; }

  RemoveBackgroundTest()
  {

    DataObjects::Workspace2D_sptr theWS  = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 15000);

    std::string wsName = theWS->getName();
    if(wsName.empty())
    {
      wsName = "sourceWS";
    }
    // Add incident energy necessary for unit conversion
    theWS->mutableRun().addProperty("Ei",13.,"meV",true);

    API::AnalysisDataService::Instance().addOrReplace(wsName,theWS);

    Algorithms::Rebin rebinner;
    rebinner.initialize();
    rebinner.setPropertyValue("InputWorkspace",theWS->getName());
    rebinner.setPropertyValue("OutputWorkspace","Background");
    rebinner.setPropertyValue("Params","10000,5000,15000");

    rebinner.execute();

    BgWS =  API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("Background");

    Algorithms::ConvertUnits unitsConv;
    unitsConv.initialize();
    unitsConv.setPropertyValue("InputWorkspace",theWS->getName());
    unitsConv.setPropertyValue("OutputWorkspace","sourceWSdE");
    unitsConv.setPropertyValue("Target","DeltaE");
    unitsConv.setPropertyValue("EMode","Direct");

    unitsConv.execute();

    Algorithms::CalculateFlatBackground bgRemoval;

    bgRemoval.initialize();
    bgRemoval.setPropertyValue("InputWorkspace",theWS->getName());
    bgRemoval.setPropertyValue("OutputWorkspace",theWS->getName());
    bgRemoval.setPropertyValue("StartX","10000");
    bgRemoval.setPropertyValue("EndX","15000");
    bgRemoval.setPropertyValue("Mode","Mean");

    bgRemoval.execute();

    unitsConv.setPropertyValue("InputWorkspace",theWS->getName());
    unitsConv.setPropertyValue("OutputWorkspace","sampleWSdE");
    unitsConv.setPropertyValue("Target","DeltaE");
    unitsConv.setPropertyValue("EMode","Direct");

    unitsConv.execute();

    SourceWS =  API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("sourceWSdE");


  }

  ~RemoveBackgroundTest()
  {
    BgWS.reset();
    SourceWS.reset();
  }
  void testWrongInit()
  {
     Algorithms::BackgroundHelper bgRemoval;
     // create workspace with units of energy transfer
     auto bkgWS = WorkspaceCreationHelper::createProcessedInelasticWS(std::vector<double>(1,1.), std::vector<double>(1,20.), std::vector<double>(1,10.));
     TSM_ASSERT_THROWS("Should throw if background workspace is not in TOF units",bgRemoval.initialize(bkgWS,SourceWS,0),std::invalid_argument);


     bkgWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 15);
     TSM_ASSERT_THROWS("Should throw if background is not 1 or equal to source",bgRemoval.initialize(bkgWS,SourceWS,0),std::invalid_argument);

     auto sourceWS = WorkspaceCreationHelper::Create2DWorkspace(5,10);
     TSM_ASSERT_THROWS("Should throw if source workspace does not have units",bgRemoval.initialize(BgWS,sourceWS,0),std::invalid_argument);

     sourceWS ->getAxis(0)->setUnit("TOF");
     TSM_ASSERT_THROWS("Should throw if source workspace does not have proper instrument",bgRemoval.initialize(BgWS,sourceWS,0),std::invalid_argument);
  }

  
  void testBackgroundHelper()
  {
    Algorithms::BackgroundHelper bgRemoval;

    int emode = static_cast<int>(Kernel::DeltaEMode().fromString("Direct"));
    bgRemoval.initialize(BgWS,SourceWS,emode);

    MantidVec& dataX = SourceWS->dataX(0);
    MantidVec& dataY = SourceWS->dataY(0);
    MantidVec& dataE = SourceWS->dataE(0);

    bgRemoval.removeBackground(0,dataX,dataY,dataE);

    auto SampleWS =  API::AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("sampleWSdE");

    const MantidVec & sampleX = SampleWS->readX(0);
    const MantidVec & sampleY = SampleWS->readY(0);
    //const MantidVec & sampleE = SampleWS->readE(0);
    for(size_t i=0;i<sampleY.size();i++)
    {
      TS_ASSERT_DELTA(dataX[i],sampleX[i],1.e-7);
      TS_ASSERT_DELTA(dataY[i],sampleY[i],1.e-7);
    }

  }

private:
  API::MatrixWorkspace_sptr BgWS;   
  API::MatrixWorkspace_sptr SourceWS;
 
};

 

#endif /*ALGORITHMTEST_H_*/
