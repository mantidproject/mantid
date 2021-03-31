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

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

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
  QMap<QString, QVariant> map;
  map.insert(QString("tag"), QVariant(QString("ISIS Reflectometry")));
  QList<QVariant> batches;
  for (size_t batchIndex = 0; batchIndex < mwv->batches().size(); ++batchIndex) {
    batches.append(QVariant(encodeBatch(mwv, static_cast<int>(batchIndex), true)));
  }
  map.insert(QString("batches"), QVariant(batches));
  return map;
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

  QMap<QString, QVariant> map;
  map.insert(QString("runsView"), QVariant(encodeRuns(gui->m_runs.get(), projectSave, reductionJobs, searcher)));
  map.insert(QString("eventView"), QVariant(encodeEvent(gui->m_eventHandling.get())));
  map.insert(QString("experimentView"), QVariant(encodeExperiment(gui->m_experiment.get())));
  map.insert(QString("instrumentView"), QVariant(encodeInstrument(gui->m_instrument.get())));
  map.insert(QString("saveView"), QVariant(encodeSave(gui->m_save.get())));
  return map;
}

QMap<QString, QVariant> Encoder::encodeRuns(const QtRunsView *gui, bool projectSave, const ReductionJobs *redJobs,
                                            QtCatalogSearcher *searcher) {
  QMap<QString, QVariant> map;
  map.insert(QString("runsTable"), QVariant(encodeRunsTable(gui->m_tableView, projectSave, redJobs)));
  map.insert(QString("comboSearchInstrument"), QVariant(gui->m_ui.comboSearchInstrument->currentIndex()));
  // This is not ideal but the search criteria may be changed on the view and
  // no longer be relevant for the search results. The latter are more
  // important so use the cached search criteria, i.e. only save the search
  // criteria if they have been used to perform a search
  map.insert(QString("textSearch"), QVariant(QString::fromStdString(searcher->searchCriteria().investigation)));
  map.insert(QString("textCycle"), QVariant(QString::fromStdString(searcher->searchCriteria().cycle)));
  map.insert(QString("textInstrument"), QVariant(QString::fromStdString(searcher->searchCriteria().instrument)));
  map.insert(QString("searchResults"), QVariant(encodeSearchModel(gui->searchResults())));
  return map;
}

QMap<QString, QVariant> Encoder::encodeRunsTable(const QtRunsTableView *gui, bool projectSave,
                                                 const ReductionJobs *redJobs) {
  QMap<QString, QVariant> map;
  map.insert(QString("filterBox"), QVariant(gui->m_ui.filterBox->text()));

  map.insert(QString("projectSave"), QVariant(projectSave));
  map.insert(QString("runsTableModel"), QVariant(encodeRunsTableModel(redJobs)));
  return map;
}

QList<QVariant> Encoder::encodeRunsTableModel(const ReductionJobs *redJobs) {
  QList<QVariant> groups;
  for (const auto &group : redJobs->groups()) {
    groups.append(QVariant(encodeGroup(group)));
  }
  return groups;
}

QMap<QString, QVariant> Encoder::encodeGroup(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group) {
  QMap<QString, QVariant> map;
  map.insert(QString("name"), QVariant(QString::fromStdString(group.m_name)));
  map.insert(QString("itemState"), QVariant(static_cast<int>(group.state())));
  map.insert(QString("postprocessedWorkspaceName"),
             QVariant(QString::fromStdString(group.m_postprocessedWorkspaceName)));
  map.insert(QString("rows"), QVariant(encodeRows(group)));
  return map;
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
  QMap<QString, QVariant> map;
  auto min = rangeInQ.min();
  auto max = rangeInQ.max();
  auto step = rangeInQ.step();
  map.insert(QString("minPresent"), QVariant(static_cast<bool>(min)));
  if (min)
    map.insert(QString("min"), QVariant(min.get()));
  map.insert(QString("maxPresent"), QVariant(static_cast<bool>(max)));
  if (max)
    map.insert(QString("max"), QVariant(max.get()));
  map.insert(QString("stepPresent"), QVariant(static_cast<bool>(step)));
  if (step)
    map.insert(QString("step"), QVariant(step.get()));
  return map;
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
  QMap<QString, QVariant> map;
  map.insert(QString("firstTransRuns"), QVariant(firstTransRunNums));
  map.insert(QString("secondTransRuns"), QVariant(secondTransRunNums));
  return map;
}

QMap<QString, QVariant> Encoder::encodeReductionWorkspace(const ReductionWorkspaces &redWs) {
  QList<QVariant> inputRunNumbers;
  for (const auto &inputRunNum : redWs.m_inputRunNumbers) {
    inputRunNumbers.append(QVariant(QString::fromStdString(inputRunNum)));
  }
  QMap<QString, QVariant> map;
  map.insert(QString("inputRunNumbers"), QVariant(inputRunNumbers));
  map.insert(QString("transPair"), QVariant(encodeTransmissionRunPair(redWs.m_transmissionRuns)));
  map.insert(QString("iVsLambda"), QVariant(QString::fromStdString(redWs.m_iVsLambda)));
  map.insert(QString("iVsQ"), QVariant(QString::fromStdString(redWs.m_iVsQ)));
  map.insert(QString("iVsQBinned"), QVariant(QString::fromStdString(redWs.m_iVsQBinned)));
  return map;
}

