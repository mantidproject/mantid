// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../Common/CoderCommonTester.h"
#include "../ReflMockObjects.h"
#include "GUI/Common/PlotOptionsProvider.h"
#include "GUI/Common/RunWorkspaceResolver.h"
#include "GUI/RunsTable/JobsViewUpdater.h"
#include "GUI/RunsTable/QtRunsTableView.h"
#include "GUI/RunsTable/RunsTablePresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "TestHelpers/ModelCreationHelper.h"

#include <QComboBox>
#include <QHeaderView>
#include <QSignalBlocker>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;

class AdjustedAngleTest : public CxxTest::TestSuite {
public:
  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void testColumnIsNextToAngleAndHiddenInitiallyForOtherInstruments() {
    QtRunsTableView view({"INTER", "POLREF"});
    QSignalBlocker blocker(view.findChild<QComboBox *>("instrumentSelector"));
    auto &tree = dynamic_cast<MantidQt::MantidWidgets::Batch::JobTreeView &>(view.jobs());
    TS_ASSERT_EQUALS(tree.header()->visualIndex(adjustedAngleColumn), tree.header()->visualIndex(1) + 1);
    TS_ASSERT_EQUALS(tree.model()->headerData(adjustedAngleColumn, Qt::Horizontal).toString(), "Adjusted Angle");
    auto algorithm = std::make_shared<ReflectometryISISLoadAndProcess>();
    algorithm->initialize();
    QtExperimentView lookupView(algorithm);
    auto lookupTable = lookupView.findChild<QTableWidget *>("optionsTable");
    for (auto const &instrument : {"INTER", "POLREF", "INTER"}) {
      view.setInstrumentName(instrument);
      lookupView.setInstrumentColumnsVisible(instrument);
      for (auto column : {LookupRow::NOMINAL_SPECULAR_PIXEL, LookupRow::ACTUAL_SPECULAR_PIXEL})
        TS_ASSERT_EQUALS(lookupTable->isColumnHidden(column), std::string(instrument) != "POLREF");
      for (auto column : {adjustedAngleColumn, nominalSpecularPixelColumn, actualSpecularPixelColumn})
        TS_ASSERT_EQUALS(tree.isColumnHidden(column), std::string(instrument) != "POLREF");
    }
  }

  void testColumnInitiallyVisibleForPOLREF() {
    QtRunsTableView view({"POLREF", "INTER"});
    auto &tree = dynamic_cast<MantidQt::MantidWidgets::Batch::JobTreeView &>(view.jobs());
    TS_ASSERT(!tree.isColumnHidden(adjustedAngleColumn));
  }

  void testNewRowsHaveReadOnlyAdjustedAngleCells() {
    QtRunsTableView view({"POLREF"});
    testing::NiceMock<MockPlotter> plotter;
    PlotOptionsProvider options;
    RunsTablePresenter presenter(&view, {"POLREF"}, 0.01, ReductionJobs(), plotter, options);
    auto cell = view.jobs().cellAt(MantidQt::MantidWidgets::Batch::RowLocation({0, 0}), adjustedAngleColumn);
    TS_ASSERT(cell.contentText().empty());
    TS_ASSERT(!cell.isEditable());
  }

  void testInsertedRowsHaveReadOnlyAdjustedAngleAndEditableInputs() {
    QtRunsTableView view({"POLREF"});
    testing::NiceMock<MockPlotter> plotter;
    testing::NiceMock<MockRunsPresenter> mainPresenter;
    PlotOptionsProvider options;
    RunsTablePresenter presenter(&view, {"POLREF"}, 0.01, ReductionJobs(), plotter, options);
    presenter.acceptMainPresenter(&mainPresenter);
    auto const location = view.jobs().appendChildRowOf(MantidQt::MantidWidgets::Batch::RowLocation({0}));
    presenter.notifyRowInserted(location);
    TS_ASSERT(!view.jobs().cellAt(location, adjustedAngleColumn).isEditable());
    TS_ASSERT(view.jobs().cellAt(location, 0).isEditable());
    TS_ASSERT(view.jobs().cellAt(location, 1).isEditable());
    TS_ASSERT(view.jobs().cellAt(location, 4).isEditable());
  }

