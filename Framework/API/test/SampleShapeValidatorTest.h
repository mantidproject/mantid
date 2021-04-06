// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleShapeValidator.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"

#include "boost/make_shared.hpp"

using namespace Mantid::Kernel;
using Mantid::API::SampleShapeValidator;

class SampleShapeValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleShapeValidatorTest *createSuite() { return new SampleShapeValidatorTest(); }
  static void destroySuite(SampleShapeValidatorTest *suite) { delete suite; }

  void test_validator_passes_for_workspace_with_defined_sample_shape() {
    auto fakeWS = std::make_shared<WorkspaceTester>();
    // Add a sample shape
    auto sphere = ComponentCreationHelper::createSphere(1.0, V3D(), "sphere");
    fakeWS->mutableSample().setShape(sphere);

    auto sampleValidator = std::make_shared<SampleShapeValidator>();
    TS_ASSERT_EQUALS(sampleValidator->isValid(fakeWS), "");
  }

  void test_validator_throws_error_for_workspace_without_shape() {
    auto fakeWS = std::make_shared<WorkspaceTester>();

    auto sampleValidator = std::make_shared<SampleShapeValidator>();
    TS_ASSERT_EQUALS(sampleValidator->isValid(fakeWS), "Invalid or no shape defined for sample");
  }
};
