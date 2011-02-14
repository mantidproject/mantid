#ifndef COMPRESSEDREBINTEST_H_
#define COMPRESSEDREBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/CompressedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class CompressedRebinTest : public CxxTest::TestSuite
{
public:

  void testworkspace2D_dist()
  {
    Workspace2D_sptr test_in2D = Create2DWorkspaceForCompressedRebin(50,20);
    test_in2D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in2D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute())
    TS_ASSERT(rebin.isExecuted())
    MatrixWorkspace_sptr rebindata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));

    const Mantid::MantidVec outX=rebindata->dataX(5);
    const Mantid::MantidVec outY=rebindata->dataY(5);
    const Mantid::MantidVec outE=rebindata->dataE(5);
    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(4.5)/2.0  ,0.000001);

    TS_ASSERT_DELTA(outX[12],24.2 ,0.000001);
    TS_ASSERT_DELTA(outY[12],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(5.445)/2.42  ,0.000001);

    TS_ASSERT_DELTA(outX[17],32.0  ,0.000001);
    TS_ASSERT_DELTA(outY[17],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(2.25)  ,0.000001);
    bool dist=rebindata->isDistribution();
    TS_ASSERT(dist);

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace2D_nondist()
  {
    Workspace2D_sptr test_in2D = Create2DWorkspaceForCompressedRebin(50,20);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    // Mask a couple of bins for a test
    test_in2D->maskBin(10,4);
    test_in2D->maskBin(10,5);
    
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in2D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute())
    TS_ASSERT(rebin.isExecuted())
    MatrixWorkspace_sptr rebindata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));
    const Mantid::MantidVec outX=rebindata->dataX(5);
    const Mantid::MantidVec outY=rebindata->dataY(5);
    const Mantid::MantidVec outE=rebindata->dataE(5);
    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],8.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(8.0)  ,0.000001);
    TS_ASSERT_DELTA(outX[12],24.2  ,0.000001);
    TS_ASSERT_DELTA(outY[12],9.68 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(9.68)  ,0.000001);
    TS_ASSERT_DELTA(outX[17],32  ,0.000001);
    TS_ASSERT_DELTA(outY[17],4.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(4.0)  ,0.000001);
    bool dist=rebindata->isDistribution();
    TS_ASSERT(!dist);

    // Test that the masking was propagated correctly
    TS_ASSERT( test_in2D->hasMaskedBins(10) )
    TS_ASSERT( rebindata->hasMaskedBins(10) )
    TS_ASSERT_THROWS_NOTHING (
      const MatrixWorkspace::MaskList& masks = rebindata->maskedBins(10);
      TS_ASSERT_EQUALS( masks.size(),1 )
      TS_ASSERT_EQUALS( masks.begin()->first, 1 )
      TS_ASSERT_EQUALS( masks.begin()->second, 0.75 )
    )
    
    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

private:

  Workspace2D_sptr Create2DWorkspaceForCompressedRebin(int xlen, int ylen)
  {
    boost::shared_ptr<Mantid::MantidVec> x1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(xlen-1,3.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(xlen-1,sqrt(3.0)));

    Workspace2D_sptr retVal(new CompressedWorkspace2D);
    retVal->initialize(ylen,xlen,xlen-1);
    double j=1.0;

    for (int i=0; i<xlen; i++)
    {
      (*x1)[i]=j*0.5;
      j+=1.5;
    }

    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }

};
#endif /* COMPRESSEDREBINTEST_H_ */
