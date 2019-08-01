// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "Encoder.h"
#include "../RunsTable/RunsTablePresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
QMap<QString, QVariant> Encoder::encode(const MainWindowView &gui) {
  QMap<QString, QVariant> map;
  QList<QVariant> batches;
  for (const auto &batchView : gui.m_batchViews) {
    batches.append(QVariant(
        encodeBatch(dynamic_cast<const BatchView *>(batchView), gui, true)));
  }
  map.insert(QString("batches"), QVariant(batches));
  return map;
}

BatchPresenter *Encoder::findBatchPresenter(const BatchView *gui,
                                            const MainWindowView &mwv) {
  for (auto ipresenter : mwv.m_presenter->m_batchPresenters) {
    auto presenter = std::dynamic_pointer_cast<BatchPresenter>(ipresenter);
    if (presenter->m_view == gui) {
      return presenter.get();
    }
  }
  return nullptr;
}

QMap<QString, QVariant> Encoder::encodeBatch(const BatchView *gui,
                                             const MainWindowView &mwv,
                                             bool projectSave,
                                             const BatchPresenter *presenter) {
  auto batchPresenter = presenter;
  if (!batchPresenter) {
    batchPresenter = findBatchPresenter(gui, mwv);
  }
  if (!batchPresenter) {
    throw std::runtime_error(
        "BatchPresenter could not be found during encode.");
  }
  auto runsPresenter =
      dynamic_cast<RunsPresenter *>(batchPresenter->m_runsPresenter.get());
  auto runsTablePresenter =
      dynamic_cast<RunsTablePresenter *>(runsPresenter->m_tablePresenter.get());
  auto reductionJobs = &runsTablePresenter->m_model.m_reductionJobs;

  QMap<QString, QVariant> map;
  map.insert(
      QString("runsView"),
      QVariant(encodeRuns(gui->m_runs.get(), projectSave, reductionJobs)));
  map.insert(QString("eventView"),
             QVariant(encodeEvent(gui->m_eventHandling.get())));
  map.insert(QString("experimentView"),
             QVariant(encodeExperiment(gui->m_experiment.get())));
  map.insert(QString("instrumentView"),
             QVariant(encodeInstrument(gui->m_instrument.get())));
  map.insert(QString("saveView"), QVariant(encodeSave(gui->m_save.get())));
  return map;
}

QMap<QString, QVariant> Encoder::encodeBatch(const IBatchPresenter *presenter,
                                             const IMainWindowView *mwv,
                                             bool projectSave) {
  auto batchPresenter = dynamic_cast<const BatchPresenter *>(presenter);
  return encodeBatch(dynamic_cast<BatchView *>(batchPresenter->m_view),
                     *dynamic_cast<const MainWindowView *>(mwv), projectSave,
                     batchPresenter);
}

QMap<QString, QVariant> Encoder::encodeRuns(const RunsView *gui,
                                            bool projectSave,
                                            const ReductionJobs *redJobs) {
  QMap<QString, QVariant> map;
  map.insert(QString("runsTable"),
             QVariant(encodeRunsTable(gui->m_tableView, projectSave, redJobs)));
  map.insert(QString("comboSearchInstrument"),
             QVariant(gui->m_ui.comboSearchInstrument->currentIndex()));
  map.insert(QString("textSearch"), QVariant(gui->m_ui.textSearch->text()));
  return map;
}

QMap<QString, QVariant> Encoder::encodeRunsTable(const RunsTableView *gui,
                                                 bool projectSave,
                                                 const ReductionJobs *redJobs) {
  QMap<QString, QVariant> map;
  map.insert(QString("filterBox"), QVariant(gui->m_ui.filterBox->text()));

  map.insert(QString("projectSave"), QVariant(projectSave));
  map.insert(QString("runsTableModel"),
             QVariant(encodeRunsTableModel(redJobs)));
  return map;
}

QList<QVariant> Encoder::encodeRunsTableModel(const ReductionJobs *redJobs) {
  QList<QVariant> groups;
  for (const auto &group : redJobs->groups()) {
    groups.append(QVariant(encodeGroup(group)));
  }
  return groups;
}

