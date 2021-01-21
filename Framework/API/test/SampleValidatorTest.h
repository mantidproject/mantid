// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleValidator.h"
#include "MantidKernel/Material.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::Kernel;
using Mantid::API::SampleValidator;

class SampleValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleValidatorTest *createSuite() { return new SampleValidatorTest(); }
  static void destroySuite(SampleValidatorTest *suite) { delete suite; }

  void test_fail() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 11, 10);
    SampleValidator validator;
    TS_ASSERT_EQUALS(validator.isValid(ws), "The sample is missing the following properties: shape,material");
  }

  void test_success() {
    auto ws = std::make_shared<WorkspaceTester>();
    auto sphere = ComponentCreationHelper::createSphere(1.0, V3D(), "sphere");
    Mantid::Kernel::Material material("stuff", Mantid::PhysicalConstants::NeutronAtom(), 10);
    sphere->setMaterial(material);
    ws->mutableSample().setShape(sphere);

    SampleValidator validator;
    TS_ASSERT_EQUALS(validator.checkValidity(ws), "");
  }
};
