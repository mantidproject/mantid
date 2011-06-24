#ifndef MANTID_ALGORITHMS_REBIN2DTEST_H_
#define MANTID_ALGORITHMS_REBIN2DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidKernel/Timer.h"

using Mantid::Algorithms::Rebin2D;
using namespace Mantid::API;

namespace
{
  /**
   * Shared methods between the unit test and performance test
   */

  /// Return the input workspace. All Y values are 2 and E values sqrt(2)
  MatrixWorkspace_sptr makeInputWS(const bool large = false)
  {
    size_t nhist(0), nbins(0);
    double x0(0.0), deltax(0.0);

    if( large )
    {
      nhist = 200;
      nbins = 200;
      x0 = 100.;
      deltax = 200.;
    }
    else
    {
      nhist = 10;
      nbins = 10;
      x0 = 5.0;
      deltax = 1.0;
    }

    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(int(nhist), int(nbins), x0, deltax);

    // We need something other than a spectrum axis, call this one theta
    NumericAxis* const thetaAxis = new NumericAxis(nhist);
    for(size_t i = 0; i < nhist; ++i)
    {
      thetaAxis->setValue(i, static_cast<double>(i));
    }
    ws->replaceAxis(1, thetaAxis);
    return ws;
  }

  MatrixWorkspace_sptr runAlgorithm(MatrixWorkspace_sptr inputWS,
                                    const std::string & axis1Params, 
                                    const std::string & axis2Params)
  {
    // Name of the output workspace.
    std::string outWSName("Rebin2DTest_OutputWS");
    
    Rebin2D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis1Binning", axis1Params) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Axis2Binning", axis2Params) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(outWSName));
    TS_ASSERT(outputWS);
    return outputWS;
  }

}

class Rebin2DTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    Rebin2D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_Rebin2D_With_Axis2_Unchanged()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS(); //10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS = runAlgorithm(inputWS, "5.,2.,15.", "-0.5,1,9.5");

    // Check values
    const size_t nxvalues(6), nhist(10);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nhist);
    // Axis sizes
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->length(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->length(), nhist);
    TS_ASSERT_EQUALS(outputWS->readX(0).size(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->readY(0).size(), nxvalues - 1);

    Mantid::API::Axis *newYAxis = outputWS->getAxis(1);
    for(size_t i = 0; i < nhist; ++i)
    {
      for(size_t j = 0; j < nxvalues - 1; ++j)
      {
        const double expected(5.0 + 2.0*static_cast<double>(j));
        TS_ASSERT_DELTA(outputWS->readX(i)[j], expected, DBL_EPSILON);
        TS_ASSERT_DELTA(outputWS->readY(i)[j], 4.0, DBL_EPSILON);
        TS_ASSERT_DELTA(outputWS->readE(i)[j], 2.0, DBL_EPSILON);
      }    
      // Final X boundary
      TS_ASSERT_DELTA(outputWS->readX(i)[nxvalues-1], 15.0, DBL_EPSILON);
      // The new Y axis value should be the centre point bin values
      TS_ASSERT_DELTA((*newYAxis)(i), static_cast<double>(i), DBL_EPSILON);
    }

    // Clean up
    AnalysisDataService::Instance().remove(outputWS->getName());
  }

  void test_Rebin2D_With_Axis1_Unchanged()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS(); //10 histograms, 10 bins
    MatrixWorkspace_sptr outputWS = runAlgorithm(inputWS, "5.,1.,15.", "-0.5,2,9.5");
    // Check values
    const size_t nxvalues(11), nhist(5);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), nhist);
    // Axis sizes
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->length(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->length(), nhist);
    TS_ASSERT_EQUALS(outputWS->readX(0).size(), nxvalues);
    TS_ASSERT_EQUALS(outputWS->readY(0).size(), nxvalues - 1);

    Mantid::API::Axis *newYAxis = outputWS->getAxis(1);
    for(size_t i = 0; i < nhist; ++i)
    {
      for(size_t j = 0; j < nxvalues - 1; ++j)
      {
        TS_ASSERT_DELTA(outputWS->readX(i)[j], 5.0 + static_cast<double>(j), DBL_EPSILON);
        TS_ASSERT_DELTA(outputWS->readY(i)[j], 4.0, DBL_EPSILON);
        TS_ASSERT_DELTA(outputWS->readE(i)[j], 2.0, DBL_EPSILON);
      }    
      // Final X boundary
      TS_ASSERT_DELTA(outputWS->readX(i)[nxvalues-1], 15.0, DBL_EPSILON);
      // The new Y axis value should be the centre point bin values
      TS_ASSERT_DELTA((*newYAxis)(i), 0.5 + 2.0*static_cast<double>(i), DBL_EPSILON);
    }
  }
};


//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class Rebin2DTestPerformance : public CxxTest::TestSuite
{
  
public:
  
  void test_On_Large_Workspace()
  {
    MatrixWorkspace_sptr inputWS = makeInputWS();
    runAlgorithm(inputWS, "200,250,40000", "-0.5,5,199.5");
  }
  

};


#endif /* MANTID_ALGORITHMS_REBIN2DTEST_H_ */

