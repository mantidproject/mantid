#ifndef INTERPOLATINGREBINTEST_H_
#define INTERPOLATINGREBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/InterpolatingRebin.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class InterpolatingRebinTest : public CxxTest::TestSuite
{
public:
  void testworkspace_dist()
  {
    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->isDistribution(true);
    AnalysisDataService::Instance().add("InterpolatingRebinTest_indist", test_in1D);

    InterpolatingRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","InterpolatingRebinTest_indist");
    rebin.setPropertyValue("OutputWorkspace","InterpolatingRebinTest_outdist");
    // Check it fails if "Params" property not set
    TS_ASSERT_THROWS( rebin.execute(), std::runtime_error )
    TS_ASSERT( ! rebin.isExecuted() )
    
    // some of the new bins would are too high to calculate, check it aborts
    rebin.setPropertyValue("Params", "1,1,35");
    TS_ASSERT( ! rebin.execute() )
    TS_ASSERT( ! rebin.isExecuted() )

    // some of the new bins would are too low to calculate, check it aborts
    rebin.setPropertyValue("Params", "0,0.001,15");
    TS_ASSERT( ! rebin.execute() )
    TS_ASSERT( ! rebin.isExecuted() )

    // set the new bins to be less than half the size of the old, one in every 2 old bins and one in every 5 old will coinside
    rebin.setPropertyValue("Params", "2.225,0.2,15");
    TS_ASSERT(rebin.execute())
    TS_ASSERT(rebin.isExecuted())

    MatrixWorkspace_sptr rebindata =
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("InterpolatingRebinTest_outdist"));
    const Mantid::MantidVec outX=rebindata->dataX(0);
    const Mantid::MantidVec outY=rebindata->dataY(0);
    const Mantid::MantidVec outE=rebindata->dataE(0);

    //this intepolated data was found by running the debugger on this test
    TS_ASSERT_DELTA(outX[0], 2.225, 0.00001);
    TS_ASSERT_DELTA(outY[0], 3.9, 0.0001);
    TS_ASSERT_DELTA(outE[0], 0.4875, 0.0001);

    //another output point between input points
    TS_ASSERT_DELTA(outX[7], 3.625,0.00001);
    TS_ASSERT_DELTA(outY[7], 6.7,0.0001);
    TS_ASSERT_DELTA(outE[7], 0.8375, 0.0001);

    //it is set up with the 49th output point being the same as the 15th input
    TS_ASSERT_DELTA(outX[49], 12.025, 0.00001);
    TS_ASSERT_DELTA(outY[49], (15*1.5)+1, 0.0001);
    TS_ASSERT_DELTA(outE[49], (15*1.5+1)/8.0, 0.0001);

    //the data is monotomically increasing and so the next out point should have higher values than the previous but no as high as the next input data point
    TS_ASSERT( outY[50] > (15*1.5)+1);
    TS_ASSERT( outY[50] < (16*1.5)+1);
    //errors -same thing
    TS_ASSERT( outE[50] > (15*1.5+1)/8.0 );
    TS_ASSERT( outE[50] < (16*1.5+1)/8.0 );

    //check the last point
    TS_ASSERT_DELTA(outX[64], 15, 0.00001);
    TS_ASSERT_DELTA(outY[63], 29.0749, 0.0001);
    TS_ASSERT_DELTA(outE[63], 3.6343,0.0001);

    TS_ASSERT( rebindata->isDistribution() );
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_indist");
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_outdist");
  }

  void testworkspace_nondist()
  {

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->isDistribution(false);
    AnalysisDataService::Instance().add("InterpolatingRebinTest_in_nondist", test_in1D);

    InterpolatingRebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","InterpolatingRebinTest_in_nondist");
    rebin.setPropertyValue("OutputWorkspace","InterpolatingRebinTest_out_nondist");

    // set the new bins to be less than half the size of the old, one in every 2 old bins and one in every 5 old will coinside
    rebin.setPropertyValue("Params", "2.225,0.2,15");
    TS_ASSERT(rebin.execute())
    TS_ASSERT(rebin.isExecuted())

    MatrixWorkspace_sptr rebindata =
      boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("InterpolatingRebinTest_out_nondist"));
    const Mantid::MantidVec outX=rebindata->dataX(0);
    const Mantid::MantidVec outY=rebindata->dataY(0);
    const Mantid::MantidVec outE=rebindata->dataE(0);

    //this intepolated data was found by running the debugger on this test
    TS_ASSERT_DELTA(outX[0], 2.225, 0.00001);
    TS_ASSERT_DELTA(outY[0], 1.0400, 0.0001);
    TS_ASSERT_DELTA(outE[0], 0.1300, 0.0001);

    ////another output point between input points
    TS_ASSERT_DELTA(outX[7], 3.625,0.00001);
    TS_ASSERT_DELTA(outY[7], 1.7866,0.0001);
    TS_ASSERT_DELTA(outE[7], 0.2233, 0.0001);

    //it is set up with the 49th output point being the same as the 15th input
    TS_ASSERT_DELTA(outX[49], 12.025, 0.00001);
    double origY = (15*1.5)+1;
    double nondistY = origY/0.75;
    double interpY = nondistY*0.2;
    TS_ASSERT_DELTA(outY[49], interpY, 0.0001);
    TS_ASSERT_DELTA(outE[49], interpY/8.0, 0.0001);

    //the data is monotomically increasing and so the next out point should have higher values than the previous
    TS_ASSERT( outY[50] > interpY);
    //same with the error
    TS_ASSERT( outE[50] < (16*1.5+1)/8.0 );

    //check the last point
    TS_ASSERT_DELTA(outX[64], 15, 0.00001);
    TS_ASSERT_DELTA(outY[63], 6.7841, 0.0001);
    TS_ASSERT_DELTA(outE[63], 0.8480,0.0001);

    //the distribution state of the output workspace should match that of the input
    TS_ASSERT( ! rebindata->isDistribution() );
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_in_nondist");
    AnalysisDataService::Instance().remove("InterpolatingRebinTest_out_nondist");
  }

private:
  Workspace2D_sptr Create1DWorkspace(int size)
  {
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(1,size,size-1);
    double j=1.0;
    int i = 0;
    for ( ; i<size-1; i++)
    {
      retVal->dataX(0)[i]=j*0.5;
      retVal->dataY(0)[i] = j;
      retVal->dataE(0)[i] = retVal->dataY(0)[i]/8;
      j+=1.5;
    }
    retVal->dataX(0)[i]=j*0.5;
    return retVal;
  }

};
#endif /* INTERPOLATINGREBINTEST */
