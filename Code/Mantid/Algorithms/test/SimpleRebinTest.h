#ifndef SIMPLEREBINTEST_H_
#define SIMPLEREBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/Workspace2D.h" 
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/SimpleRebin.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class SimpleRebinTest : public CxxTest::TestSuite
{
public:
  void testworkspace1D_dist()
  {    
    Workspace1D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);    

    SimpleRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in1D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("params", "1.5,2.0,20,-0.1,30,1.0,35");
    rebin.execute();
    Workspace_sptr rebindata = AnalysisDataService::Instance().retrieve("test_out");
    const std::vector<double> outX=rebindata->dataX(0);
    const std::vector<double> outY=rebindata->dataY(0);
    const std::vector<double> outE=rebindata->dataE(0);

    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7], sqrt(4.5)/2.0  ,0.000001);
    
    TS_ASSERT_DELTA(outX[12],24.2 ,0.000001);
    TS_ASSERT_DELTA(outY[12],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(5.445)/2.42 ,0.000001);

    TS_ASSERT_DELTA(outX[17],32.0  ,0.000001);
    TS_ASSERT_DELTA(outY[17],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(2.25) ,0.000001);

    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");    
  }

  void testworkspace1D_nondist()
  {
    Workspace1D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    SimpleRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in1D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("params", "1.5,2.0,20,-0.1,30,1.0,35");
    rebin.execute();
    Workspace_sptr rebindata = AnalysisDataService::Instance().retrieve("test_out");

    const std::vector<double> outX=rebindata->dataX(0);
    const std::vector<double> outY=rebindata->dataY(0);
    const std::vector<double> outE=rebindata->dataE(0);

    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],8.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(8.0)  ,0.000001);
    TS_ASSERT_DELTA(outX[12],24.2  ,0.000001);
    TS_ASSERT_DELTA(outY[12],9.68 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(9.68)  ,0.000001);
    TS_ASSERT_DELTA(outX[17],32  ,0.000001);
    TS_ASSERT_DELTA(outY[17],4.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(4.0)  ,0.000001);

    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out"); 
  }

  void testworkspace2D_dist()
  {
    Workspace2D_sptr test_in2D = Create2DWorkspace(50,20);
    test_in2D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    SimpleRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in2D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("params", "1.5,2.0,20,-0.1,30,1.0,35");
    rebin.execute();
    Workspace_sptr rebindata = AnalysisDataService::Instance().retrieve("test_out");

    const std::vector<double> outX=rebindata->dataX(5);
    const std::vector<double> outY=rebindata->dataY(5);
    const std::vector<double> outE=rebindata->dataE(5);
    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(4.5)/2.0  ,0.000001);

    TS_ASSERT_DELTA(outX[12],24.2 ,0.000001);
    TS_ASSERT_DELTA(outY[12],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(5.445)/2.42  ,0.000001);

    TS_ASSERT_DELTA(outX[17],32.0  ,0.000001);
    TS_ASSERT_DELTA(outY[17],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(2.25)  ,0.000001);


    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace2D_nondist()
  {
    Workspace2D_sptr test_in2D = Create2DWorkspace(50,20);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    SimpleRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in2D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("params", "1.5,2.0,20,-0.1,30,1.0,35");
    rebin.execute();
    Workspace_sptr rebindata = AnalysisDataService::Instance().retrieve("test_out");
    const std::vector<double> outX=rebindata->dataX(5);
    const std::vector<double> outY=rebindata->dataY(5);
    const std::vector<double> outE=rebindata->dataE(5);
    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],8.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(8.0)  ,0.000001);
    TS_ASSERT_DELTA(outX[12],24.2  ,0.000001);
    TS_ASSERT_DELTA(outY[12],9.68 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(9.68)  ,0.000001);
    TS_ASSERT_DELTA(outX[17],32  ,0.000001);
    TS_ASSERT_DELTA(outY[17],4.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(4.0)  ,0.000001);

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

private:
  Workspace1D_sptr Create1DWorkspace(int size)
  {
    std::vector<double> x1(size,0.0),y1(size-1,3.0),e1(size-1,sqrt(3.0)),e2(size-1,0);
    Workspace1D_sptr retVal(new Workspace1D);
    double j=1.0;
    for (int i=0; i<size; i++)    
    {
      x1[i]=j*0.5;
      j+=1.5;
    }
    retVal->setX(x1);
    retVal->setData(y1,e1,e2);
    return retVal;
  }
  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen)
  {
    std::vector<double> x1(xlen,0.0),y1(xlen-1,3.0),e1(xlen-1,sqrt(3.0)),e2(xlen-1,0);

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,xlen,xlen-1);
    double j=1.0;

    for (int i=0; i<xlen; i++)    
    {
      x1[i]=j*0.5;
      j+=1.5;
    }

    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);     
      retVal->setData(i,y1,e1,e2);
    }

    return retVal;
  }

};
#endif /* SIMPLEREBINTEST */
