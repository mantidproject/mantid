// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "Decoder.h"
#include "../../Reduction/Group.h"
#include "../../Reduction/ReductionJobs.h"
#include "../../Reduction/ReductionWorkspaces.h"
#include "../../Reduction/Row.h"
#include "../Batch/BatchPresenter.h"
#include "../Batch/QtBatchView.h"
#include "../Experiment/QtExperimentView.h"
#include "../Instrument/QtInstrumentView.h"
#include "../MainWindow/QtMainWindowView.h"
#include "../Runs/QtRunsView.h"
#include "../Runs/RunsPresenter.h"
#include "../RunsTable/QtRunsTableView.h"
#include "../RunsTable/RunsTablePresenter.h"
#include "../Save/QtSaveView.h"
#include "Encoder.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QApplication>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
BatchPresenter *Decoder::findBatchPresenter(const QtBatchView *gui,
                                            const QtMainWindowView *mwv) {
  for (auto &ipresenter : mwv->m_presenter->m_batchPresenters) {
    auto presenter = dynamic_cast<BatchPresenter *>(ipresenter.get());
    if (presenter->m_view == gui) {
      return presenter;
    }
  }
  return nullptr;
}

QWidget *Decoder::decode(const QMap<QString, QVariant> &map,
                         const std::string &directory) {
  UNUSED_ARG(directory)
  auto userSubWindow =
      MantidQt::API::InterfaceManager().createSubWindow("ISIS Reflectometry");
  auto mwv = dynamic_cast<QtMainWindowView *>(userSubWindow);
  auto batches = map["batches"].toList();
  // Create the number of batches required.
  for (auto batchIndex = mwv->batches().size();
       batchIndex < static_cast<long unsigned int>(batches.size());
       ++batchIndex) {
    mwv->newBatch();
  }
  for (auto ii = 0; ii < batches.size(); ++ii) {
    decodeBatch(dynamic_cast<QtBatchView *>(mwv->m_batchViews[ii]), mwv,
                batches[ii].toMap());
  }
  return mwv;
}

QList<QString> Decoder::tags() {
  return QList<QString>({QString("ISIS Reflectometry")});
}

void Decoder::decodeBatch(const QtBatchView *gui, const QtMainWindowView *mwv,
                          const QMap<QString, QVariant> &map,
                          const BatchPresenter *presenter) {
  auto batchPresenter = presenter;
  if (!batchPresenter) {
    batchPresenter = findBatchPresenter(gui, mwv);
  }
  if (!batchPresenter) {
    throw std::runtime_error(
        "BatchPresenter could not be found during decode.");
  }
  auto runsPresenter =
      dynamic_cast<RunsPresenter *>(batchPresenter->m_runsPresenter.get());
  auto runsTablePresenter =
      dynamic_cast<RunsTablePresenter *>(runsPresenter->m_tablePresenter.get());
  auto reductionJobs = &runsTablePresenter->m_model.m_reductionJobs;
  decodeRuns(gui->m_runs.get(), reductionJobs, runsTablePresenter,
             map[QString("runsView")].toMap());
  decodeEvent(gui->m_eventHandling.get(), map[QString("eventView")].toMap());
  decodeExperiment(gui->m_experiment.get(),
                   map[QString("experimentView")].toMap());
  decodeInstrument(gui->m_instrument.get(),
                   map[QString("instrumentView")].toMap());
  decodeSave(gui->m_save.get(), map[QString("saveView")].toMap());
}

void Decoder::decodeBatch(const IBatchPresenter *presenter,
                          const IMainWindowView *mwv,
                          const QMap<QString, QVariant> &map) {
  auto batchPresenter = dynamic_cast<const BatchPresenter *>(presenter);
  decodeBatch(dynamic_cast<QtBatchView *>(batchPresenter->m_view),
              dynamic_cast<const QtMainWindowView *>(mwv), map, batchPresenter);
}

