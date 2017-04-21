#ifndef CONVERTTOHISTOGRAMTEST_H_
#define CONVERTTOHISTOGRAMTEST_H_

#include "MantidHistogramData/LinearGenerator.h"
#include "MantidAlgorithms/ConvertToHistogram.h"
#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Algorithms::ConvertToHistogram;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::MantidVecPtr;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;

class ConvertToHistogramTest : public CxxTest::TestSuite {

public:
  void test_That_The_Algorithm_Has_Two_Properties() {
    ConvertToHistogram alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_EQUALS(alg.getProperties().size(), 2);
  }

  void
  test_That_Output_Is_The_Same_As_Input_If_Input_Contains_Histogram_Data() {
    // True indicates a non histogram workspace
    Workspace2D_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspace123(5, 10, true);

    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    // Check that the algorithm just pointed the output data at the input
    TS_ASSERT_EQUALS(&(*testWS), &(*outputWS));
    Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
  }

  void test_A_Point_Data_InputWorkspace_Is_Converted_To_A_Histogram() {
    // Creates a workspace with 10 points
    const int numYPoints(10);
    const int numSpectra(2);
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspace123(
        numSpectra, numYPoints, false);
    // Reset the X data to something reasonable
    Points x(numYPoints, LinearGenerator(0.0, 1.0));
    for (int i = 0; i < numSpectra; ++i) {
      testWS->setPoints(i, x);
    }

    TS_ASSERT_EQUALS(testWS->isHistogramData(), false);

    MatrixWorkspace_sptr outputWS = runAlgorithm(testWS);
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    TS_ASSERT_EQUALS(outputWS->isHistogramData(), true);
    const int numBoundaries = numYPoints + 1;

    // This makes the new X values more readable rather than using a dynamic
    // array
    // so I'll live with the hard coding
    const double expectedX[11] = {-0.5, 0.5, 1.5, 2.5, 3.5, 4.5,
                                  5.5,  6.5, 7.5, 8.5, 9.5};
    for (int j = 0; j < numBoundaries; ++j) {
      TS_ASSERT_EQUALS(outputWS->readX(0)[j], expectedX[j]);
    }

    // for( int i = 0; i < numSpectra; ++i )
    // {
    //   const Mantid::MantidVec & yValues = outputWS->readY(i);
    //   const Mantid::MantidVec & xValues = outputWS->readX(i);
    //   const Mantid::MantidVec & eValues = outputWS->readE(i);

    //   TS_ASSERT_EQUALS(xValues.size(), numBins);
    //   // The y and e values sizes be unchanged
    //   TS_ASSERT_EQUALS(yValues.size(), numYPoints);
    //   TS_ASSERT_EQUALS(eValues.size(), numYPoints);

    //   for( int j = 0; j < numYPoints; ++j )
    //   {
    // 	// Now the data. Y and E unchanged
    // 	TS_ASSERT_EQUALS(yValues[j], 2.0);
    // 	TS_ASSERT_EQUALS(eValues[j], M_SQRT2);

    // 	// X data originally was 0->10 in steps of 1. Now it should be the
    // centre of each bin which is
    // 	// 1.0 away from the last centre
    // 	const double expectedX = 0.5 + j*1.0;
    // 	TS_ASSERT_EQUALS(xValues[j], expectedX);
    //   }
    //   // And the final X points
    //   TS_ASSERT_EQUALS(xValues.back(), 100.);
    // }

    Mantid::API::AnalysisDataService::Instance().remove(outputWS->getName());
  }

private:
  MatrixWorkspace_sptr runAlgorithm(Workspace2D_sptr inputWS) {
    IAlgorithm_sptr alg(new ConvertToHistogram());
    alg->initialize();
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS));
    const std::string outputName = "__algOut";
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", outputName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    MatrixWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputName));

    return outputWS;
  }
};

class ConvertToHistogramTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToHistogramTestPerformance *createSuite() {
    return new ConvertToHistogramTestPerformance();
  }

  static void destroySuite(ConvertToHistogramTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    inputWS =
        WorkspaceCreationHelper::create2DWorkspace123(20000, 10000, false);
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("output");
  }

  void testPerformanceWS() {
    ConvertToHistogram cth;
    cth.initialize();
    cth.setProperty("InputWorkspace", inputWS);
    cth.setProperty("OutputWorkspace", "output");
    cth.execute();
  }

private:
  Workspace2D_sptr inputWS;
};

#endif // CONVERTTOHISTOGRAMTEST_H_
