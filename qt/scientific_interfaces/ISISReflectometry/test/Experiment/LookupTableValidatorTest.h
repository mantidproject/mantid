// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/GUI/Experiment/LookupTableValidator.h"
#include "../../../ISISReflectometry/Reduction/TransmissionRunPair.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class LookupTableValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LookupTableValidatorTest *createSuite() { return new LookupTableValidatorTest(); }
  static void destroySuite(LookupTableValidatorTest *suite) { delete suite; }
  static auto constexpr TOLERANCE = 0.001;
  using Cells = LookupRow::ValueArray;
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
    runTestInvalidThetas(table, LookupCriteriaError::MultipleWildcards,
                         expectedErrors({0, 1}, {LookupRow::Column::THETA, LookupRow::Column::TITLE}));
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
    runTestInvalidThetas(table, LookupCriteriaError::NonUniqueSearchCriteria,
                         expectedErrors({0, 1}, {LookupRow::Column::THETA, LookupRow::Column::TITLE}));
  }

  void testMatchingAngleRowsWithDifferentTitleMatchersAreUnique() {
    auto const title1 = std::string("title1");
    auto const title2 = std::string("title2");
    auto table = Table({Cells({"0.5", title1}), Cells({"0.5", title2})});

    auto results = runTestValid(table);

    TS_ASSERT_EQUALS(results.size(), 2);
    TS_ASSERT(results[0].thetaOrWildcard().is_initialized());
    TS_ASSERT(results[1].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[0].thetaOrWildcard().get(), results[1].thetaOrWildcard().get());
    TS_ASSERT_EQUALS(results[0].titleMatcher().value().expression(), title1);
    TS_ASSERT_EQUALS(results[1].titleMatcher().value().expression(), title2);
  }

  void testDuplicateAnglesAndTitleMatchersAreInvalid() {
    auto table = Table({Cells({"0.5", "title"}), Cells({"0.5", "title"})});
    runTestInvalidThetas(table, LookupCriteriaError::NonUniqueSearchCriteria,
                         expectedErrors({0, 1}, {LookupRow::Column::THETA, LookupRow::Column::TITLE}));
  }

  void testInvalidAngle() {
    auto table = Table({Cells({"bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {LookupRow::Column::THETA}));
  }

  void testValidTransmissionRuns() {
    auto table = Table({Cells({"", "", "13463", "13464"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(results[0].transmissionWorkspaceNames(), TransmissionRunPair("13463", "13464"));
  }

  void testTransmissionRunsAreWorkspaceNames() {
    auto table = Table({Cells({"", "", "some workspace", "another_workspace"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(results[0].transmissionWorkspaceNames(),
                     TransmissionRunPair("some workspace", "another_workspace"));
  }

  void testValidTransmissionProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "1-3"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT(results[0].transmissionProcessingInstructions().is_initialized());
    TS_ASSERT_EQUALS(results[0].transmissionProcessingInstructions().get(), "1-3");
  }

  void testInvalidTransmissionProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {LookupRow::Column::TRANS_SPECTRA}));
  }

  void testValidQRange() {
    auto table = Table({Cells({"", "", "", "", "", "0.05", "1.3", "0.021"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(results[0].qRange(), RangeInQ(0.05, 0.021, 1.3));
  }

  void testInvalidQRange() {
    auto table = Table({Cells({"", "", "", "", "", "bad", "bad", "bad"})});
    runTestInvalidCells(
        table, expectedErrors({0}, {LookupRow::Column::QMIN, LookupRow::Column::QMAX, LookupRow::Column::QSTEP}));
  }

  void testValidScaleFactor() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "1.4"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT_EQUALS(results[0].scaleFactor(), 1.4);
  }

  void testInvalidScaleFactor() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {LookupRow::Column::SCALE}));
  }

  void testValidProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "", "1-3"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT(results[0].processingInstructions().is_initialized());
    TS_ASSERT_EQUALS(results[0].processingInstructions().get(), "1-3");
  }

  void testInvalidProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {LookupRow::Column::RUN_SPECTRA}));
  }

  void testValidBackgroundProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "", "", "1-3"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT(results[0].backgroundProcessingInstructions().is_initialized());
    TS_ASSERT_EQUALS(results[0].backgroundProcessingInstructions().get(), "1-3");
  }

  void testInvalidBackgroundProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "", "", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {LookupRow::Column::BACKGROUND_SPECTRA}));
  }

  void testValidROIDetectorProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "", "", "", "1-3"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 1);
    TS_ASSERT(results[0].roiDetectorIDs().is_initialized());
    TS_ASSERT_EQUALS(results[0].roiDetectorIDs().get(), "1-3");
  }

  void testInvalidROIDetectorProcessingInstructions() {
    auto table = Table({Cells({"", "", "", "", "", "", "", "", "", "", "", "bad"})});
    runTestInvalidCells(table, expectedErrors({0}, {LookupRow::Column::ROI_DETECTOR_IDS}));
  }

  void testAnglesThatDifferByTolerance() {
    auto table = Table({Cells({"0.5"}), Cells({"0.5011"})});
    auto results = runTestValid(table);
    TS_ASSERT_EQUALS(results.size(), 2);
    TS_ASSERT(results[0].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[0].thetaOrWildcard().get(), 0.5);
    TS_ASSERT(results[1].thetaOrWildcard().is_initialized());
    TS_ASSERT_EQUALS(results[1].thetaOrWildcard().get(), 0.5011);
  }

  void testAnglesThatDifferByLessThanTolerance() {
    auto table = Table({Cells({"0.5"}), Cells({"0.5009"})});
    runTestInvalidThetas(table, LookupCriteriaError::NonUniqueSearchCriteria,
                         expectedErrors({0, 1}, {LookupRow::Column::THETA, LookupRow::Column::TITLE}));
  }

  void testCorrectRowMarkedAsInvalidInMultiRowTable() {
    auto row0 = Cells({"0.5"});
    auto row1 = Cells({"1.2", "", "", "", "bad"});
    auto row2 = Cells({"2.3"});
    auto table = Table({row0, row1, row2});
    runTestInvalidCells(table, expectedErrors({1}, {LookupRow::Column::TRANS_SPECTRA}));
  }