void Decoder::decodeExperiment(const QtExperimentView *gui,
                               const QMap<QString, QVariant> &map) {
  gui->m_ui.analysisModeComboBox->setCurrentIndex(
      map[QString("analysisModeComboBox")].toInt());
  gui->m_ui.debugCheckBox->setChecked(map[QString("debugCheckbox")].toBool());
  gui->m_ui.summationTypeComboBox->setCurrentIndex(
      map[QString("summationTypeComboBox")].toInt());
  gui->m_ui.reductionTypeComboBox->setCurrentIndex(
      map[QString("reductionTypeComboBox")].toInt());
  gui->m_ui.includePartialBinsCheckBox->setChecked(
      map[QString("includePartialBinsCheckBox")].toBool());
  decodePerAngleDefaults(gui->m_ui.optionsTable,
                         map[QString("perAngleDefaults")].toMap());
  gui->m_ui.startOverlapEdit->setValue(
      map[QString("startOverlapEdit")].toDouble());
  gui->m_ui.endOverlapEdit->setValue(map[QString("endOverlapEdit")].toDouble());
  gui->m_ui.transStitchParamsEdit->setText(
      map[QString("transStitchParamsEdit")].toString());
  gui->m_ui.transScaleRHSCheckBox->setChecked(
      map[QString("transScaleRHSCheckBox")].toBool());
  gui->m_ui.polCorrCheckBox->setChecked(
      map[QString("polCorrCheckBox")].toBool());
  gui->m_ui.floodCorComboBox->setCurrentIndex(
      map[QString("floodCorComboBox")].toInt());
  gui->m_ui.floodWorkspaceWsSelector->setCurrentIndex(
      map[QString("floodWorkspaceWsSelector")].toInt());
  gui->m_stitchEdit->setText(map[QString("stitchEdit")].toString());
}

void Decoder::decodePerAngleDefaults(QTableWidget *tab,
                                     const QMap<QString, QVariant> &map) {
  // Clear the rows
  for (auto rowIndex = 0; rowIndex < tab->rowCount(); ++rowIndex) {
    tab->removeRow(rowIndex);
  }
  const int rowsNum = map[QString("rowsNum")].toInt();
  const int columnsNum = map[QString("columnsNum")].toInt();
  decodePerAngleDefaultsRows(tab, rowsNum, columnsNum,
                             map[QString("rows")].toList());
}

void Decoder::decodePerAngleDefaultsRows(QTableWidget *tab, int rowsNum,
                                         int columnsNum,
                                         const QList<QVariant> &list) {
  for (auto rowIndex = 0; rowIndex < rowsNum; ++rowIndex) {
    tab->insertRow(rowIndex);
    decodePerAngleDefaultsRow(tab, rowIndex, columnsNum,
                              list[rowIndex].toList());
  }
}

void Decoder::decodePerAngleDefaultsRow(QTableWidget *tab, int rowIndex,
                                        int columnsNum,
                                        const QList<QVariant> &list) {
  MantidQt::API::SignalBlocker blocker(tab);
  for (auto columnIndex = 0; columnIndex < columnsNum; ++columnIndex) {
    auto tableWidgetItem = new QTableWidgetItem(list[columnIndex].toString());
    tab->setItem(rowIndex, columnIndex, tableWidgetItem);
  }
}

void Decoder::decodeInstrument(const QtInstrumentView *gui,
                               const QMap<QString, QVariant> &map) {
  gui->m_ui.intMonCheckBox->setChecked(map[QString("intMonCheckBox")].toBool());
  gui->m_ui.monIntMinEdit->setValue(map[QString("monIntMinEdit")].toDouble());
  gui->m_ui.monIntMaxEdit->setValue(map[QString("monIntMaxEdit")].toDouble());
  gui->m_ui.monBgMinEdit->setValue(map[QString("monBgMinEdit")].toDouble());
  gui->m_ui.monBgMaxEdit->setValue(map[QString("monBgMaxEdit")].toDouble());
  gui->m_ui.lamMinEdit->setValue(map[QString("lamMinEdit")].toDouble());
  gui->m_ui.lamMaxEdit->setValue(map[QString("lamMaxEdit")].toDouble());
  gui->m_ui.I0MonitorIndex->setValue(
      static_cast<int>(map[QString("I0MonitorIndex")].toDouble()));
  gui->m_ui.correctDetectorsCheckBox->setChecked(
      map[QString("correctDetectorsCheckBox")].toBool());
  gui->m_ui.detectorCorrectionTypeComboBox->setCurrentIndex(
      map[QString("detectorCorrectionTypeComboBox")].toInt());
}

