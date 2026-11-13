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
#include "GUI/RunsTable/JobsViewUpdater.h"
#include "GUI/RunsTable/QtRunsTableView.h"
#include "GUI/RunsTable/RunsTablePresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
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
    for (auto column : {LookupRow::NOMINAL_SPECULAR_PIXEL, LookupRow::ACTUAL_SPECULAR_PIXEL})
      TS_ASSERT(lookupTable->isColumnHidden(column));
    for (auto column : {adjustedAngleColumn, nominalSpecularPixelColumn, actualSpecularPixelColumn})
      TS_ASSERT(tree.isColumnHidden(column));
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
    for (auto column : {adjustedAngleColumn, nominalSpecularPixelColumn, actualSpecularPixelColumn})
      TS_ASSERT(!tree.isColumnHidden(column));
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

  void testAdjustedAngleReadsReducedOutput() {
    auto row = withReductionOutput(makeSimpleRow("12345"));
    auto output = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("IvsQ_binned");
    output->mutableRun().addProperty("reflectometry_adjusted_theta", 0.875);
    auto cell = adjustedAngleCell(row, 3);
    TS_ASSERT_EQUALS(cell.contentText(), "0.875");
    TS_ASSERT(!cell.isEditable());
    TS_ASSERT(cell.isOutput());
  }

  void testAdjustedAngleReadsGroupedReducedOutput() {
    auto output = addWorkspace("period");
    output->mutableRun().addProperty("reflectometry_adjusted_theta", 0.875);
    auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
    group->addWorkspace(output);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("reduced_group", group);
    auto row = makeSimpleRow("12345");
    row.setOutputNames({{""}, {""}, {"reduced_group"}});
    TS_ASSERT_EQUALS(adjustedAngleCell(row, 3).contentText(), "0.875");
  }

private:
  Row withReductionOutput(Row row) {
    addWorkspace("IvsQ_binned");
    row.setOutputNames({{""}, {""}, {"IvsQ_binned"}});
    return row;
  }

  Mantid::API::MatrixWorkspace_sptr addWorkspace(std::string const &name) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 2);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(name, workspace);
    return workspace;
  }
};
