#ifndef MANTID_MDALGORITHMS_THRESHOLDMDTEST_H_
#define MANTID_MDALGORITHMS_THRESHOLDMDTEST_H_

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDAlgorithms/ThresholdMD.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;

using Mantid::coord_t;
using Mantid::signal_t;

class ThresholdMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThresholdMDTest *createSuite() { return new ThresholdMDTest(); }
  static void destroySuite( ThresholdMDTest *suite ) { delete suite; }


  MDHistoWorkspace_sptr createInputWorkspace(signal_t signal, signal_t errorSQ=0, const int nBins=1)
  {
    MDHistoDimension_sptr dim = boost::make_shared<MDHistoDimension>("X", "X", "", static_cast<coord_t>(0), static_cast<coord_t>(10), static_cast<size_t>(nBins));
    MDHistoWorkspace_sptr histo = boost::make_shared<MDHistoWorkspace>(dim);
    signal_t* signals = histo->getSignalArray();
    signal_t* errorsSQ = histo->getErrorSquaredArray();
    for (int i = 0; i < nBins; ++i)
    {
      signals[i] = signal;
      errorsSQ[i] = errorSQ;
    }
    return histo;
  }

  void test_Init()
  {
    ThresholdMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  IMDHistoWorkspace_sptr doExecute(IMDHistoWorkspace_sptr inWS, const std::string& condition, const double& referenceValue)
  {
    const std::string outWSName = "OutWS";
    ThresholdMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("Condition", condition);
    alg.setProperty("ReferenceValue", referenceValue);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName);
    return outWS;
  }


  void test_below_lower()
  {
    const int nBins = 2;
    auto inWS = createInputWorkspace(1, 1, nBins); // Signal on input = 1;
    inWS->setSignalAt(1, 3); // Signal values are now [1, 3] in this 1D workspace
    auto outWS = doExecute(inWS, "Less Than", 3); // Overwrite those less than 3 with 0.

    TS_ASSERT_EQUALS(inWS->getNPoints(), outWS->getNPoints());
    TSM_ASSERT_EQUALS("Overwrite the first entry", 0, outWS->getSignalAt(0)); // Overwrite the first entry.
    TSM_ASSERT_EQUALS("Do Not overwrite the second entry", 3, outWS->getSignalAt(1)); // Do NOT overwrite the second entry.
    TSM_ASSERT_EQUALS("Do not touch the errors", inWS->getErrorAt(0), outWS->getErrorAt(0)); // Don't touch these.
  }
  
  void test_above_upper()
  {
    const int nBins = 2;
    auto inWS = createInputWorkspace(1, 1, nBins); // Signal on input = 1;
    inWS->setSignalAt(1, 2); // Signal values are now [1, 2] in this 1D workspace
    auto outWS = doExecute(inWS, "Greater Than", 1); // Overwrite those greater than 1 with 0.

    TS_ASSERT_EQUALS(inWS->getNPoints(), outWS->getNPoints());
    TSM_ASSERT_EQUALS("Do not overwrite the first entry", 1, outWS->getSignalAt(0));
    TSM_ASSERT_EQUALS("Overwrite the second entry", 0, outWS->getSignalAt(1));
    TSM_ASSERT_EQUALS("Do not touch the errors", inWS->getErrorAt(0), outWS->getErrorAt(0)); // Don't touch these.
  }


  void test_custom_overwrite()
  {
    const std::string outWSName = "OutWS";

    const int nBins = 2;
    auto inWS = createInputWorkspace(1, 1, nBins); // Signal on input = 1;
    inWS->setSignalAt(1, 3); // Signal values are now [1, 3] in this 1D workspace

    ThresholdMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("Condition", "Less Than");
    alg.setProperty("ReferenceValue", 3.0);
    alg.setProperty("OverwriteWithZero", false);
    alg.setProperty("CustomOverwriteValue", 9.0);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName);
    TS_ASSERT_EQUALS(inWS->getNPoints(), outWS->getNPoints());
    TSM_ASSERT_EQUALS("Overwrite the first entry with the custom overwrite value.", 9,
        outWS->getSignalAt(0));
    // Overwrite the first entry.
    TSM_ASSERT_EQUALS("Do Not overwrite the second entry", 3, outWS->getSignalAt(1));
    // Do NOT overwrite the second entry.
    TSM_ASSERT_EQUALS("Do not touch the errors", inWS->getErrorAt(0), outWS->getErrorAt(0));
    // Don't touch these.
}



};


#endif /* MANTID_MDALGORITHMS_THRESHOLDMDTEST_H_ */