void Decoder::decodeRuns(QtRunsView *gui, ReductionJobs *redJobs,
                         RunsTablePresenter *presenter,
                         const QMap<QString, QVariant> &map) {
  decodeRunsTable(gui->m_tableView, redJobs, presenter,
                  map[QString("runsTable")].toMap());
  gui->m_ui.comboSearchInstrument->setCurrentIndex(
      map[QString("comboSearchInstrument")].toInt());
  gui->m_ui.textSearch->setText(map[QString("textSearch")].toString());
}

namespace HIDDEN_LOCAL {
std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsFromRow(Row const &row) {
  return std::vector<MantidQt::MantidWidgets::Batch::Cell>(
      {MantidQt::MantidWidgets::Batch::Cell(boost::join(row.runNumbers(), "+")),
       MantidQt::MantidWidgets::Batch::Cell(std::to_string(row.theta())),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().firstRunList()),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().secondRunList()),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::min),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::max),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::step),
       MantidQt::MantidWidgets::Batch::Cell(
           optionalToString(row.scaleFactor())),
       MantidQt::MantidWidgets::Batch::Cell(
           MantidWidgets::optionsToString(row.reductionOptions()))});
}
} // namespace HIDDEN_LOCAL

void Decoder::updateRunsTableViewFromModel(QtRunsTableView *view,
                                           const ReductionJobs *model) {
  auto jobTreeView = view->m_jobs.get();
  auto groups = model->groups();
  for (auto groupIndex = 0u; groupIndex < groups.size(); ++groupIndex) {
    // Update view for groups
    auto group = groups[groupIndex];

    auto modelName = group.name();
    // If name doesn't contain "HiddenGroupName" update groupname as it
    // represents a none user defined name
    if (modelName.find("HiddenGroupName") == std::string::npos) {
      MantidQt::MantidWidgets::Batch::RowLocation location(
          {static_cast<int>(groupIndex)});
      MantidQt::MantidWidgets::Batch::Cell groupCell(modelName);
      jobTreeView->setCellAt({location}, 0, groupCell);
    }

    // Update view for rows
    auto rows = groups[groupIndex].rows();
    for (auto rowIndex = 0u; rowIndex < rows.size(); ++rowIndex) {
      auto row = rows[rowIndex];
      // If row has content in the model.
      if (row) {
        MantidQt::MantidWidgets::Batch::RowLocation location(
            {static_cast<int>(groupIndex), static_cast<int>(rowIndex)});
        jobTreeView->setCellsAt({location},
                                HIDDEN_LOCAL::cellsFromRow(row.get()));
      }
    }
  }
}

void Decoder::decodeRunsTable(QtRunsTableView *gui, ReductionJobs *redJobs,
                              RunsTablePresenter *presenter,
                              const QMap<QString, QVariant> &map) {
  MantidQt::API::SignalBlocker signalBlockerView(gui);

  m_projectSave = map[QString("projectSave")].toBool();
  auto runsTable = map[QString("runsTableModel")].toList();

  // Clear
  presenter->removeAllRowsAndGroupsFromView();
  presenter->removeAllRowsAndGroupsFromModel();

  // Construct the table.
  for (auto groupIndex = 1; groupIndex < runsTable.size() + 1; ++groupIndex) {
    presenter->appendEmptyGroupInModel();
    presenter->appendEmptyGroupInView();
    for (auto rowIndex = 0;
         rowIndex <
         runsTable[groupIndex - 1].toMap()[QString("rows")].toList().size();
         ++rowIndex) {
      presenter->appendRowsToGroupsInView({groupIndex});
      presenter->appendRowsToGroupsInModel({groupIndex});
    }
  }
  // Remove initial group made on construction
  presenter->removeGroupsFromView({0});
  presenter->removeGroupsFromModel({0});

  decodeRunsTableModel(redJobs, runsTable);

  // Still need to do this for groups
  updateRunsTableViewFromModel(gui, redJobs);

  if (m_projectSave) {
    // Apply styling and restore completed state for output range values
    presenter->notifyRowOutputsChanged();
    presenter->notifyRowStateChanged();
  }
  gui->m_ui.filterBox->setText(map[QString("filterBox")].toString());
}