  void testExactWorkspaceNameTakesPrecedence() {
    addRunWorkspace("sample", "12345", 0.625);
    addRunWorkspace("TOF_sample", "12345", 0.75);
    TS_ASSERT_EQUALS(
        std::stod(adjustedAngleCell(withReductionOutput(makeSimpleRow("sample")), std::nullopt).contentText()), 0.625);
  }

  void testSummedRunsReadSummedWorkspaceLog() {
    addRunWorkspace("TOF_12345+12346", "12345+12346", 0.625);
    addRunWorkspace("12345+12346", "12345+12346", 0.875);
    addRunWorkspace("TOF_12345", "12345", 0.75);
    auto row = withReductionOutput(makeRow(std::vector<std::string>{"12345", "12346"}), "12345+12346");
    TS_ASSERT_EQUALS(std::stod(adjustedAngleCell(row, std::nullopt).contentText()), 0.625);
    auto namedRow = withReductionOutput(makeRow(std::vector<std::string>{"TOF_12345", "TOF_12346"}), "12345+12346");
    TS_ASSERT_EQUALS(std::stod(adjustedAngleCell(namedRow, std::nullopt).contentText()), 0.625);
  }

  void testCollapsedInputGroupFindsMatchingRunAmongOtherRuns() {
    auto unrelated = addRunWorkspace("other_run_period", "99999", 0.875);
    auto period1 = addRunWorkspace("TOF_12345_1", "12345", 0.625);
    auto period2 = addRunWorkspace("TOF_12345_2", "12345", 0.625);
    addGroup("TOF", {unrelated, period1, period2});
    TS_ASSERT(!Mantid::API::AnalysisDataService::Instance().doesExist("TOF_12345"));
    TS_ASSERT_EQUALS(adjustedAngleCell(withReductionOutput(makeSimpleRow("12345")), 3).contentText(), "0.625");
  }

  void testCollapsedHiddenInputGroupFindsMatchingRun() {
    auto period = addRunWorkspace("__TOF_12345_1", "12345", 0.625);
    addGroup("__TOF", {period});
    TS_ASSERT_EQUALS(adjustedAngleCell(withReductionOutput(makeSimpleRow("12345")), 3).contentText(), "0.625");
  }

  void testHiddenSummedWorkspaceIsFound() {
    addRunWorkspace("__TOF_12345+12346", "12345+12346", 0.625);
    auto row = withReductionOutput(makeRow(std::vector<std::string>{"12345", "12346"}), "12345+12346");
    TS_ASSERT_EQUALS(adjustedAngleCell(row, 3).contentText(), "0.625");
  }

  void testSummedWorkspaceUsesExplicitInputAliasesInsteadOfTheirRunNumbers() {
    addRunWorkspace("sampleA", "12345", 0.875);
    addRunWorkspace("sampleB", "12346", 0.875);
    addRunWorkspace("TOF_sampleA+sampleB", "sampleA+sampleB", 0.625);
    auto row = withReductionOutput(makeRow(std::vector<std::string>{"sampleA", "sampleB"}), "sampleA+sampleB");
    TS_ASSERT_EQUALS(adjustedAngleCell(row, 3).contentText(), "0.625");
  }

  void testReducedRunMetadataIsUsedVerbatimForSummedAliases() {
    addRunWorkspace("TOF_0012345+0012346", "0012345+0012346", 0.625);
    addRunWorkspace("IvsQ_binned", "0012345+0012346", 0.875);
    auto row = makeRow(std::vector<std::string>{"first.nxs", "second.nxs"});
    row.setOutputNames({"IvsLam", "IvsQ", "IvsQ_binned"});
    TS_ASSERT_EQUALS(adjustedAngleCell(row, 3).contentText(), "0.625");
  }

  void testExistingPaddedSingleInputMatchesItsNumericRunLog() {
    addRunWorkspace("TOF_0012345", "12345", 0.625);
    TS_ASSERT_EQUALS(adjustedAngleCell(withReductionOutput(makeSimpleRow("0012345")), 3).contentText(), "0.625");
  }

