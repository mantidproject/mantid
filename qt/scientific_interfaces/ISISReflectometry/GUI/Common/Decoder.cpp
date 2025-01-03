// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "../Runs/QtCatalogSearcher.h"
#include "../Runs/QtRunsView.h"
#include "../Runs/RunsPresenter.h"
#include "../RunsTable/QtRunsTableView.h"
#include "../RunsTable/RunsTablePresenter.h"
#include "../Save/QtSaveView.h"
#include "Encoder.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

#include <QApplication>
#include <QSignalBlocker>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

BatchPresenter *Decoder::findBatchPresenter(const QtBatchView *gui, const IMainWindowView *view) {
  auto mwv = dynamic_cast<const QtMainWindowView *>(view);
  for (auto &ipresenter : mwv->m_presenter->m_batchPresenters) {
    auto presenter = dynamic_cast<BatchPresenter *>(ipresenter.get());
    if (presenter->m_view == gui) {
      return presenter;
    }
  }
  return nullptr;
}

QWidget *Decoder::decode(const QMap<QString, QVariant> &map, const std::string &directory) {
  UNUSED_ARG(directory)
  auto userSubWindow = MantidQt::API::InterfaceManager().createSubWindow("ISIS Reflectometry");
  auto mwv = dynamic_cast<QtMainWindowView *>(userSubWindow);
  auto batches = map["batches"].toList();
  // Create the number of batches required.
  for (auto batchIndex = mwv->batches().size(); batchIndex < static_cast<long unsigned int>(batches.size());
       ++batchIndex) {
    mwv->newBatch();
  }
  for (auto batchIndex = 0; batchIndex < batches.size(); ++batchIndex) {
    decodeBatch(mwv, batchIndex, batches[batchIndex].toMap());
  }
  return mwv;
}

QList<QString> Decoder::tags() { return QList<QString>({QString("ISIS Reflectometry")}); }

void Decoder::decodeBatch(const IMainWindowView *mwv, int batchIndex, const QMap<QString, QVariant> &batchMap) {
  m_currentBatchVersion = decodeVersion(batchMap);
  auto gui = dynamic_cast<const QtBatchView *>(mwv->batches()[batchIndex]);
  auto batchPresenter = findBatchPresenter(gui, mwv);
  if (!batchPresenter) {
    throw std::runtime_error("BatchPresenter could not be found during decode.");
  }
  auto runsPresenter = dynamic_cast<RunsPresenter *>(batchPresenter->m_runsPresenter.get());
  auto runsTablePresenter = dynamic_cast<RunsTablePresenter *>(runsPresenter->m_tablePresenter.get());
  auto reductionJobs = &runsTablePresenter->m_model.m_reductionJobs;
  auto destinationPrecision = batchPresenter->m_mainPresenter->roundPrecision();
  auto searcher = dynamic_cast<QtCatalogSearcher *>(runsPresenter->m_searcher.get());
  // We must do the Runs tab first because this sets the instrument, which
  // other settings may need to be correct. There is also a notification to set
  // defaults for this instrument so we need to do that before other settings
  // or it will override them.
  decodeRuns(gui->m_runs.get(), reductionJobs, runsTablePresenter, batchMap[QString("runsView")].toMap(),
             destinationPrecision, searcher);
  decodeEvent(gui->m_eventHandling.get(), batchMap[QString("eventView")].toMap());
  decodeExperiment(gui->m_experiment.get(), batchMap[QString("experimentView")].toMap());
  decodeInstrument(gui->m_instrument.get(), batchMap[QString("instrumentView")].toMap());
  decodeSave(gui->m_save.get(), batchMap[QString("saveView")].toMap());
}

size_t Decoder::decodeVersion(const QMap<QString, QVariant> &batchMap) const {
  return batchMap[QString("version")].toUInt();
}