void Decoder::decodeRunsTableModel(ReductionJobs *jobs,
                                   const QList<QVariant> &list) {
  for (auto groupIndex = 0; groupIndex < list.size(); ++groupIndex) {
    auto group = decodeGroup(list[groupIndex].toMap());
    jobs->mutableGroups()[groupIndex] = group;
  }
}

MantidQt::CustomInterfaces::ISISReflectometry::Group
Decoder::decodeGroup(const QMap<QString, QVariant> &map) {
  auto rows = decodeRows(map["rows"].toList());
  MantidQt::CustomInterfaces::ISISReflectometry::Group group(
      map[QString("name")].toString().toStdString(), rows);
  if (m_projectSave) {
    auto itemState = map[QString("itemState")].toInt();
    group.setState(State(itemState));
  }
  group.m_postprocessedWorkspaceName =
      map[QString("postprocessedWorkspaceName")].toString().toStdString();
  return group;
}

std::vector<boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>>
Decoder::decodeRows(const QList<QVariant> &list) {
  std::vector<
      boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>>
      rows;
  for (const auto &rowMap : list) {
    rows.emplace_back(decodeRow(rowMap.toMap()));
  }
  return rows;
}

namespace {
ReductionOptionsMap decodeReductionOptions(const QMap<QString, QVariant> &map) {
  ReductionOptionsMap rom;
  for (const auto &key : map.keys()) {
    rom.insert(std::pair<std::string, std::string>(
        key.toStdString(), map[key].toString().toStdString()));
  }
  return rom;
}
} // namespace

boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>
Decoder::decodeRow(const QMap<QString, QVariant> &map) {
  if (map.size() == 0) {
    return boost::optional<
        MantidQt::CustomInterfaces::ISISReflectometry::Row>();
  }
  std::vector<std::string> number;
  for (const auto &runNumber : map[QString("runNumbers")].toList()) {
    number.emplace_back(runNumber.toString().toStdString());
  }
  boost::optional<double> maybeScaleFactor = boost::none;
  bool scaleFactorPresent = map[QString("scaleFactorPresent")].toBool();
  if (scaleFactorPresent) {
    maybeScaleFactor = map[QString("scaleFactor")].toDouble();
  }
  auto row = MantidQt::CustomInterfaces::ISISReflectometry::Row(
      number, map[QString("theta")].toDouble(),
      decodeTransmissionRunPair(map[QString("transRunNums")].toMap()),
      decodeRangeInQ(map[QString("qRange")].toMap()), maybeScaleFactor,
      decodeReductionOptions(map[QString("reductionOptions")].toMap()),
      decodeReductionWorkspace(map[QString("reductionWorkspaces")].toMap()));
  if (m_projectSave) {
    auto itemState = map[QString("itemState")].toInt();
    row.setState(State(itemState));
    row.setOutputQRange(decodeRangeInQ(map[QString("qRangeOutput")].toMap()));
  }
  return row;
}

RangeInQ Decoder::decodeRangeInQ(const QMap<QString, QVariant> &map) {
  double min = 0;
  double step = 0;
  double max = 0;
  bool minPresent = map[QString("minPresent")].toBool();
  bool maxPresent = map[QString("maxPresent")].toBool();
  bool stepPresent = map[QString("stepPresent")].toBool();
  if (minPresent) {
    min = map[QString("min")].toDouble();
  }
  if (maxPresent) {
    max = map[QString("max")].toDouble();
  }
  if (stepPresent) {
    step = map[QString("step")].toDouble();
  }
  // Preffered implementation is using boost::optional<double> instead of bool
  // and double but older versions of GCC seem to be complaining about
  // -Wmaybe-uninitialized
  if (minPresent && maxPresent && stepPresent) {
    return RangeInQ(min, step, max);
  } else if (minPresent && maxPresent) {
    return RangeInQ(min, boost::none, max);
  } else if (minPresent && stepPresent) {
    return RangeInQ(min, step);
  } else if (minPresent) {
    return RangeInQ(min);
  } else if (stepPresent && maxPresent) {
    return RangeInQ(boost::none, step, max);
  } else if (stepPresent) {
    return RangeInQ(boost::none, step);
  } else if (maxPresent) {
    return RangeInQ(boost::none, boost::none, max);
  } else {
    return RangeInQ();
  }
}