QMap<QString, QVariant>
Encoder::encodeGroup(const MantidQt::CustomInterfaces::Group &group) {
  QMap<QString, QVariant> map;
  map.insert(QString("name"), QVariant(QString::fromStdString(group.m_name)));
  map.insert(
      QString("postprocessedWorkspaceName"),
      QVariant(QString::fromStdString(group.m_postprocessedWorkspaceName)));
  map.insert(QString("rows"), QVariant(encodeRows(group)));
  return map;
}

QList<QVariant>
Encoder::encodeRows(const MantidQt::CustomInterfaces::Group &group) {
  QList<QVariant> rows;
  for (const auto &row : group.m_rows) {
    if (row) {
      rows.append(encodeRow(row.get()));
    } else {
      rows.append(QList<QVariant>());
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

QMap<QString, QVariant>
Encoder::encodeTransmissionRunPair(const TransmissionRunPair &transRunPair) {
  QList<QVariant> firstTransRunNums;
  for (const auto firstTransRunNum :
       transRunPair.m_firstTransmissionRunNumbers) {
    firstTransRunNums.append(
        QVariant(QString::fromStdString(firstTransRunNum)));
  }
  QList<QVariant> secondTransRunNums;
  for (const auto secondTransRunNum :
       transRunPair.m_secondTransmissionRunNumbers) {
    secondTransRunNums.append(
        QVariant(QString::fromStdString(secondTransRunNum)));
  }
  QMap<QString, QVariant> map;
  map.insert(QString("firstTransRuns"), QVariant(firstTransRunNums));
  map.insert(QString("secondTransRuns"), QVariant(secondTransRunNums));
  return map;
}

QMap<QString, QVariant>
Encoder::encodeReductionWorkspace(const ReductionWorkspaces &redWs) {
  QList<QVariant> inputRunNumbers;
  for (const auto &inputRunNum : redWs.m_inputRunNumbers) {
    inputRunNumbers.append(QVariant(QString::fromStdString(inputRunNum)));
  }
  QMap<QString, QVariant> map;
  map.insert(QString("inputRunNumbers"), QVariant(inputRunNumbers));
  map.insert(QString("transPair"),
             QVariant(encodeTransmissionRunPair(redWs.m_transmissionRuns)));
  map.insert(QString("iVsLambda"),
             QVariant(QString::fromStdString(redWs.m_iVsLambda)));
  map.insert(QString("iVsQ"), QVariant(QString::fromStdString(redWs.m_iVsQ)));
  map.insert(QString("iVsQBinned"),
             QVariant(QString::fromStdString(redWs.m_iVsQBinned)));
  return map;
}

QMap<QString, QVariant>
Encoder::encodeReductionOptions(const ReductionOptionsMap &rom) {
  QMap<QString, QVariant> map;
  for (const auto &elem : rom) {
    map.insert(QString::fromStdString(elem.first),
               QVariant(QString::fromStdString(elem.second)));
  }
  return map;
}

QMap<QString, QVariant>
Encoder::encodeRow(const MantidQt::CustomInterfaces::Row &row) {
  QMap<QString, QVariant> map;
  QList<QVariant> runNumbers;
  for (const auto &runNumber : row.m_runNumbers) {
    runNumbers.append(QVariant(QString::fromStdString(runNumber)));
  }
  map.insert(QString("runNumbers"), QVariant(runNumbers));
  map.insert(QString("theta"), QVariant(row.m_theta));
  map.insert(QString("qRange"), QVariant(encodeRangeInQ(row.m_qRange)));
  map.insert(QString("qRangeOutput"),
             QVariant(encodeRangeInQ(row.m_qRangeOutput)));
  map.insert(QString("scaleFactorPresent"),
             QVariant(static_cast<bool>(row.m_scaleFactor)));
  if (row.m_scaleFactor) {
    map.insert(QString("scaleFactor"), QVariant(row.m_scaleFactor.get()));
  }
  map.insert(QString("transRunNums"),
             QVariant(encodeTransmissionRunPair(row.m_transmissionRuns)));
  map.insert(QString("reductionWorkspaces"),
             QVariant(encodeReductionWorkspace(row.m_reducedWorkspaceNames)));
  map.insert(QString("reductionOptions"),
             QVariant(encodeReductionOptions(row.m_reductionOptions)));
  return map;
}

QMap<QString, QVariant> Encoder::encodeEvent(const EventView *gui) {
  QMap<QString, QVariant> map;
  map.insert(QString("disabledSlicingButton"),
             QVariant(gui->m_ui.disabledSlicingButton->isChecked()));

  // Uniform Slicing
  map.insert(QString("uniformEvenButton"),
             QVariant(gui->m_ui.uniformEvenButton->isChecked()));
  map.insert(QString("uniformEvenEdit"),
             QVariant(gui->m_ui.uniformEvenEdit->text()));
  map.insert(QString("uniformButton"),
             QVariant(gui->m_ui.uniformButton->isChecked()));
  map.insert(QString("uniformEdit"), QVariant(gui->m_ui.uniformEdit->text()));

  // Custom Slicing
  map.insert(QString("customButton"),
             QVariant(gui->m_ui.customButton->isChecked()));
  map.insert(QString("customEdit"), QVariant(gui->m_ui.customEdit->text()));

  // Slicing by log value
  map.insert(QString("logValueButton"),
             QVariant(gui->m_ui.logValueButton->isChecked()));
  map.insert(QString("logValueEdit"), QVariant(gui->m_ui.logValueEdit->text()));
  map.insert(QString("logValueTypeEdit"),
             QVariant(gui->m_ui.logValueTypeEdit->text()));
  return map;
}

QMap<QString, QVariant> Encoder::encodeInstrument(const InstrumentView *gui) {
  QMap<QString, QVariant> map;
  map.insert(QString("intMonCheckBox"),
             QVariant(gui->m_ui.intMonCheckBox->isChecked()));
  map.insert(QString("monIntMinEdit"),
             QVariant(gui->m_ui.monIntMinEdit->text()));
  map.insert(QString("monIntMaxEdit"),
             QVariant(gui->m_ui.monIntMaxEdit->text()));
  map.insert(QString("monBgMinEdit"), QVariant(gui->m_ui.monBgMinEdit->text()));
  map.insert(QString("monBgMaxEdit"), QVariant(gui->m_ui.monBgMaxEdit->text()));
  map.insert(QString("lamMinEdit"), QVariant(gui->m_ui.lamMinEdit->text()));
  map.insert(QString("lamMaxEdit"), QVariant(gui->m_ui.lamMaxEdit->text()));
  map.insert(QString("I0MonitorIndex"),
             QVariant(gui->m_ui.I0MonitorIndex->text()));
  map.insert(QString("correctDetectorsCheckBox"),
             QVariant(gui->m_ui.correctDetectorsCheckBox->isChecked()));
  map.insert(
      QString("detectorCorrectionTypeComboBox"),
      QVariant(gui->m_ui.detectorCorrectionTypeComboBox->currentIndex()));
  return map;
}

QMap<QString, QVariant> Encoder::encodeExperiment(const ExperimentView *gui) {
  QMap<QString, QVariant> map;
  map.insert(QString("analysisModeComboBox"),
             QVariant(gui->m_ui.analysisModeComboBox->currentIndex()));
  map.insert(QString("debugCheckbox"),
             QVariant(gui->m_ui.debugCheckBox->isChecked()));
  map.insert(QString("summationTypeComboBox"),
             QVariant(gui->m_ui.summationTypeComboBox->currentIndex()));
  map.insert(QString("reductionTypeComboBox"),
             QVariant(gui->m_ui.reductionTypeComboBox->currentIndex()));
  map.insert(QString("includePartialBinsCheckBox"),
             QVariant(gui->m_ui.includePartialBinsCheckBox->isChecked()));
  map.insert(QString("perAngleDefaults"),
             QVariant(encodePerAngleDefaults(gui->m_ui.optionsTable)));
  map.insert(QString("startOverlapEdit"),
             QVariant(gui->m_ui.startOverlapEdit->text()));
  map.insert(QString("endOverlapEdit"),
             QVariant(gui->m_ui.endOverlapEdit->text()));
  map.insert(QString("transStitchParamsEdit"),
             QVariant(gui->m_ui.transStitchParamsEdit->text()));
  map.insert(QString("transScaleRHSCheckBox"),
             QVariant(gui->m_ui.transScaleRHSCheckBox->isChecked()));
  map.insert(QString("polCorrCheckBox"),
             QVariant(gui->m_ui.polCorrCheckBox->isChecked()));
  map.insert(QString("floodCorComboBox"),
             QVariant(gui->m_ui.floodCorComboBox->currentIndex()));
  map.insert(QString("floodWorkspaceWsSelector"),
             QVariant(gui->m_ui.floodWorkspaceWsSelector->currentIndex()));
  map.insert(QString("stitchEdit"), QVariant(gui->m_stitchEdit->text()));
  return map;
}

QMap<QString, QVariant>
Encoder::encodePerAngleDefaults(const QTableWidget *tab) {
  QMap<QString, QVariant> map;
  const int rowsNum = tab->rowCount();
  const int columnNum = tab->columnCount();
  map.insert(QString("rowsNum"), QVariant(rowsNum));
  map.insert(QString("columnsNum"), QVariant(columnNum));
  map.insert(QString("rows"),
             QVariant(encodePerAngleDefaultsRow(tab, rowsNum - 1, columnNum)));
  return map;
}

QList<QVariant> Encoder::encodePerAngleDefaultsRows(const QTableWidget *tab,
                                                    int rowsNum,
                                                    int columnsNum) {
  QList<QVariant> rows;
  for (auto rowIndex = 0; rowIndex < rowsNum; ++rowIndex) {
    rows.append(QVariant(encodePerAngleDefaultsRow(tab, rowIndex, columnsNum)));
  }
  return rows;
}

QList<QVariant> Encoder::encodePerAngleDefaultsRow(const QTableWidget *tab,
                                                   int rowIndex,
                                                   int columnsNum) {
  QList<QVariant> row;
  for (auto columnIndex = 0; columnIndex < columnsNum; ++columnIndex) {
    row.append(QVariant(tab->item(rowIndex, columnIndex)->text()));
  }
  return row;
}

QMap<QString, QVariant> Encoder::encodeSave(const SaveView *gui) {
  QMap<QString, QVariant> map;
  map.insert(QString("savePathEdit"), QVariant(gui->m_ui.savePathEdit->text()));
  map.insert(QString("prefixEdit"), QVariant(gui->m_ui.prefixEdit->text()));
  map.insert(QString("titleCheckBox"),
             QVariant(gui->m_ui.titleCheckBox->isChecked()));
  map.insert(QString("qResolutionCheckBox"),
             QVariant(gui->m_ui.qResolutionCheckBox->isChecked()));
  map.insert(QString("commaRadioButton"),
             QVariant(gui->m_ui.commaRadioButton->isChecked()));
  map.insert(QString("spaceRadioButton"),
             QVariant(gui->m_ui.spaceRadioButton->isChecked()));
  map.insert(QString("tabRadioButton"),
             QVariant(gui->m_ui.tabRadioButton->isChecked()));
  map.insert(QString("fileFormatComboBox"),
             QVariant(gui->m_ui.fileFormatComboBox->currentIndex()));
  map.insert(QString("filterEdit"), QVariant(gui->m_ui.filterEdit->text()));
  map.insert(QString("regexCheckBox"),
             QVariant(gui->m_ui.regexCheckBox->isChecked()));
  map.insert(QString("saveReductionResultsCheckBox"),
             QVariant(gui->m_ui.saveReductionResultsCheckBox->isChecked()));
  return map;
}
} // namespace CustomInterfaces
} // namespace MantidQt