QMap<QString, QVariant> Encoder::encodeReductionOptions(const ReductionOptionsMap &rom) {
  QMap<QString, QVariant> map;
  for (const auto &elem : rom) {
    map.insert(QString::fromStdString(elem.first), QVariant(QString::fromStdString(elem.second)));
  }
  return map;
}

QMap<QString, QVariant> Encoder::encodeRow(const MantidQt::CustomInterfaces::ISISReflectometry::Row &row) {
  QMap<QString, QVariant> map;
  map.insert(QString("itemState"), QVariant(static_cast<int>(row.state())));
  QList<QVariant> runNumbers;
  for (const auto &runNumber : row.m_runNumbers) {
    runNumbers.append(QVariant(QString::fromStdString(runNumber)));
  }
  map.insert(QString("runNumbers"), QVariant(runNumbers));
  map.insert(QString("theta"), QVariant(row.m_theta));
  map.insert(QString("qRange"), QVariant(encodeRangeInQ(row.m_qRange)));
  map.insert(QString("qRangeOutput"), QVariant(encodeRangeInQ(row.m_qRangeOutput)));
  map.insert(QString("scaleFactorPresent"), QVariant(static_cast<bool>(row.m_scaleFactor)));
  if (row.m_scaleFactor) {
    map.insert(QString("scaleFactor"), QVariant(row.m_scaleFactor.get()));
  }
  map.insert(QString("transRunNums"), QVariant(encodeTransmissionRunPair(row.m_transmissionRuns)));
  map.insert(QString("reductionWorkspaces"), QVariant(encodeReductionWorkspace(row.m_reducedWorkspaceNames)));
  map.insert(QString("reductionOptions"), QVariant(encodeReductionOptions(row.m_reductionOptions)));
  return map;
}

QList<QVariant> Encoder::encodeSearchModel(const ISearchModel &searchModel) {
  QList<QVariant> list;
  auto const &rows = searchModel.getRows();
  for (const auto &row : rows)
    list.append(QVariant(encodeSearchResult(row)));
  return list;
}

QMap<QString, QVariant> Encoder::encodeSearchResult(const SearchResult &row) {
  QMap<QString, QVariant> map;
  map.insert(QString("runNumber"), QVariant(QString::fromStdString(row.runNumber())));
  map.insert(QString("title"), QVariant(QString::fromStdString(row.title())));
  map.insert(QString("groupName"), QVariant(QString::fromStdString(row.groupName())));
  map.insert(QString("theta"), QVariant(QString::fromStdString(row.theta())));
  map.insert(QString("error"), QVariant(QString::fromStdString(row.error())));
  map.insert(QString("excludeReason"), QVariant(QString::fromStdString(row.excludeReason())));
  map.insert(QString("comment"), QVariant(QString::fromStdString(row.comment())));
  return map;
}

QMap<QString, QVariant> Encoder::encodeEvent(const QtEventView *gui) {
  QMap<QString, QVariant> map;
  map.insert(QString("disabledSlicingButton"), QVariant(gui->m_ui.disabledSlicingButton->isChecked()));

  // Uniform Slicing
  map.insert(QString("uniformEvenButton"), QVariant(gui->m_ui.uniformEvenButton->isChecked()));
  map.insert(QString("uniformEvenEdit"), QVariant(gui->m_ui.uniformEvenEdit->value()));
  map.insert(QString("uniformButton"), QVariant(gui->m_ui.uniformButton->isChecked()));
  map.insert(QString("uniformEdit"), QVariant(gui->m_ui.uniformEdit->value()));

  // Custom Slicing
  map.insert(QString("customButton"), QVariant(gui->m_ui.customButton->isChecked()));
  map.insert(QString("customEdit"), QVariant(gui->m_ui.customEdit->text()));

  // Slicing by log value
  map.insert(QString("logValueButton"), QVariant(gui->m_ui.logValueButton->isChecked()));
  map.insert(QString("logValueEdit"), QVariant(gui->m_ui.logValueEdit->text()));
  map.insert(QString("logValueTypeEdit"), QVariant(gui->m_ui.logValueTypeEdit->text()));
  return map;
}

