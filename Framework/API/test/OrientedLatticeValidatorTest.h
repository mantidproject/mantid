// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <memory>

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/OrientedLatticeValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

class OrientedLatticeValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static OrientedLatticeValidatorTest *createSuite() { return new OrientedLatticeValidatorTest(); }
  static void destroySuite(OrientedLatticeValidatorTest *suite) { delete suite; }

  void test_getType() {
    OrientedLatticeValidator validator;
    TS_ASSERT_EQUALS(validator.getType(), "orientedlattice")
  }

  void test_isValid_is_valid_when_latticeDefined() {
    auto info = std::make_shared<ExperimentInfo>();
    info->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>());

    OrientedLatticeValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(info), "");
  };

  void test_isValid_is_invalid_when_latticeUndefined() {
    auto info = std::make_shared<ExperimentInfo>();
    OrientedLatticeValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(info), "Workspace must have a sample with an orientation matrix defined.");
  };
};
