// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Encoder.h"
#include "../../Reduction/Group.h"
#include "../../Reduction/ReductionJobs.h"
#include "../../Reduction/ReductionWorkspaces.h"
#include "../../Reduction/Row.h"
#include "../Batch/BatchPresenter.h"
#include "../Batch/QtBatchView.h"
#include "../Experiment/QtExperimentView.h"
#include "../Instrument/QtInstrumentView.h"
#include "../MainWindow/QtMainWindowView.h"
#include "../Runs/QtCatalogSearcher.h"
#include "../Runs/QtRunsView.h"
#include "../Runs/RunsPresenter.h"
#include "../RunsTable/QtRunsTableView.h"
#include "../RunsTable/RunsTablePresenter.h"
#include "../Save/QtSaveView.h"

namespace {
auto constexpr BATCH_VERSION = 2;
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

BatchPresenter *Encoder::findBatchPresenter(const QtBatchView *gui, const IMainWindowView *view) {
  auto mwv = dynamic_cast<const QtMainWindowView *>(view);
  for (auto &ipresenter : mwv->m_presenter->m_batchPresenters) {
    auto presenter = dynamic_cast<BatchPresenter *>(ipresenter.get());
    if (presenter->m_view == gui) {
      return presenter;
    }
  }
  return nullptr;
}

QMap<QString, QVariant> Encoder::encode(const QWidget *gui, const std::string &directory) {
  UNUSED_ARG(directory)
  auto mwv = dynamic_cast<const QtMainWindowView *>(gui);
  QMap<QString, QVariant> topLevelMap;
  topLevelMap.insert(QString("tag"), QVariant(QString("ISIS Reflectometry")));
  QList<QVariant> batches;
  for (size_t batchIndex = 0; batchIndex < mwv->batches().size(); ++batchIndex) {
    batches.append(QVariant(encodeBatch(mwv, static_cast<int>(batchIndex), true)));
  }
  topLevelMap.insert(QString("batches"), QVariant(batches));
  return topLevelMap;
}

QList<QString> Encoder::tags() { return QList<QString>({QString("ISIS Reflectometry")}); }

QMap<QString, QVariant> Encoder::encodeBatch(const IMainWindowView *mwv, int batchIndex, bool projectSave) {
  auto gui = dynamic_cast<const QtBatchView *>(mwv->batches()[batchIndex]);
  auto batchPresenter = findBatchPresenter(gui, mwv);
  if (!batchPresenter) {
    throw std::runtime_error("BatchPresenter could not be found during encode.");
  }
  auto runsPresenter = dynamic_cast<RunsPresenter *>(batchPresenter->m_runsPresenter.get());
  auto runsTablePresenter = dynamic_cast<RunsTablePresenter *>(runsPresenter->m_tablePresenter.get());
  auto reductionJobs = &runsTablePresenter->m_model.m_reductionJobs;
  auto searcher = dynamic_cast<QtCatalogSearcher *>(runsPresenter->m_searcher.get());

  QMap<QString, QVariant> batchMap;
  batchMap.insert(QString("version"), QVariant(BATCH_VERSION));
  batchMap.insert(QString("runsView"), QVariant(encodeRuns(gui->m_runs.get(), projectSave, reductionJobs, searcher)));
  batchMap.insert(QString("eventView"), QVariant(encodeEvent(gui->m_eventHandling.get())));
  batchMap.insert(QString("experimentView"), QVariant(encodeExperiment(gui->m_experiment.get())));
  batchMap.insert(QString("instrumentView"), QVariant(encodeInstrument(gui->m_instrument.get())));
  batchMap.insert(QString("saveView"), QVariant(encodeSave(gui->m_save.get())));
  return batchMap;
}

QMap<QString, QVariant> Encoder::encodeRuns(const QtRunsView *gui, bool projectSave, const ReductionJobs *redJobs,
                                            QtCatalogSearcher *searcher) {
  QMap<QString, QVariant> runsMap;
  runsMap.insert(QString("runsTable"), QVariant(encodeRunsTable(gui->m_tableView, projectSave, redJobs)));
  runsMap.insert(QString("comboSearchInstrument"), QVariant(gui->m_ui.comboSearchInstrument->currentIndex()));
  // This is not ideal but the search criteria may be changed on the view and
  // no longer be relevant for the search results. The latter are more
  // important so use the cached search criteria, i.e. only save the search
  // criteria if they have been used to perform a search
  runsMap.insert(QString("textSearch"), QVariant(QString::fromStdString(searcher->searchCriteria().investigation)));
  runsMap.insert(QString("textCycle"), QVariant(QString::fromStdString(searcher->searchCriteria().cycle)));
  runsMap.insert(QString("textInstrument"), QVariant(QString::fromStdString(searcher->searchCriteria().instrument)));
  runsMap.insert(QString("searchResults"), QVariant(encodeSearchModel(gui->searchResults())));
  return runsMap;
}

QMap<QString, QVariant> Encoder::encodeRunsTable(const QtRunsTableView *gui, bool projectSave,
                                                 const ReductionJobs *redJobs) {
  QMap<QString, QVariant> runTableMap;
  runTableMap.insert(QString("filterBox"), QVariant(gui->m_ui.filterBox->text()));

  runTableMap.insert(QString("projectSave"), QVariant(projectSave));
  runTableMap.insert(QString("runsTableModel"), QVariant(encodeRunsTableModel(redJobs)));
  return runTableMap;
}

QList<QVariant> Encoder::encodeRunsTableModel(const ReductionJobs *redJobs) {
  QList<QVariant> groups;
  for (const auto &group : redJobs->groups()) {
    groups.append(QVariant(encodeGroup(group)));
  }
  return groups;
}

QMap<QString, QVariant> Encoder::encodeGroup(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group) {
  QMap<QString, QVariant> groupMap;
  groupMap.insert(QString("name"), QVariant(QString::fromStdString(group.m_name)));
  groupMap.insert(QString("itemState"), QVariant(static_cast<int>(group.state())));
  groupMap.insert(QString("postprocessedWorkspaceName"),
                  QVariant(QString::fromStdString(group.m_postprocessedWorkspaceName)));
  groupMap.insert(QString("rows"), QVariant(encodeRows(group)));
  return groupMap;
}

QList<QVariant> Encoder::encodeRows(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group) {
  QList<QVariant> rows;
  for (const auto &row : group.m_rows) {
    if (row) {
      rows.append(QVariant(encodeRow(row.get())));
    } else {
      rows.append(QVariant(QMap<QString, QVariant>()));
    }
  }
  return rows;
}

QMap<QString, QVariant> Encoder::encodeRangeInQ(const RangeInQ &rangeInQ) {
  QMap<QString, QVariant> qRangeMap;
  auto min = rangeInQ.min();
  auto max = rangeInQ.max();
  auto step = rangeInQ.step();
  qRangeMap.insert(QString("minPresent"), QVariant(static_cast<bool>(min)));
  if (min)
    qRangeMap.insert(QString("min"), QVariant(min.value()));
  qRangeMap.insert(QString("maxPresent"), QVariant(static_cast<bool>(max)));
  if (max)
    qRangeMap.insert(QString("max"), QVariant(max.value()));
  qRangeMap.insert(QString("stepPresent"), QVariant(static_cast<bool>(step)));
  if (step)
    qRangeMap.insert(QString("step"), QVariant(step.value()));
  return qRangeMap;
}

QMap<QString, QVariant> Encoder::encodeTransmissionRunPair(const TransmissionRunPair &transRunPair) {
  QList<QVariant> firstTransRunNums;
  for (const auto &firstTransRunNum : transRunPair.firstTransmissionRunNumbers()) {
    firstTransRunNums.append(QVariant(QString::fromStdString(firstTransRunNum)));
  }
  QList<QVariant> secondTransRunNums;
  for (const auto &secondTransRunNum : transRunPair.secondTransmissionRunNumbers()) {
    secondTransRunNums.append(QVariant(QString::fromStdString(secondTransRunNum)));
  }
  QMap<QString, QVariant> transmissionMap;
  transmissionMap.insert(QString("firstTransRuns"), QVariant(firstTransRunNums));
  transmissionMap.insert(QString("secondTransRuns"), QVariant(secondTransRunNums));
  return transmissionMap;
}

QMap<QString, QVariant> Encoder::encodeReductionWorkspace(const ReductionWorkspaces &redWs) {
  QList<QVariant> inputRunNumbers;
  for (const auto &inputRunNum : redWs.m_inputRunNumbers) {
    inputRunNumbers.append(QVariant(QString::fromStdString(inputRunNum)));
  }
  QMap<QString, QVariant> reductionMap;
  reductionMap.insert(QString("inputRunNumbers"), QVariant(inputRunNumbers));
  reductionMap.insert(QString("transPair"), QVariant(encodeTransmissionRunPair(redWs.m_transmissionRuns)));
  reductionMap.insert(QString("iVsLambda"), QVariant(QString::fromStdString(redWs.m_iVsLambda)));
  reductionMap.insert(QString("iVsQ"), QVariant(QString::fromStdString(redWs.m_iVsQ)));
  reductionMap.insert(QString("iVsQBinned"), QVariant(QString::fromStdString(redWs.m_iVsQBinned)));
  return reductionMap;
}

QMap<QString, QVariant> Encoder::encodeReductionOptions(const ReductionOptionsMap &rom) {
  QMap<QString, QVariant> reductionOptionsMap;
  for (const auto &elem : rom) {
    reductionOptionsMap.insert(QString::fromStdString(elem.first), QVariant(QString::fromStdString(elem.second)));
  }
  return reductionOptionsMap;
}

QMap<QString, QVariant> Encoder::encodeRow(const MantidQt::CustomInterfaces::ISISReflectometry::Row &row) {
  QMap<QString, QVariant> rowMap;
  rowMap.insert(QString("itemState"), QVariant(static_cast<int>(row.state())));
  QList<QVariant> runNumbers;
  for (const auto &runNumber : row.m_runNumbers) {
    runNumbers.append(QVariant(QString::fromStdString(runNumber)));
  }
  rowMap.insert(QString("runNumbers"), QVariant(runNumbers));
  rowMap.insert(QString("theta"), QVariant(row.m_theta));
  rowMap.insert(QString("qRange"), QVariant(encodeRangeInQ(row.m_qRange)));
  rowMap.insert(QString("qRangeOutput"), QVariant(encodeRangeInQ(row.m_qRangeOutput)));
  rowMap.insert(QString("scaleFactorPresent"), QVariant(static_cast<bool>(row.m_scaleFactor)));
  if (row.m_scaleFactor) {
    rowMap.insert(QString("scaleFactor"), QVariant(row.m_scaleFactor.get()));
  }
  rowMap.insert(QString("transRunNums"), QVariant(encodeTransmissionRunPair(row.m_transmissionRuns)));
  rowMap.insert(QString("reductionWorkspaces"), QVariant(encodeReductionWorkspace(row.m_reducedWorkspaceNames)));
  rowMap.insert(QString("reductionOptions"), QVariant(encodeReductionOptions(row.m_reductionOptions)));
  return rowMap;
}

QList<QVariant> Encoder::encodeSearchModel(const ISearchModel &searchModel) {
  QList<QVariant> list;
  auto const &rows = searchModel.getRows();
  for (const auto &row : rows)
    list.append(QVariant(encodeSearchResult(row)));
  return list;
}

QMap<QString, QVariant> Encoder::encodeSearchResult(const SearchResult &row) {
  QMap<QString, QVariant> searchResultMap;
  searchResultMap.insert(QString("runNumber"), QVariant(QString::fromStdString(row.runNumber())));
  searchResultMap.insert(QString("title"), QVariant(QString::fromStdString(row.title())));
  searchResultMap.insert(QString("groupName"), QVariant(QString::fromStdString(row.groupName())));
  searchResultMap.insert(QString("theta"), QVariant(QString::fromStdString(row.theta())));
  searchResultMap.insert(QString("error"), QVariant(QString::fromStdString(row.error())));
  searchResultMap.insert(QString("excludeReason"), QVariant(QString::fromStdString(row.excludeReason())));
  searchResultMap.insert(QString("comment"), QVariant(QString::fromStdString(row.comment())));
  return searchResultMap;
}

QMap<QString, QVariant> Encoder::encodeEvent(const QtEventView *gui) {
  QMap<QString, QVariant> eventMap;
  eventMap.insert(QString("disabledSlicingButton"), QVariant(gui->m_ui.disabledSlicingButton->isChecked()));

  // Uniform Slicing
  eventMap.insert(QString("uniformEvenButton"), QVariant(gui->m_ui.uniformEvenButton->isChecked()));
  eventMap.insert(QString("uniformEvenEdit"), QVariant(gui->m_ui.uniformEvenEdit->value()));
  eventMap.insert(QString("uniformButton"), QVariant(gui->m_ui.uniformButton->isChecked()));
  eventMap.insert(QString("uniformEdit"), QVariant(gui->m_ui.uniformEdit->value()));

  // Custom Slicing
  eventMap.insert(QString("customButton"), QVariant(gui->m_ui.customButton->isChecked()));
  eventMap.insert(QString("customEdit"), QVariant(gui->m_ui.customEdit->text()));

  // Slicing by log value
  eventMap.insert(QString("logValueButton"), QVariant(gui->m_ui.logValueButton->isChecked()));
  eventMap.insert(QString("logValueEdit"), QVariant(gui->m_ui.logValueEdit->text()));
  eventMap.insert(QString("logValueTypeEdit"), QVariant(gui->m_ui.logValueTypeEdit->text()));
  return eventMap;
}

QMap<QString, QVariant> Encoder::encodeInstrument(const QtInstrumentView *gui) {
  QMap<QString, QVariant> instrumentMap;
  instrumentMap.insert(QString("intMonCheckBox"), QVariant(gui->m_ui.intMonCheckBox->isChecked()));
  instrumentMap.insert(QString("monIntMinEdit"), QVariant(gui->m_ui.monIntMinEdit->value()));
  instrumentMap.insert(QString("monIntMaxEdit"), QVariant(gui->m_ui.monIntMaxEdit->value()));
  instrumentMap.insert(QString("monBgMinEdit"), QVariant(gui->m_ui.monBgMinEdit->value()));
  instrumentMap.insert(QString("monBgMaxEdit"), QVariant(gui->m_ui.monBgMaxEdit->value()));
  instrumentMap.insert(QString("lamMinEdit"), QVariant(gui->m_ui.lamMinEdit->value()));
  instrumentMap.insert(QString("lamMaxEdit"), QVariant(gui->m_ui.lamMaxEdit->value()));
  instrumentMap.insert(QString("I0MonitorIndex"), QVariant(gui->m_ui.I0MonitorIndex->value()));
  instrumentMap.insert(QString("correctDetectorsCheckBox"), QVariant(gui->m_ui.correctDetectorsCheckBox->isChecked()));
  instrumentMap.insert(QString("detectorCorrectionTypeComboBox"),
                       QVariant(gui->m_ui.detectorCorrectionTypeComboBox->currentIndex()));
  instrumentMap.insert(QString("calibrationPathEdit"), QVariant(gui->m_ui.calibrationPathEdit->text()));
  return instrumentMap;
}

QMap<QString, QVariant> Encoder::encodeExperiment(const QtExperimentView *gui) {
  QMap<QString, QVariant> experimentMap;
  experimentMap.insert(QString("analysisModeComboBox"), QVariant(gui->m_ui.analysisModeComboBox->currentIndex()));
  experimentMap.insert(QString("debugCheckbox"), QVariant(gui->m_ui.debugCheckBox->isChecked()));
  experimentMap.insert(QString("summationTypeComboBox"), QVariant(gui->m_ui.summationTypeComboBox->currentIndex()));
  experimentMap.insert(QString("reductionTypeComboBox"), QVariant(gui->m_ui.reductionTypeComboBox->currentIndex()));
  experimentMap.insert(QString("includePartialBinsCheckBox"),
                       QVariant(gui->m_ui.includePartialBinsCheckBox->isChecked()));
  experimentMap.insert(QString("perAngleDefaults"), QVariant(encodePerAngleDefaults(gui->m_ui.optionsTable)));
  experimentMap.insert(QString("startOverlapEdit"), QVariant(gui->m_ui.startOverlapEdit->value()));
  experimentMap.insert(QString("endOverlapEdit"), QVariant(gui->m_ui.endOverlapEdit->value()));
  experimentMap.insert(QString("transStitchParamsEdit"), QVariant(gui->m_ui.transStitchParamsEdit->text()));
  experimentMap.insert(QString("transScaleRHSCheckBox"), QVariant(gui->m_ui.transScaleRHSCheckBox->isChecked()));
  experimentMap.insert(QString("subtractBackgroundCheckBox"),
                       QVariant(gui->m_ui.subtractBackgroundCheckBox->isChecked()));
  experimentMap.insert(QString("backgroundMethodComboBox"),
                       QVariant(gui->m_ui.backgroundMethodComboBox->currentIndex()));
  experimentMap.insert(QString("polynomialDegreeSpinBox"), QVariant(gui->m_ui.polynomialDegreeSpinBox->value()));
  experimentMap.insert(QString("costFunctionComboBox"), QVariant(gui->m_ui.costFunctionComboBox->currentIndex()));
  experimentMap.insert(QString("polCorrComboBox"), QVariant(gui->m_ui.polCorrComboBox->currentText()));
  experimentMap.insert(QString("polCorrEfficienciesWsSelector"),
                       QVariant(gui->m_polCorrEfficienciesWsSelector->currentText()));
  experimentMap.insert(QString("polCorrEfficienciesLineEdit"), QVariant(gui->m_polCorrEfficienciesLineEdit->text()));
  experimentMap.insert(QString("polCorrFredrikzeSpinStateEdit"),
                       QVariant(gui->m_ui.polCorrFredrikzeSpinStateEdit->text()));
  experimentMap.insert(QString("floodCorComboBox"), QVariant(gui->m_ui.floodCorComboBox->currentIndex()));
  experimentMap.insert(QString("floodWorkspaceWsSelector"), QVariant(gui->m_floodCorrWsSelector->currentText()));
  experimentMap.insert(QString("floodWorkspaceLineEdit"), QVariant(gui->m_floodCorrLineEdit->text()));
  experimentMap.insert(QString("stitchEdit"), QVariant(gui->m_stitchEdit->text()));
  return experimentMap;
}

QMap<QString, QVariant> Encoder::encodePerAngleDefaults(const QTableWidget *tab) {
  QMap<QString, QVariant> defaultsMap;
  const int rowsNum = tab->rowCount();
  const int columnNum = tab->columnCount();
  defaultsMap.insert(QString("rowsNum"), QVariant(rowsNum));
  defaultsMap.insert(QString("columnsNum"), QVariant(columnNum));
  defaultsMap.insert(QString("rows"), QVariant(encodePerAngleDefaultsRows(tab, rowsNum, columnNum)));
  return defaultsMap;
}

QList<QVariant> Encoder::encodePerAngleDefaultsRows(const QTableWidget *tab, int rowsNum, int columnsNum) {
  QList<QVariant> rows;
  for (auto rowIndex = 0; rowIndex < rowsNum; ++rowIndex) {
    rows.append(QVariant(encodePerAngleDefaultsRow(tab, rowIndex, columnsNum)));
  }
  return rows;
}

QList<QVariant> Encoder::encodePerAngleDefaultsRow(const QTableWidget *tab, int rowIndex, int columnsNum) {
  QList<QVariant> row;
  for (auto columnIndex = 0; columnIndex < columnsNum; ++columnIndex) {
    auto text = tab->item(rowIndex, columnIndex)->text();
    row.append(QVariant(text));
  }
  return row;
}

QMap<QString, QVariant> Encoder::encodeSave(const QtSaveView *gui) {
  QMap<QString, QVariant> saveMap;
  saveMap.insert(QString("savePathEdit"), QVariant(gui->m_ui.savePathEdit->text()));
  saveMap.insert(QString("prefixEdit"), QVariant(gui->m_ui.prefixEdit->text()));
  saveMap.insert(QString("headerCheckBox"), QVariant(gui->m_ui.headerCheckBox->isChecked()));
  saveMap.insert(QString("qResolutionCheckBox"), QVariant(gui->m_ui.qResolutionCheckBox->isChecked()));
  saveMap.insert(QString("extraColumnsCheckBox"), QVariant(gui->m_ui.extraColumnsCheckBox->isChecked()));
  saveMap.insert(QString("multipleDatasetsCheckBox"), QVariant(gui->m_ui.multipleDatasetsCheckBox->isChecked()));
  saveMap.insert(QString("commaRadioButton"), QVariant(gui->m_ui.commaRadioButton->isChecked()));
  saveMap.insert(QString("spaceRadioButton"), QVariant(gui->m_ui.spaceRadioButton->isChecked()));
  saveMap.insert(QString("tabRadioButton"), QVariant(gui->m_ui.tabRadioButton->isChecked()));
  saveMap.insert(QString("fileFormatComboBox"), QVariant(gui->m_ui.fileFormatComboBox->currentIndex()));
  saveMap.insert(QString("filterEdit"), QVariant(gui->m_ui.filterEdit->text()));
  saveMap.insert(QString("regexCheckBox"), QVariant(gui->m_ui.regexCheckBox->isChecked()));
  saveMap.insert(QString("saveReductionResultsCheckBox"),
                 QVariant(gui->m_ui.saveReductionResultsCheckBox->isChecked()));
  saveMap.insert(QString("saveIndividualRowsCheckBox"), QVariant(gui->m_ui.saveIndividualRowsCheckBox->isChecked()));
  return saveMap;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