QMap<QString, QVariant> Encoder::encodeInstrument(const QtInstrumentView *gui) {
  QMap<QString, QVariant> map;
  map.insert(QString("intMonCheckBox"), QVariant(gui->m_ui.intMonCheckBox->isChecked()));
  map.insert(QString("monIntMinEdit"), QVariant(gui->m_ui.monIntMinEdit->value()));
  map.insert(QString("monIntMaxEdit"), QVariant(gui->m_ui.monIntMaxEdit->value()));
  map.insert(QString("monBgMinEdit"), QVariant(gui->m_ui.monBgMinEdit->value()));
  map.insert(QString("monBgMaxEdit"), QVariant(gui->m_ui.monBgMaxEdit->value()));
  map.insert(QString("lamMinEdit"), QVariant(gui->m_ui.lamMinEdit->value()));
  map.insert(QString("lamMaxEdit"), QVariant(gui->m_ui.lamMaxEdit->value()));
  map.insert(QString("I0MonitorIndex"), QVariant(gui->m_ui.I0MonitorIndex->value()));
  map.insert(QString("correctDetectorsCheckBox"), QVariant(gui->m_ui.correctDetectorsCheckBox->isChecked()));
  map.insert(QString("detectorCorrectionTypeComboBox"),
             QVariant(gui->m_ui.detectorCorrectionTypeComboBox->currentIndex()));
  return map;
}

QMap<QString, QVariant> Encoder::encodeExperiment(const QtExperimentView *gui) {
  QMap<QString, QVariant> map;
  map.insert(QString("analysisModeComboBox"), QVariant(gui->m_ui.analysisModeComboBox->currentIndex()));
  map.insert(QString("debugCheckbox"), QVariant(gui->m_ui.debugCheckBox->isChecked()));
  map.insert(QString("summationTypeComboBox"), QVariant(gui->m_ui.summationTypeComboBox->currentIndex()));
  map.insert(QString("reductionTypeComboBox"), QVariant(gui->m_ui.reductionTypeComboBox->currentIndex()));
  map.insert(QString("includePartialBinsCheckBox"), QVariant(gui->m_ui.includePartialBinsCheckBox->isChecked()));
  map.insert(QString("perAngleDefaults"), QVariant(encodePerAngleDefaults(gui->m_ui.optionsTable)));
  map.insert(QString("startOverlapEdit"), QVariant(gui->m_ui.startOverlapEdit->value()));
  map.insert(QString("endOverlapEdit"), QVariant(gui->m_ui.endOverlapEdit->value()));
  map.insert(QString("transStitchParamsEdit"), QVariant(gui->m_ui.transStitchParamsEdit->text()));
  map.insert(QString("transScaleRHSCheckBox"), QVariant(gui->m_ui.transScaleRHSCheckBox->isChecked()));
  map.insert(QString("subtractBackgroundCheckBox"), QVariant(gui->m_ui.subtractBackgroundCheckBox->isChecked()));
  map.insert(QString("backgroundMethodComboBox"), QVariant(gui->m_ui.backgroundMethodComboBox->currentIndex()));
  map.insert(QString("polynomialDegreeSpinBox"), QVariant(gui->m_ui.polynomialDegreeSpinBox->value()));
  map.insert(QString("costFunctionComboBox"), QVariant(gui->m_ui.costFunctionComboBox->currentIndex()));
  map.insert(QString("polCorrCheckBox"), QVariant(gui->m_ui.polCorrCheckBox->isChecked()));
  map.insert(QString("floodCorComboBox"), QVariant(gui->m_ui.floodCorComboBox->currentIndex()));
  map.insert(QString("floodWorkspaceWsSelector"), QVariant(gui->m_ui.floodWorkspaceWsSelector->currentIndex()));
  map.insert(QString("stitchEdit"), QVariant(gui->m_stitchEdit->text()));
  return map;
}

QMap<QString, QVariant> Encoder::encodePerAngleDefaults(const QTableWidget *tab) {
  QMap<QString, QVariant> map;
  const int rowsNum = tab->rowCount();
  const int columnNum = tab->columnCount();
  map.insert(QString("rowsNum"), QVariant(rowsNum));
  map.insert(QString("columnsNum"), QVariant(columnNum));
  map.insert(QString("rows"), QVariant(encodePerAngleDefaultsRows(tab, rowsNum, columnNum)));
  return map;
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
  QMap<QString, QVariant> map;
  map.insert(QString("savePathEdit"), QVariant(gui->m_ui.savePathEdit->text()));
  map.insert(QString("prefixEdit"), QVariant(gui->m_ui.prefixEdit->text()));
  map.insert(QString("headerCheckBox"), QVariant(gui->m_ui.headerCheckBox->isChecked()));
  map.insert(QString("qResolutionCheckBox"), QVariant(gui->m_ui.qResolutionCheckBox->isChecked()));
  map.insert(QString("commaRadioButton"), QVariant(gui->m_ui.commaRadioButton->isChecked()));
  map.insert(QString("spaceRadioButton"), QVariant(gui->m_ui.spaceRadioButton->isChecked()));
  map.insert(QString("tabRadioButton"), QVariant(gui->m_ui.tabRadioButton->isChecked()));
  map.insert(QString("fileFormatComboBox"), QVariant(gui->m_ui.fileFormatComboBox->currentIndex()));
  map.insert(QString("filterEdit"), QVariant(gui->m_ui.filterEdit->text()));
  map.insert(QString("regexCheckBox"), QVariant(gui->m_ui.regexCheckBox->isChecked()));
  map.insert(QString("saveReductionResultsCheckBox"), QVariant(gui->m_ui.saveReductionResultsCheckBox->isChecked()));
  return map;
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
