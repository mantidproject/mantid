#ifndef MANTID_ALGORITHMS_FITPEAKTEST_H_
#define MANTID_ALGORITHMS_FITPEAKTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FitPeak.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::Algorithms::FitPeak;

using namespace Mantid::API;
using namespace Mantid::DataObjects;

class FitPeakTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitPeakTest *createSuite() { return new FitPeakTest(); }
  static void destroySuite( FitPeakTest *suite ) { delete suite; }


  void test_FitPeakWithHighBkgd()
  {
    // Generate data
    MatrixWorkspace_sptr dataws = gen_4866P5Data();

  }



  /** Generate a workspace contains PG3_4866 5-th peak
    */
  MatrixWorkspace_sptr gen_4866P5Data()
  {


  }


};


#endif /* MANTID_ALGORITHMS_FITPEAKTEST_H_ */
