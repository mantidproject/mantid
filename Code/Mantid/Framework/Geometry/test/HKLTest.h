#ifndef MANTID_GEOMETRY_HKLTEST_H_
#define MANTID_GEOMETRY_HKLTEST_H_

#include <cxxtest/TestSuite.h>
#include <memory>
#include "MantidKernel/MDUnit.h"
#include "MantidGeometry/MDGeometry/HKL.h"

using Mantid::Geometry::HKL;

class HKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLTest *createSuite() { return new HKLTest(); }
  static void destroySuite(HKLTest *suite) { delete suite; }

  void test_check_unit_compatibility() {

    TSM_ASSERT_THROWS("Input unit for this frame must be a QUnit",
                      HKL(new Mantid::Kernel::LabelUnit("MeV")),
                      std::invalid_argument &);
  }

  void test_check_unit_compatibility_unique_ptr(){
      std::unique_ptr<Mantid::Kernel::MDUnit> badUnit(new Mantid::Kernel::LabelUnit("MeV"));


      HKL* testHKL = NULL;
      TSM_ASSERT_THROWS("Input unit for this frame must be a QUnit",
                        testHKL = new HKL(badUnit),
                        std::invalid_argument&);
      TSM_ASSERT("Construction should not have succeeded", testHKL == NULL );
      TSM_ASSERT("Ownership of input should not have changed", badUnit.get() != NULL );
  }

  void test_name() {
    HKL frame(new Mantid::Kernel::ReciprocalLatticeUnit);
    TS_ASSERT_EQUALS(HKL::HKLName, frame.name());
  }
};

#endif /* MANTID_GEOMETRY_HKLTEST_H_ */
