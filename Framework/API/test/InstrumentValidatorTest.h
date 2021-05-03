// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/InstrumentValidator.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::Kernel;
using Mantid::API::InstrumentValidator;

class InstrumentValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentValidatorTest *createSuite() { return new InstrumentValidatorTest(); }
  static void destroySuite(InstrumentValidatorTest *suite) { delete suite; }

  void test_success() {
    auto ws = std::make_shared<WorkspaceTester>();
    auto instr = std::make_shared<Mantid::Geometry::Instrument>("TestInstrument");
    ws->setInstrument(instr);
    // Define a sample position
    auto sample = new Mantid::Geometry::Component("samplePos", instr.get());
    instr->setPos(0.0, 0.0, 0.0);
    instr->add(sample);
    instr->markAsSamplePos(sample);

    InstrumentValidator validator;
    TS_ASSERT_EQUALS(validator.checkValidity(ws), "");
  }

  void test_that_the_expected_error_message_is_returned_when_the_instrument_is_missing_a_sample_component() {
    auto const workspace = std::make_shared<WorkspaceTester>();

    InstrumentValidator validator(InstrumentValidator::SamplePosition);

    TS_ASSERT_EQUALS(validator.checkValidity(workspace),
                     "The instrument is missing the following components: sample holder");
  }

  void test_that_the_expected_error_message_is_returned_when_the_instrument_is_missing_a_source_component() {
    auto const workspace = std::make_shared<WorkspaceTester>();

    InstrumentValidator validator(InstrumentValidator::SourcePosition);

    TS_ASSERT_EQUALS(validator.checkValidity(workspace), "The instrument is missing the following components: source");
  }
};