void Decoder::decodeExperiment(QtExperimentView *gui, const QMap<QString, QVariant> &map) {
  gui->m_ui.analysisModeComboBox->setCurrentIndex(map[QString("analysisModeComboBox")].toInt());
  gui->m_ui.debugCheckBox->setChecked(map[QString("debugCheckbox")].toBool());
  gui->m_ui.summationTypeComboBox->setCurrentIndex(map[QString("summationTypeComboBox")].toInt());
  gui->m_ui.reductionTypeComboBox->setCurrentIndex(map[QString("reductionTypeComboBox")].toInt());
  gui->m_ui.includePartialBinsCheckBox->setChecked(map[QString("includePartialBinsCheckBox")].toBool());
  decodePerAngleDefaults(gui->m_ui.optionsTable, map[QString("perAngleDefaults")].toMap());
  gui->m_ui.startOverlapEdit->setValue(map[QString("startOverlapEdit")].toDouble());
  gui->m_ui.endOverlapEdit->setValue(map[QString("endOverlapEdit")].toDouble());
  gui->m_ui.transStitchParamsEdit->setText(map[QString("transStitchParamsEdit")].toString());
  gui->m_ui.transScaleRHSCheckBox->setChecked(map[QString("transScaleRHSCheckBox")].toBool());
  gui->m_ui.subtractBackgroundCheckBox->setChecked(map[QString("subtractBackgroundCheckBox")].toBool());
  gui->m_ui.backgroundMethodComboBox->setCurrentIndex(map[QString("backgroundMethodComboBox")].toInt());
  gui->m_ui.polynomialDegreeSpinBox->setValue(map[QString("polynomialDegreeSpinBox")].toInt());
  gui->m_ui.costFunctionComboBox->setCurrentIndex(map[QString("costFunctionComboBox")].toInt());
  decodePolarizationCorrectionsComboBox(gui->m_ui.polCorrComboBox, map);
  gui->m_polCorrEfficienciesWsSelector->setCurrentText(map[QString("polCorrEfficienciesWsSelector")].toString());
  gui->m_polCorrEfficienciesLineEdit->setText(map[QString("polCorrEfficienciesLineEdit")].toString());
  gui->m_ui.floodCorComboBox->setCurrentIndex(map[QString("floodCorComboBox")].toInt());
  gui->m_floodCorrWsSelector->setCurrentText(map[QString("floodWorkspaceWsSelector")].toString());
  gui->m_floodCorrLineEdit->setText(map[QString("floodWorkspaceLineEdit")].toString());
  gui->m_stitchEdit->setText(map[QString("stitchEdit")].toString());
  gui->onSettingsChanged();
}

void Decoder::decodePolarizationCorrectionsComboBox(QComboBox *polCorrComboBox,
                                                    const QMap<QString, QVariant> &map) const {
  if (m_currentBatchVersion >= 2) {
    polCorrComboBox->setCurrentText(map[QString("polCorrComboBox")].toString());
    return;
  }
  if (map[QString("polCorrCheckBox")].toBool()) {
    polCorrComboBox->setCurrentText("ParameterFile");
  } else {
    polCorrComboBox->setCurrentText("None");
  }
}

void Decoder::decodePerAngleDefaults(QTableWidget *tab, const QMap<QString, QVariant> &map) {
  // Clear the rows
  tab->setRowCount(0);
  const int rowsNum = map[QString("rowsNum")].toInt();
  const int columnsNum = map[QString("columnsNum")].toInt();

  if (m_currentBatchVersion >= 1) {
    decodePerAngleDefaultsRows(tab, rowsNum, columnsNum, map[QString("rows")].toList());
  } else {
    decodeLegacyPerAngleDefaultsRows(tab, rowsNum, columnsNum, map[QString("rows")].toList());
  }
}

void Decoder::decodeLegacyPerAngleDefaultsRows(QTableWidget *tab, int rowsNum, int columnsNum,
                                               const QList<QVariant> &list) {
  for (auto rowIndex = 0; rowIndex < rowsNum; ++rowIndex) {
    tab->insertRow(rowIndex);
    decodeLegacyPerAngleDefaultsRow(tab, rowIndex, columnsNum, list[rowIndex].toList());
  }
}

void Decoder::decodePerAngleDefaultsRows(QTableWidget *tab, int rowsNum, int columnsNum, const QList<QVariant> &list) {
  for (auto rowIndex = 0; rowIndex < rowsNum; ++rowIndex) {
    tab->insertRow(rowIndex);
    decodePerAngleDefaultsRow(tab, rowIndex, columnsNum, list[rowIndex].toList());
  }
}

