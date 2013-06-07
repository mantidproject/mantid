#ifndef MANTID_MDALGORITHMS_THRESHOLDMDTEST_H_
#define MANTID_MDALGORITHMS_THRESHOLDMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ThresholdMD.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using Mantid::MDAlgorithms::ThresholdMD;

class ThresholdMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThresholdMDTest *createSuite() { return new ThresholdMDTest(); }
  static void destroySuite( ThresholdMDTest *suite ) { delete suite; }


  MDHistoWorkspace_sptr createInputWorkspace(signal_t signal, signal_t errorSQ=0, const int nBins=10)
  {
    MDHistoDimension_sptr dim = boost::make_shared<MDHistoDimension>("X", "X", "", 0, 10, nBins);
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
  

  void test_exec()
  {
    const std::string outWSName = "OutWS";

    ThresholdMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", createInputWorkspace(1));
    alg.setProperty("CurrentValue", 3.0);
    alg.setPropertyValue("OutputWorkspace", outWSName);
  }
  


};


#endif /* MANTID_MDALGORITHMS_THRESHOLDMDTEST_H_ */