private:
  Table emptyTable() { return Table(); }
  Cells emptyRow() { return Cells(); }

  std::vector<InvalidLookupRowCells> expectedErrors(const std::vector<int> &rows,
                                                    const std::unordered_set<int> &columns) {
    std::vector<InvalidLookupRowCells> errors;
    for (auto row : rows)
      errors.emplace_back(InvalidLookupRowCells(row, columns));
    return errors;
  }

  std::vector<LookupRow> runTestValid(const Table &table) {
    LookupTableValidator validator;
    auto result = validator(table, TOLERANCE);
    TS_ASSERT(result.isValid());
    if (result.isValid()) {
      return result.assertValid();
    }
    return {};
  }

  void runTestInvalidThetas(const Table &table, LookupCriteriaError thetaValuesError,
                            std::vector<InvalidLookupRowCells> expectedErrors) {
    LookupTableValidator validator;
    auto result = validator(table, TOLERANCE);
    TS_ASSERT(result.isError());
    auto validationError = result.assertError();
    TS_ASSERT(validationError.fullTableError().is_initialized());
    TS_ASSERT_EQUALS(validationError.fullTableError().get(), thetaValuesError);
    TS_ASSERT_EQUALS(validationError.errors(), expectedErrors);
  }

  void runTestInvalidCells(const Table &table, std::vector<InvalidLookupRowCells> expectedErrors) {
    LookupTableValidator validator;
    auto result = validator(table, TOLERANCE);
    TS_ASSERT(result.isError());
    auto validationError = result.assertError();
    TS_ASSERT_EQUALS(validationError.errors(), expectedErrors);
  }
};