void Decoder::decodeLegacyPerAngleDefaultsRow(QTableWidget *tab, int rowIndex, int columnsNum, QList<QVariant> list) {
  // WORKAROUND: This method can only handle 9/10 column legacy files. All future files (e.g. 11+ cols) should
  // be versioned and should never hit the below path:
  if (!(columnsNum == 9 || columnsNum == 10)) {
    throw std::out_of_range(
        "Cannot decode malformed row. Unexpected number of columns for legacy row. Should have 9 or 10.");
  }
  // Column 2 was created to hold a title matcher
  list.insert(1, QString(""));

  if (columnsNum == 9) {
    // Column 11 was created to hold the background ROI
    list.append(QString(""));
  }

  // We've now fixed this up to an 11 column file, so hardcode this:
  decodePerAngleDefaultsRow(tab, rowIndex, 11, list);
}

void Decoder::decodePerAngleDefaultsRow(QTableWidget *tab, int rowIndex, int columnsNum, const QList<QVariant> &list) {
  QSignalBlocker blocker(tab);
  for (auto columnIndex = 0; columnIndex < tab->columnCount(); ++columnIndex) {
    auto const columnValue = columnIndex < columnsNum ? list[columnIndex].toString() : QString();
    auto tableWidgetItem = new QTableWidgetItem(columnValue);
    tab->setItem(rowIndex, columnIndex, tableWidgetItem);
  }
}

void Decoder::decodeInstrument(const QtInstrumentView *gui, const QMap<QString, QVariant> &map) {
  gui->m_ui.intMonCheckBox->setChecked(map[QString("intMonCheckBox")].toBool());
  gui->m_ui.monIntMinEdit->setValue(map[QString("monIntMinEdit")].toDouble());
  gui->m_ui.monIntMaxEdit->setValue(map[QString("monIntMaxEdit")].toDouble());
  gui->m_ui.monBgMinEdit->setValue(map[QString("monBgMinEdit")].toDouble());
  gui->m_ui.monBgMaxEdit->setValue(map[QString("monBgMaxEdit")].toDouble());
  gui->m_ui.lamMinEdit->setValue(map[QString("lamMinEdit")].toDouble());
  gui->m_ui.lamMaxEdit->setValue(map[QString("lamMaxEdit")].toDouble());
  gui->m_ui.I0MonitorIndex->setValue(static_cast<int>(map[QString("I0MonitorIndex")].toDouble()));
  gui->m_ui.correctDetectorsCheckBox->setChecked(map[QString("correctDetectorsCheckBox")].toBool());
  gui->m_ui.detectorCorrectionTypeComboBox->setCurrentIndex(map[QString("detectorCorrectionTypeComboBox")].toInt());
  gui->m_ui.calibrationPathEdit->setText(map[QString("calibrationPathEdit")].toString());
}

void Decoder::decodeRuns(QtRunsView *gui, ReductionJobs *redJobs, RunsTablePresenter *presenter,
                         const QMap<QString, QVariant> &map, std::optional<int> precision,
                         QtCatalogSearcher *searcher) {
  decodeRunsTable(gui->m_tableView, redJobs, presenter, map[QString("runsTable")].toMap(), std::move(precision));
  gui->m_ui.comboSearchInstrument->setCurrentIndex(map[QString("comboSearchInstrument")].toInt());
  gui->m_ui.textSearch->setText(map[QString("textSearch")].toString());
  gui->m_ui.textCycle->setText(map[QString("textCycle")].toString());
  gui->mutableSearchResults().replaceResults(decodeSearchResults(map[QString("searchResults")].toList()));
  // To avoid thinking we are doing a "new search" we need to set the cached
  // search criteria to be the same as the displayed criteria.
  searcher->m_searchCriteria.investigation = map[QString("textSearch")].toString().toStdString();
  searcher->m_searchCriteria.cycle = map[QString("textCycle")].toString().toStdString();
  searcher->m_searchCriteria.instrument = map[QString("textInstrument")].toString().toStdString();
}

