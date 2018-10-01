#ifndef MANTID_CUSTOMINTERFACES_PERTHETADEFAULTSTABLEVALIDATORTEST_H_
#define MANTID_CUSTOMINTERFACES_PERTHETADEFAULTSTABLEVALIDATORTEST_H_
#include "../../../ISISReflectometry/GUI/Experiment/PerThetaDefaultsTableValidator.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

class PerThetaDefaultsTableValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PerThetaDefaultsTableValidatorTest *createSuite() {
    return new PerThetaDefaultsTableValidatorTest();
  }
  static void destroySuite(PerThetaDefaultsTableValidatorTest *suite) {
    delete suite;
  }
  static auto constexpr TOLERANCE = 0.000001;

  // TODO
  void testThing() {}
};
#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAULTSTABLEVALIDATORTEST_H_
