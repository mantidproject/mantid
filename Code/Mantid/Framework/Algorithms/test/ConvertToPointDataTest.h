#ifndef CONVERTTOPONTDATATEST_H_
#define CONVERTTOPONTDATATEST_H_

#include "MantidAlgorithms/ConvertToPointData.h"
#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ConvertToPointData;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;

class ConvertToPointDataTest : public CxxTest::TestSuite
{

public:

  void test_That_The_Algorithm_Has_Two_Properties()
  {
    ConvertToPointData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_EQUALS(alg.getProperties().size(), 2);
  }

  void test_That_Output_Is_The_Same_As_Input_If_Input_Contains_Point_Data()
  {
    // False indicates a non histogram workspace
    Workspace2D_sptr testWS = WorkspaceCreationHelper::Create2DWorkspace123(5,10,false);

    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);
    TS_ASSERT(outputWS);
    if( !outputWS ) return; 

    // Check that the algorithm just pointed the output data at the input
    TS_ASSERT_EQUALS(&(*testWS), &(*outputWS));
    Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
  }

  void test_A_Uniformly_Binned_Histogram_Is_Transformed_Correctly()
  {

    // Creates a workspace with 2 spectra, 10 bins with bin width 1.0 starting from 0.0
    const int numBins(10);
    const int numSpectra(2);
    Workspace2D_sptr testWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(numSpectra, numBins);
    TS_ASSERT_EQUALS(testWS->isHistogramData(), true);

    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);

    TS_ASSERT(outputWS);
    if( !outputWS ) return; 

    TS_ASSERT_EQUALS(outputWS->isHistogramData(), false);
    for( int i = 0; i < numSpectra; ++i )
    {
      const Mantid::MantidVec & yValues = outputWS->readY(i); 
      const Mantid::MantidVec & xValues = outputWS->readX(i);
      const Mantid::MantidVec & eValues = outputWS->readE(i);

      // The X size should be now equal to the number of bins
      TS_ASSERT_EQUALS(xValues.size(), numBins);
      // The y and e values sizes be unchanged
      TS_ASSERT_EQUALS(yValues.size(), numBins);
      TS_ASSERT_EQUALS(eValues.size(), numBins);

      for( int j = 0; j < numBins; ++j )
      {
	// Now the data. Y and E unchanged
	TS_ASSERT_EQUALS(yValues[j], 2.0);
	TS_ASSERT_EQUALS(eValues[j], sqrt(2.0));
	// X data originally was 0->10 in steps of 1. Now it should be the centre of each bin which is
	// 1.0 away from the last centre
	const double expectedX = 0.5 + j*1.0;
	TS_ASSERT_EQUALS(xValues[j], expectedX);
      }
    }

    Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
  }

  void test_A_Non_Uniformly_Binned_Histogram_Is_Transformed_Correctly()
  {
    // Creates a workspace with 2 spectra, and the given bin structure
    double xBoundaries[11] = {0.0,1.0,3.0,5.0,6.0,7.0,10.0,13.0,16.0,17.0,17.5};
    const int numSpectra(2);
    Workspace2D_sptr testWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(numSpectra, 11, xBoundaries);
    const int numBins = testWS->blocksize();
    TS_ASSERT_EQUALS(testWS->isHistogramData(), true);
    
    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);

    TS_ASSERT(outputWS);
    if( !outputWS ) return; 

    TS_ASSERT_EQUALS(outputWS->isHistogramData(), false);
    for( int i = 0; i < numSpectra; ++i )
    {
      const Mantid::MantidVec & yValues = outputWS->readY(i); 
      const Mantid::MantidVec & xValues = outputWS->readX(i);
      const Mantid::MantidVec & eValues = outputWS->readE(i);

      // The X size should be now equal to the number of bins
      TS_ASSERT_EQUALS(xValues.size(), numBins);
      // The y and e values sizes be unchanged
      TS_ASSERT_EQUALS(yValues.size(), numBins);
      TS_ASSERT_EQUALS(eValues.size(), numBins);

      for( int j = 0; j < numBins; ++j )
      {
	// Now the data. Y and E unchanged
	TS_ASSERT_EQUALS(yValues[j], 2.0);
	TS_ASSERT_EQUALS(eValues[j], sqrt(2.0));
	// X data originally was 0->10 in steps of 1. Now it should be the centre of each bin which is
	// 1.0 away from the last centre
	const double expectedX = 0.5*(xBoundaries[j] + xBoundaries[j+1]);
	TS_ASSERT_EQUALS(xValues[j], expectedX);
      }
    }

  }

private:

  MatrixWorkspace_sptr runAlgorithm(Workspace2D_sptr inputWS)
  {
    IAlgorithm_sptr alg(new ConvertToPointData());
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS));
    const std::string outputName = "__algOut";
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", outputName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(outputName));

    return outputWS;
  }

};

#endif // CONVERTTOPONTDATATEST_H_