namespace {
using ValueFunction = std::optional<double> (RangeInQ::*)() const;

MantidWidgets::Batch::Cell qRangeCellOrDefault(RangeInQ const &qRangeInput, RangeInQ const &qRangeOutput,
                                               ValueFunction valueFunction, std::optional<int> precision) {
  auto maybeValue = (qRangeInput.*valueFunction)();
  auto useOutputValue = false;
  if (!maybeValue.has_value()) {
    maybeValue = (qRangeOutput.*valueFunction)();
    useOutputValue = true;
  }
  auto result = MantidWidgets::Batch::Cell(optionalToString(maybeValue, precision));
  if (useOutputValue)
    result.setOutput();
  else
    result.setInput();
  return result;
}

std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsFromRow(Row const &row, const std::optional<int> &precision) {
  return std::vector<MantidQt::MantidWidgets::Batch::Cell>(
      {MantidQt::MantidWidgets::Batch::Cell(boost::join(row.runNumbers(), "+")),
       MantidQt::MantidWidgets::Batch::Cell(valueToString(row.theta(), precision)),
       MantidQt::MantidWidgets::Batch::Cell(row.transmissionWorkspaceNames().firstRunList()),
       MantidQt::MantidWidgets::Batch::Cell(row.transmissionWorkspaceNames().secondRunList()),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::min, precision),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::max, precision),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::step, precision),
       MantidQt::MantidWidgets::Batch::Cell(optionalToString(row.scaleFactor(), precision)),
       MantidQt::MantidWidgets::Batch::Cell(MantidWidgets::optionsToString(row.reductionOptions())),
       MantidQt::MantidWidgets::Batch::Cell(optionalToString(row.lookupIndex(), precision))});
}
} // namespace

void Decoder::updateRunsTableViewFromModel(QtRunsTableView *view, const ReductionJobs *model,
                                           const std::optional<int> &precision) {
  auto jobTreeView = view->m_jobs.get();
  auto const &groups = model->groups();
  for (auto groupIndex = 0u; groupIndex < groups.size(); ++groupIndex) {
    // Update view for groups

    auto const &modelName = groups[groupIndex].name();
    // If name doesn't contain "HiddenGroupName" update groupname as it
    // represents a none user defined name
    if (modelName.find("HiddenGroupName") == std::string::npos) {
      MantidQt::MantidWidgets::Batch::RowLocation location({static_cast<int>(groupIndex)});
      MantidQt::MantidWidgets::Batch::Cell groupCell(modelName);
      jobTreeView->setCellAt({location}, 0, groupCell);
    }

    // Update view for rows
    auto const &rows = groups[groupIndex].rows();
    for (auto rowIndex = 0u; rowIndex < rows.size(); ++rowIndex) {
      auto const &row = rows[rowIndex];
      // If row has content in the model.
      if (row) {
        MantidQt::MantidWidgets::Batch::RowLocation location(
            {static_cast<int>(groupIndex), static_cast<int>(rowIndex)});
        jobTreeView->setCellsAt({location}, cellsFromRow(row.get(), precision));
      }
    }
  }
}

void Decoder::decodeRunsTable(QtRunsTableView *gui, ReductionJobs *redJobs, RunsTablePresenter *presenter,
                              const QMap<QString, QVariant> &map, std::optional<int> precision) {
  QSignalBlocker signalBlockerView(gui);

  m_projectSave = map[QString("projectSave")].toBool();
  auto runsTable = map[QString("runsTableModel")].toList();

  // Clear
  presenter->removeAllRowsAndGroupsFromView();
  presenter->removeAllRowsAndGroupsFromModel();

  // Construct the table.
  for (auto groupIndex = 1; groupIndex < runsTable.size() + 1; ++groupIndex) {
    presenter->appendEmptyGroupInModel();
    presenter->appendEmptyGroupInView();
    for (auto rowIndex = 0; rowIndex < runsTable[groupIndex - 1].toMap()[QString("rows")].toList().size(); ++rowIndex) {
      presenter->appendRowsToGroupsInView({groupIndex});
      presenter->appendRowsToGroupsInModel({groupIndex});
    }
  }
  // Remove initial group made on construction
  presenter->removeGroupsFromView({0});
  presenter->removeGroupsFromModel({0});

  decodeRunsTableModel(redJobs, runsTable);

  // Still need to do this for groups
  updateRunsTableViewFromModel(gui, redJobs, precision);

  if (m_projectSave) {
    // Apply styling and restore completed state for output range values
    presenter->notifyRowModelChanged();
    presenter->notifyRowStateChanged();
  }
  gui->m_ui.filterBox->setText(map[QString("filterBox")].toString());
}

