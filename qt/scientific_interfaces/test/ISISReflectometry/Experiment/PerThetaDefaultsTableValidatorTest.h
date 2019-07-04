// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_PERTHETADEFAULTSTABLEVALIDATORTEST_H_
#define MANTID_CUSTOMINTERFACES_PERTHETADEFAULTSTABLEVALIDATORTEST_H_
#include "../../../ISISReflectometry/GUI/Experiment/PerThetaDefaultsTableValidator.h"
#include "../../../ISISReflectometry/Reduction/TransmissionRunPair.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")

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
  static auto constexpr TOLERANCE = 0.001;
  using Cells = PerThetaDefaults::ValueArray;
  using Table = std::vector<Cells>;

  void testEmptyTable() {
    auto results = runTestValid(emptyTable());
    TS_ASSERT(results.size() == 0);
  }

  void testOneWildcardRow() {
    auto table = Table({emptyRow()});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT(results[0].isWildcard());
  }

  void testTwoWildcardRowsIsInvalid() {
    auto table = Table({emptyRow(), emptyRow()});
    runTestInvalidThetas(table, ThetaValuesValidationError::MultipleWildcards,
                         expectedErrors({0, 1}, {0}));
  }

  void testOneAngleRow() {
    auto table = Table({Cells({"0.5"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT(results[0].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[0].thetaOrWildcard().get(), 0.5);
  }

  void testTwoUniqueAngleRows() {
    auto table = Table({Cells({"0.5"}), Cells({"2.3"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 2);
    TS_ASSERT(results[0].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[0].thetaOrWildcard().get(), 0.5);
    TS_ASSERT(results[1].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[1].thetaOrWildcard().get(), 2.3);
  }

  void testTwoNonUniqueAngleRowsIsInvalid() {
    auto table = Table({Cells({"0.5"}), Cells({"0.5"})});
    runTestInvalidThetas(table, ThetaValuesValidationError::NonUniqueTheta,
                         expectedErrors({0, 1}, {0}));
  }

  void testInvalidAngle() {
    auto table = Table({Cells({"bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {0}));
  }

  void testValidTransmissionRuns() {
    auto table = Table({Cells({"", "13463", "13464"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(results[0].transmissionWorkspaceNames(),
                     TransmissionRunPair("13463", "13464"));
  }

  void testTransmissionRunsAreWorkspaceNames() {
    auto table = Table({Cells({"", "some workspace", "another_workspace"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(
        results[0].transmissionWorkspaceNames(),
        TransmissionRunPair("some workspace", "another_workspace"));
  }

  void testValidQRange() {
    auto table = Table({Cells({"", "", "", "0.05", "1.3", "0.021"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(results[0].qRange(), RangeInQ(0.05, 0.021, 1.3));
  }

  void testInvalidQRange() {
    auto table = Table({Cells({"", "", "", "bad", "bad", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {3, 4, 5}));
  }

  void testValidScaleFactor() {
    auto table = Table({Cells({"", "", "", "", "", "", "1.4"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(results[0].scaleFactor(), 1.4);
  }

  void testInvalidScaleFactor() {
    auto table = Table({Cells({"", "", "", "", "", "", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {6}));
  }

  void testValidProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "1-3"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT(results[0].processingInstructions().is_initialized());
    TS_ASSERT_EQUALS(results[0].processingInstructions().get(), "1-3");
  }

  void testInvalidProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {7}));
  }

  void testAnglesThatDifferByTolerance() {
    auto table = Table({Cells({"0.5"}), Cells({"0.501"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 2);
    TS_ASSERT(results[0].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[0].thetaOrWildcard().get(), 0.5);
    TS_ASSERT(results[1].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[1].thetaOrWildcard().get(), 0.501);
  }

  void testAnglesThatDifferByLessThanTolerance() {
    auto table = Table({Cells({"0.5"}), Cells({"0.5009"})});
    runTestInvalidThetas(table, ThetaValuesValidationError::NonUniqueTheta,
                         expectedErrors({0, 1}, {0}));
  }

  void testCorrectRowMarkedAsInvalidInMultiRowTable() {
    auto row1 = Cells({"0.5"});
    auto row2 = Cells({"1.2", "", "", "bad"});
    auto row3 = Cells({"2.3"});
    auto table = Table({row1, row2, row3});
    runTestInvalidCells(table, expectedErrors({1}, {3}));
  }

private:
  Table emptyTable() { return Table(); }
  Cells emptyRow() { return Cells(); }

  std::vector<InvalidDefaultsError> expectedErrors(std::vector<int> rows,
                                                   std::vector<int> columns) {
    std::vector<InvalidDefaultsError> errors;
    for (auto row : rows)
      errors.push_back(InvalidDefaultsError(row, columns));
    return errors;
  }

  std::vector<PerThetaDefaults> runTestValid(Table table) {
    PerThetaDefaultsTableValidator validator;
    auto result = validator(table, TOLERANCE);
    TS_ASSERT(result.isValid());
    return result.assertValid();
  }

  void runTestInvalidThetas(Table table,
                            ThetaValuesValidationError thetaValuesError,
                            std::vector<InvalidDefaultsError> expectedErrors) {
    PerThetaDefaultsTableValidator validator;
    auto result = validator(table, TOLERANCE);
    TS_ASSERT(result.isError());
    auto validationError = result.assertError();
    TS_ASSERT(validationError.fullTableError().is_initialized());
    TS_ASSERT_EQUALS(validationError.fullTableError().get(), thetaValuesError);
    TS_ASSERT_EQUALS(validationError.errors(), expectedErrors);
  }

  void runTestInvalidCells(Table table,
                           std::vector<InvalidDefaultsError> expectedErrors) {
    PerThetaDefaultsTableValidator validator;
    auto result = validator(table, TOLERANCE);
    TS_ASSERT(result.isError());
    auto validationError = result.assertError();
    TS_ASSERT_EQUALS(validationError.errors(), expectedErrors);
  }
};

GNU_DIAG_ON("missing-braces")

#endif // MANTID_CUSTOMINTERFACES_PERTHETADEFAULTSTABLEVALIDATORTEST_H_
