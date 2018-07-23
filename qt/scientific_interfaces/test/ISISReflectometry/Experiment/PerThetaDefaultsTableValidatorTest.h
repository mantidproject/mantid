#ifndef MANTID_CUSTOMINTERFACES_VALIDATEPERTHETADEFAULTSTABLETEST_H_
#define MANTID_CUSTOMINTERFACES_VALIDATEPERTHETADEFAULTSTABLETEST_H_
#include <cxxtest/TestSuite.h>
#include "../../../ISISReflectometry/GUI/Experiment/PerThetaDefaultsTableValidator.h"

using namespace MantidQt::CustomInterfaces;

class ValidatePerThetaDefaultsTableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ValidatePerThetaDefaultsTableTest *createSuite() { return new ValidatePerThetaDefaultsTableTest(); }
  static void destroySuite(ValidatePerThetaDefaultsTableTest *suite) { delete suite; }
  static auto constexpr TOLERANCE = 0.000001;

  void testThing() {
    
  }

};
#endif // MANTID_CUSTOMINTERFACES_VALIDATEPERTHETADEFAULTSTABLETEST_H_