void Decoder::decodeRunsTableModel(ReductionJobs *jobs, const QList<QVariant> &list) {
  for (auto groupIndex = 0; groupIndex < list.size(); ++groupIndex) {
    jobs->mutableGroups()[groupIndex] = decodeGroup(list[groupIndex].toMap());
  }
}

MantidQt::CustomInterfaces::ISISReflectometry::Group Decoder::decodeGroup(const QMap<QString, QVariant> &map) {
  auto rows = decodeRows(map["rows"].toList());
  MantidQt::CustomInterfaces::ISISReflectometry::Group group(map[QString("name")].toString().toStdString(), rows);
  if (m_projectSave) {
    auto itemState = map[QString("itemState")].toInt();
    group.setState(State(itemState));
  }
  group.m_postprocessedWorkspaceName = map[QString("postprocessedWorkspaceName")].toString().toStdString();
  return group;
}

std::vector<boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>>
Decoder::decodeRows(const QList<QVariant> &list) {
  std::vector<boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>> rows;
  std::transform(list.cbegin(), list.cend(), std::back_inserter(rows),
                 [this](const auto &rowMap) { return decodeRow(rowMap.toMap()); });
  return rows;
}

namespace {
ReductionOptionsMap decodeReductionOptions(const QMap<QString, QVariant> &map) {
  ReductionOptionsMap rom;
  for (const auto &key : map.keys()) {
    rom.insert(std::pair<std::string, std::string>(key.toStdString(), map[key].toString().toStdString()));
  }
  return rom;
}
} // namespace

boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>
Decoder::decodeRow(const QMap<QString, QVariant> &map) {
  if (map.size() == 0) {
    return boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>();
  }
  std::vector<std::string> number;
  const auto runNoList = map[QString("runNumbers")].toList();
  std::transform(runNoList.cbegin(), runNoList.cend(), std::back_inserter(number),
                 [](const auto &runNumber) { return runNumber.toString().toStdString(); });
  boost::optional<double> maybeScaleFactor = boost::make_optional<double>(false, 0.0);
  bool scaleFactorPresent = map[QString("scaleFactorPresent")].toBool();
  if (scaleFactorPresent) {
    maybeScaleFactor = map[QString("scaleFactor")].toDouble();
  }
  auto row = MantidQt::CustomInterfaces::ISISReflectometry::Row(
      number, map[QString("theta")].toDouble(), decodeTransmissionRunPair(map[QString("transRunNums")].toMap()),
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
    return RangeInQ(min, std::nullopt, max);
  } else if (minPresent && stepPresent) {
    return RangeInQ(min, step);
  } else if (minPresent) {
    return RangeInQ(min);
  } else if (stepPresent && maxPresent) {
    return RangeInQ(std::nullopt, step, max);
  } else if (stepPresent) {
    return RangeInQ(std::nullopt, step);
  } else if (maxPresent) {
    return RangeInQ(std::nullopt, std::nullopt, max);
  } else {
    return RangeInQ();
  }
}

TransmissionRunPair Decoder::decodeTransmissionRunPair(const QMap<QString, QVariant> &map) {
  auto firstTransRunsQt = map[QString("firstTransRuns")].toList();
  auto secondTransRunsQt = map[QString("secondTransRuns")].toList();
  std::vector<std::string> firstTransRuns;
  std::vector<std::string> secondTransRuns;
  std::transform(firstTransRunsQt.cbegin(), firstTransRunsQt.cend(), std::back_inserter(firstTransRuns),
                 [](const auto &item) { return item.toString().toStdString(); });
  std::transform(secondTransRunsQt.cbegin(), secondTransRunsQt.cend(), std::back_inserter(secondTransRuns),
                 [](const auto &item) { return item.toString().toStdString(); });
  return TransmissionRunPair(firstTransRuns, secondTransRuns);
}

MantidQt::CustomInterfaces::ISISReflectometry::SearchResults Decoder::decodeSearchResults(const QList<QVariant> &list) {
  SearchResults rows;
  std::transform(list.cbegin(), list.cend(), std::back_inserter(rows),
                 [this](const auto &rowMap) { return decodeSearchResult(rowMap.toMap()); });
  return rows;
}

