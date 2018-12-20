#ifndef MANTID_ORIENTEDLATTICEVALIDATOR_TEST_H
#define MANTID_ORIENTEDLATTICEVALIDATOR_TEST_H

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/OrientedLatticeValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidTestHelpers/FakeObjects.h"

class OrientedLatticeValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static OrientedLatticeValidatorTest *createSuite() {
    return new OrientedLatticeValidatorTest();
  }
  static void destroySuite(OrientedLatticeValidatorTest *suite) {
    delete suite;
  }

  void test_getType() {
    OrientedLatticeValidator validator;
    TS_ASSERT_EQUALS(validator.getType(), "orientedlattice")
  }

  void test_isValid_is_valid_when_latticeDefined() {
    OrientedLattice lattice;
    auto info = boost::make_shared<ExperimentInfo>();
    info->mutableSample().setOrientedLattice(&lattice);

    OrientedLatticeValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(info), "");
  };

  void test_isValid_is_invalid_when_latticeUndefined() {
    auto info = boost::make_shared<ExperimentInfo>();
    OrientedLatticeValidator validator;
    TS_ASSERT_EQUALS(
        validator.isValid(info),
        "Workspace must have a sample with an orientation matrix defined.");
  };
};

#endif // MANTID_ORIENTEDLATTICEVALIDATOR_TEST_H
