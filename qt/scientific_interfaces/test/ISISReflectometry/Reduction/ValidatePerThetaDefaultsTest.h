#ifndef MANTID_CUSTOMINTERFACES_VALIDATEPERTHETADEFAULTSTEST_H_
#define MANTID_CUSTOMINTERFACES_VALIDATEPERTHETADEFAULTSTEST_H_
#include <cxxtest/TestSuite.h>
#include "../../../ISISReflectometry/Reduction/ValidatePerThetaDefaults.h"

using namespace MantidQt::CustomInterfaces;

class ValidatePerThetaDefaultsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ValidatePerThetaDefaultsTest *createSuite() { return new ValidatePerThetaDefaultsTest(); }
  static void destroySuite(ValidatePerThetaDefaultsTest *suite) { delete suite; }

  static auto constexpr TOLERANCE = 0.000001;

  void testThing() {}

};
#endif // MANTID_CUSTOMINTERFACES_VALIDATEPERTHETADEFAULTSTEST_H_