  void testFilenameInputUsesReducedWorkspaceRunMetadata() {
    auto input = addRunWorkspace("period_with_arbitrary_name", "12345", 0.625);
    addGroup("TOF", {input});
    auto output = addRunWorkspace("IvsQ_binned_12345_1", "12345", 0.875);
    addGroup("IvsQ_binned_12345", {output});
    auto row = makeSimpleRow("/data/POLREF00012345.nxs");
    row.setOutputNames({"IvsLam_12345", "IvsQ_12345", "IvsQ_binned_12345"});
    TS_ASSERT_EQUALS(adjustedAngleCell(row, 3).contentText(), "0.625");
  }

  void testMissingInputDoesNotUseReducedWorkspaceAdjustedAngle() {
    addRunWorkspace("IvsQ_binned_12345", "12345", 0.875);
    auto row = makeSimpleRow("12345");
    row.setOutputNames({"IvsLam_12345", "IvsQ_12345", "IvsQ_binned_12345"});
    auto cell = adjustedAngleCell(row, std::nullopt);
    TS_ASSERT(cell.contentText().empty());
    TS_ASSERT(!cell.isEditable());
    TS_ASSERT(cell.isOutput());
  }

  void testMisleadingWorkspaceNameDoesNotOverrideRunMetadata() {
    addRunWorkspace("TOF_12345", "99999", 0.875);
    auto input = addRunWorkspace("actual_input_period", "12345", 0.625);
    addGroup("TOF", {input});
    TS_ASSERT_EQUALS(adjustedAngleCell(withReductionOutput(makeSimpleRow("12345")), 3).contentText(), "0.625");
  }

  void testMissingLogOnMatchingRunDoesNotUseAnotherRunsLog() {
    auto unrelated = addRunWorkspace("other_run_period", "99999", 0.875);
    auto input = addWorkspace("TOF_12345_1");
    input->mutableRun().addProperty("run_number", std::string("12345"));
    addGroup("TOF", {unrelated, input});
    TS_ASSERT(adjustedAngleCell(withReductionOutput(makeSimpleRow("12345")), std::nullopt).contentText().empty());
  }

  void testNonMatrixGroupMembersAreIgnored() {
    auto input = addRunWorkspace("input_period", "12345", 0.625);
    auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
    group->addWorkspace(Mantid::API::WorkspaceFactory::Instance().createTable());
    group->addWorkspace(input);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("TOF", group);
    TS_ASSERT_EQUALS(adjustedAngleCell(withReductionOutput(makeSimpleRow("12345")), 3).contentText(), "0.625");
  }

  void testInvalidPeriodMetadataDoesNotHideMatchingPeriod() {
    auto invalid = addRunWorkspace("invalid_period", "12345", 0.875);
    invalid->mutableRun().addProperty("current_period", std::string("invalid"));
    auto input = addRunWorkspace("period_two", "12345", 0.625);
    input->mutableRun().addProperty("current_period", 2);
    addGroup("TOF", {invalid, input});
    auto row = withReductionOutput(makeSimpleRow("12345"));
    TS_ASSERT_EQUALS(findInputWorkspace(row.runNumbers(), 2, {row.reducedWorkspaceNames().iVsQBinned()}), input);
  }

  void testEmptyWorkspaceGroupIsBlank() {
    Mantid::API::AnalysisDataService::Instance().addOrReplace("TOF_12345",
                                                              std::make_shared<Mantid::API::WorkspaceGroup>());
    TS_ASSERT(adjustedAngleCell(withReductionOutput(makeSimpleRow("12345")), std::nullopt).contentText().empty());
  }

private:
  Row withReductionOutput(Row row, std::string const &runNumber = "12345") {
    auto output = addWorkspace("IvsQ_binned");
    output->mutableRun().addProperty("run_number", runNumber);
    row.setOutputNames({"", "", "IvsQ_binned"});
    return row;
  }

  Mantid::API::MatrixWorkspace_sptr addRunWorkspace(std::string const &name, std::string const &runNumber,
                                                    double angle) {
    auto workspace = addWorkspace(name);
    workspace->mutableRun().addProperty("run_number", runNumber);
    workspace->mutableRun().addProperty("reflectometry_adjusted_theta", angle);
    return workspace;
  }

  void addGroup(std::string const &name, std::vector<Mantid::API::MatrixWorkspace_sptr> const &members) {
    auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
    for (auto const &member : members)
      group->addWorkspace(member);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(name, group);
  }

  Mantid::API::MatrixWorkspace_sptr addWorkspace(std::string const &name) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 2);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(name, workspace);
    return workspace;
  }
};
