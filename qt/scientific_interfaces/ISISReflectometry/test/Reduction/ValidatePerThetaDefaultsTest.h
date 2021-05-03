// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/Reduction/ValidatePerThetaDefaults.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")

class ValidatePerThetaDefaultsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ValidatePerThetaDefaultsTest *createSuite() { return new ValidatePerThetaDefaultsTest(); }
  static void destroySuite(ValidatePerThetaDefaultsTest *suite) { delete suite; }

  void testParseTheta() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"1.3"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().thetaOrWildcard().get(), 1.3);
  }

  void testParseThetaWildcard() {
    PerThetaDefaultsValidator validator;
    auto result = validator({""});
    TS_ASSERT(result.isValid());
    TS_ASSERT(!result.assertValid().thetaOrWildcard().is_initialized());
  }

  void testParseThetaError() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"bad"});
    std::vector<int> errorCells = {0};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseTransmissionRuns() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "13463", "13464"});
    auto expected = TransmissionRunPair("13463", "13464");
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().transmissionWorkspaceNames(), expected);
  }

  void testParseTransmissionRunsWithWorkspaceNames() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "some workspace", "another_workspace"});
    auto expected = TransmissionRunPair("some workspace", "another_workspace");
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().transmissionWorkspaceNames(), expected);
  }

  void testParseTransmissionProcessingInstructions() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "4-7"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().transmissionProcessingInstructions().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().transmissionProcessingInstructions().get(), "4-7");
  }

  void testParseTransmissionProcessingInstructionsError() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "bad"});
    std::vector<int> errorCells = {3};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseQRange() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "0.05", "1.3", "0.02"});
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().qRange(), RangeInQ(0.05, 0.02, 1.3));
  }

  void testParseQRangeError() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "bad", "bad", "bad"});
    std::vector<int> errorCells = {4, 5, 6};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseScaleFactor() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "1.4"});
    TS_ASSERT(result.isValid());
    TS_ASSERT_EQUALS(result.assertValid().scaleFactor(), 1.4);
  }

  void testParseScaleFactorError() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "bad"});
    std::vector<int> errorCells = {7};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseProcessingInstructions() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "1-3"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().processingInstructions().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().processingInstructions().get(), "1-3");
  }

  void testParseProcessingInstructionsError() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "bad"});
    std::vector<int> errorCells = {8};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }

  void testParseBackgroundProcessingInstructions() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "4-7"});
    TS_ASSERT(result.isValid());
    TS_ASSERT(result.assertValid().backgroundProcessingInstructions().is_initialized());
    TS_ASSERT_EQUALS(result.assertValid().backgroundProcessingInstructions().get(), "4-7");
  }

  void testParseBackgroundProcessingInstructionsError() {
    PerThetaDefaultsValidator validator;
    auto result = validator({"", "", "", "", "", "", "", "", "", "bad"});
    std::vector<int> errorCells = {9};
    TS_ASSERT(result.isError());
    TS_ASSERT_EQUALS(result.assertError(), errorCells);
  }
};

GNU_DIAG_ON("missing-braces")
