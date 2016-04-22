#ifndef MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_
#define MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Objects/Object.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Algorithms::MCInteractionVolume;

class MCInteractionVolumeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MCInteractionVolumeTest *createSuite() {
    return new MCInteractionVolumeTest();
  }
  static void destroySuite(MCInteractionVolumeTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------
  void test_Construction_With_Invalid_Sample_Shape_Throws_Error() {
    using Mantid::API::Sample;

    Sample sample;
    // nothing
    TS_ASSERT_THROWS(MCInteractionVolume mcv(sample), std::invalid_argument);
    // valid shape
    sample.setShape(*ComponentCreationHelper::createSphere(1));
    TS_ASSERT_THROWS_NOTHING(MCInteractionVolume mcv(sample));
  }
};

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_ */