TransmissionRunPair
Decoder::decodeTransmissionRunPair(const QMap<QString, QVariant> &map) {
  auto firstTransRunsQt = map[QString("firstTransRuns")].toList();
  auto secondTransRunsQt = map[QString("secondTransRuns")].toList();
  std::vector<std::string> firstTransRuns;
  std::vector<std::string> secondTransRuns;
  for (const auto &item : firstTransRunsQt) {
    firstTransRuns.emplace_back(item.toString().toStdString());
  }
  for (const auto &item : secondTransRunsQt) {
    secondTransRuns.emplace_back(item.toString().toStdString());
  }
  return TransmissionRunPair(firstTransRuns, secondTransRuns);
}

ReductionWorkspaces
Decoder::decodeReductionWorkspace(const QMap<QString, QVariant> &map) {
  std::vector<std::string> inputRunNumbers;
  for (const auto &elem : map[QString("inputRunNumbers")].toList()) {
    inputRunNumbers.emplace_back(elem.toString().toStdString());
  }
  auto transmissionRunPair =
      decodeTransmissionRunPair(map[QString("transPair")].toMap());
  ReductionWorkspaces redWs(inputRunNumbers, transmissionRunPair);
  redWs.setOutputNames(map[QString("iVsLambda")].toString().toStdString(),
                       map[QString("iVsQ")].toString().toStdString(),
                       map[QString("iVsQBinned")].toString().toStdString());
  return redWs;
}

void Decoder::decodeSave(const QtSaveView *gui,
                         const QMap<QString, QVariant> &map) {
  gui->m_ui.savePathEdit->setText(map[QString("savePathEdit")].toString());
  gui->m_ui.prefixEdit->setText(map[QString("prefixEdit")].toString());
  gui->m_ui.titleCheckBox->setChecked(map[QString("titleCheckBox")].toBool());
  gui->m_ui.qResolutionCheckBox->setChecked(
      map[QString("qResolutionCheckBox")].toBool());
  gui->m_ui.commaRadioButton->setChecked(
      map[QString("commaRadioButton")].toBool());
  gui->m_ui.spaceRadioButton->setChecked(
      map[QString("spaceRadioButton")].toBool());
  gui->m_ui.tabRadioButton->setChecked(map[QString("tabRadioButton")].toBool());
  gui->m_ui.fileFormatComboBox->setCurrentIndex(
      map[QString("fileFormatComboBox")].toInt());
  gui->m_ui.filterEdit->setText(map[QString("filterEdit")].toString());
  gui->m_ui.regexCheckBox->setChecked(map[QString("regexCheckBox")].toBool());
  gui->m_ui.saveReductionResultsCheckBox->setChecked(
      map[QString("saveReductionResultsCheckBox")].toBool());
}

void Decoder::decodeEvent(const QtEventView *gui,
                          const QMap<QString, QVariant> &map) {
  gui->m_ui.disabledSlicingButton->setChecked(
      map[QString("disabledSlicingButton")].toBool());
  gui->m_ui.uniformEvenButton->setChecked(
      map[QString("uniformEvenButton")].toBool());
  gui->m_ui.uniformEvenEdit->setValue(
      static_cast<int>(map[QString("uniformEvenEdit")].toDouble()));
  gui->m_ui.uniformButton->setChecked(map[QString("uniformButton")].toBool());
  gui->m_ui.uniformEdit->setValue(map[QString("uniformEdit")].toDouble());
  gui->m_ui.customButton->setChecked(map[QString("customButton")].toBool());
  gui->m_ui.customEdit->setText(map[QString("customEdit")].toString());
  gui->m_ui.logValueButton->setChecked(map[QString("logValueButton")].toBool());
  gui->m_ui.logValueEdit->setText(map[QString("logValueEdit")].toString());
  gui->m_ui.logValueTypeEdit->setText(
      map[QString("logValueTypeEdit")].toString());
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