MantidQt::CustomInterfaces::ISISReflectometry::SearchResult
Decoder::decodeSearchResult(const QMap<QString, QVariant> &map) {
  SearchResult searchResult(
      map[QString("runNumber")].toString().toStdString(), map[QString("title")].toString().toStdString(),
      map[QString("groupName")].toString().toStdString(), map[QString("theta")].toString().toStdString(),
      map[QString("error")].toString().toStdString(), map[QString("excludeReason")].toString().toStdString(),
      map[QString("comment")].toString().toStdString());
  return searchResult;
}

ReductionWorkspaces Decoder::decodeReductionWorkspace(const QMap<QString, QVariant> &map) {
  std::vector<std::string> inputRunNumbers;
  const auto inputRunList = map[QString("inputRunNumbers")].toList();
  std::transform(inputRunList.cbegin(), inputRunList.cend(), std::back_inserter(inputRunNumbers),
                 [](const auto &elem) { return elem.toString().toStdString(); });
  auto transmissionRunPair = decodeTransmissionRunPair(map[QString("transPair")].toMap());
  ReductionWorkspaces redWs(inputRunNumbers, transmissionRunPair);
  redWs.setOutputNames(map[QString("iVsLambda")].toString().toStdString(),
                       map[QString("iVsQ")].toString().toStdString(),
                       map[QString("iVsQBinned")].toString().toStdString());
  return redWs;
}

void Decoder::decodeSave(const QtSaveView *gui, const QMap<QString, QVariant> &map) {
  gui->m_ui.savePathEdit->setText(map[QString("savePathEdit")].toString());
  gui->m_ui.prefixEdit->setText(map[QString("prefixEdit")].toString());
  gui->m_ui.headerCheckBox->setChecked(map[QString("headerCheckBox")].toBool());
  gui->m_ui.qResolutionCheckBox->setChecked(map[QString("qResolutionCheckBox")].toBool());
  gui->m_ui.extraColumnsCheckBox->setChecked(map[QString("extraColumnsCheckBox")].toBool());
  gui->m_ui.multipleDatasetsCheckBox->setChecked(map[QString("multipleDatasetsCheckBox")].toBool());
  gui->m_ui.commaRadioButton->setChecked(map[QString("commaRadioButton")].toBool());
  gui->m_ui.spaceRadioButton->setChecked(map[QString("spaceRadioButton")].toBool());
  gui->m_ui.tabRadioButton->setChecked(map[QString("tabRadioButton")].toBool());
  gui->m_ui.fileFormatComboBox->setCurrentIndex(map[QString("fileFormatComboBox")].toInt());
  gui->m_ui.filterEdit->setText(map[QString("filterEdit")].toString());
  gui->m_ui.regexCheckBox->setChecked(map[QString("regexCheckBox")].toBool());
  gui->m_ui.saveReductionResultsCheckBox->setChecked(map[QString("saveReductionResultsCheckBox")].toBool());
  gui->m_ui.saveIndividualRowsCheckBox->setChecked(map[QString("saveIndividualRowsCheckBox")].toBool());
}

void Decoder::decodeEvent(const QtEventView *gui, const QMap<QString, QVariant> &map) {
  gui->m_ui.disabledSlicingButton->setChecked(map[QString("disabledSlicingButton")].toBool());
  gui->m_ui.uniformEvenButton->setChecked(map[QString("uniformEvenButton")].toBool());
  gui->m_ui.uniformButton->setChecked(map[QString("uniformButton")].toBool());
  gui->m_ui.customButton->setChecked(map[QString("customButton")].toBool());
  gui->m_ui.logValueButton->setChecked(map[QString("logValueButton")].toBool());
  gui->m_ui.uniformEvenEdit->setValue(static_cast<int>(map[QString("uniformEvenEdit")].toDouble()));
  gui->m_ui.uniformEdit->setValue(map[QString("uniformEdit")].toDouble());
  gui->m_ui.customEdit->setText(map[QString("customEdit")].toString());
  gui->m_ui.logValueEdit->setText(map[QString("logValueEdit")].toString());
  gui->m_ui.logValueTypeEdit->setText(map[QString("logValueTypeEdit")].toString());
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
