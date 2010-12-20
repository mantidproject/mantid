#include "MantidAlgorithms/ConvertToPointData.h"
#include <cxxtest/TestSuite.h>

#include "WorkspaceCreationHelper.hh"

using Mantid::API::IAlgorithm_sptr;
using Mantid::Algorithms::ConvertToPointData;
using Mantid::DataObjects::Workspace2D_sptr;

class ConvertToPointDataTest : public CxxTest::TestSuite
{

public:

  void test_That_The_Algorithm_Has_Two_Properties()
  {
    IAlgorithm_sptr alg = createAlgorithm();
    TS_ASSERT_EQUALS(alg->getProperties().size(), 2);
  }

  void test_That_A_Workspace_Containing_Non_Histogram_Data_Is_Not_Accepted()
  {
    IAlgorithm_sptr alg = createAlgorithm();
    // False indicates a non histogram workspace
    Workspace2D_sptr testWS = WorkspaceCreationHelper::Create2DWorkspace123(10,5,false);
    TS_ASSERT_THROWS(alg->setProperty("InputWorkspace", testWS),std::invalid_argument);
  }

  void test_A_Uniformly_Binned_Histogram_Is_Transformed_Correctly()
  {
    IAlgorithm_sptr alg = createAlgorithm();
    // Creates a workspace with 2 spectra, 10 bins with bin width 1.0 starting from 0.0
    const int numBins(10);
    const int numSpectra(9);
    Workspace2D_sptr testWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(numSpectra, numBins);

    TS_ASSERT_EQUALS(testWS->isHistogramData(), true);
    TS_ASSERT_THROWS_NOTHING(alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    const std::string outputName = "uniform_bins";
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", outputName));
    
    alg->setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(outputName));

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

    Mantid::API::AnalysisDataService::Instance().remove(outputName);
  }

private:

  IAlgorithm_sptr createAlgorithm()
  {
    boost::shared_ptr<ConvertToPointData> converter(new ConvertToPointData());
    converter->initialize();
    return converter;
  }

};
