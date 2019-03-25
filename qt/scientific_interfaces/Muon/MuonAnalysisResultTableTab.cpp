// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MuonAnalysisResultTableTab.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MuonAnalysisHelper.h"
#include "MuonAnalysisResultTableCreator.h"
#include "MuonSequentialFitDialog.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/shared_ptr.hpp>

#include <QFileInfo>
#include <QHash>
#include <QLineEdit>
#include <QMessageBox>

#include <algorithm>

namespace {
const std::string RUNNING_LOG_NAME = "running";
}

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using Mantid::Types::Core::DateAndTime;

const std::string MuonAnalysisResultTableTab::WORKSPACE_POSTFIX("_Workspace");
const std::string MuonAnalysisResultTableTab::PARAMS_POSTFIX("_Parameters");
const QString MuonAnalysisResultTableTab::RUN_NUMBER_LOG("run_number");
const QString MuonAnalysisResultTableTab::RUN_START_LOG("run_start");
const QString MuonAnalysisResultTableTab::RUN_END_LOG("run_end");
const QStringList MuonAnalysisResultTableTab::NON_TIMESERIES_LOGS{
    MuonAnalysisResultTableTab::RUN_NUMBER_LOG,
    "group",
    "period",
    RUN_START_LOG,
    RUN_END_LOG,
    "sample_temp",
    "sample_magn_field"};

/**
 * Constructor
 */
MuonAnalysisResultTableTab::MuonAnalysisResultTableTab(Ui::MuonAnalysis &uiForm)
    : m_uiForm(uiForm), m_savedLogsState(), m_unselectedFittings() {
  // Connect the help button to the wiki page.
  connect(m_uiForm.muonAnalysisHelpResults, SIGNAL(clicked()), this,
          SLOT(helpResultsClicked()));

  // Set the default name
  m_uiForm.tableName->setText("ResultsTable");

  // Connect the select/deselect all buttons.
  connect(m_uiForm.selectAllLogValues, SIGNAL(toggled(bool)), this,
          SLOT(selectAllLogs(bool)));
  connect(m_uiForm.selectAllFittingResults, SIGNAL(toggled(bool)), this,
          SLOT(selectAllFittings(bool)));

  // Connect the create table button
  connect(m_uiForm.createTableBtn, SIGNAL(clicked()), this,
          SLOT(onCreateTableClicked()));

  // Enable relevant label combo-box only when matching fit type selected
  connect(m_uiForm.sequentialFit, SIGNAL(toggled(bool)), m_uiForm.fitLabelCombo,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.simultaneousFit, SIGNAL(toggled(bool)),
          m_uiForm.cmbFitLabelSimultaneous, SLOT(setEnabled(bool)));

  // Re-populate tables when fit type or seq./sim. fit label is changed
  connect(m_uiForm.fitType, SIGNAL(buttonClicked(QAbstractButton *)), this,
          SLOT(populateTables()));
  connect(m_uiForm.fitLabelCombo, SIGNAL(activated(int)), this,
          SLOT(populateTables()));
  connect(m_uiForm.cmbFitLabelSimultaneous, SIGNAL(activated(int)), this,
          SLOT(populateTables()));
}

/**
 * Muon Analysis Results Table Help (slot)
 */
void MuonAnalysisResultTableTab::helpResultsClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Muon Analysis"), QString("results-table"));
}

/**
 * Select/Deselect all log values to be included in the table
 */
void MuonAnalysisResultTableTab::selectAllLogs(bool state) {
  if (state) {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
      QTableWidgetItem *temp =
          static_cast<QTableWidgetItem *>(m_uiForm.valueTable->item(i, 0));
      // If there is an item there then check the box
      if (temp != nullptr) {
        QCheckBox *includeCell =
            static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(i, 1));
        includeCell->setChecked(true);
      }
    }
  } else {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
      QCheckBox *includeCell =
          static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(i, 1));
      includeCell->setChecked(false);
    }
  }
}

/**
 * Select/Deselect all fitting results to be included in the table
 */
void MuonAnalysisResultTableTab::selectAllFittings(bool state) {
  if (state) {
    for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++) {
      QTableWidgetItem *temp = static_cast<QTableWidgetItem *>(
          m_uiForm.fittingResultsTable->item(i, 0));
      // If there is an item there then check the box
      if (temp != nullptr) {
        QCheckBox *includeCell = static_cast<QCheckBox *>(
            m_uiForm.fittingResultsTable->cellWidget(i, 1));
        includeCell->setChecked(true);
      }
    }
  } else {
    for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++) {
      QCheckBox *includeCell = static_cast<QCheckBox *>(
          m_uiForm.fittingResultsTable->cellWidget(i, 1));
      includeCell->setChecked(false);
    }
  }
}

/**
 * Remembers which fittings and logs have been selected/deselected by the user.
 * Used in combination with
 * applyUserSettings() so that we dont lose what the user has chosen when
 * switching tabs.
 */
void MuonAnalysisResultTableTab::storeUserSettings() {
  m_savedLogsState.clear();

  // Find which logs have been selected by the user.
  for (int row = 0; row < m_uiForm.valueTable->rowCount(); ++row) {
    if (QTableWidgetItem *log = m_uiForm.valueTable->item(row, 0)) {
      QCheckBox *logCheckBox =
          static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(row, 1));
      m_savedLogsState[log->text()] = logCheckBox->checkState();
    }
  }

  m_unselectedFittings.clear();

  // Find which fittings have been deselected by the user.
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); ++row) {
    QTableWidgetItem *temp = m_uiForm.fittingResultsTable->item(row, 0);
    if (temp) {
      QCheckBox *fittingChoice = static_cast<QCheckBox *>(
          m_uiForm.fittingResultsTable->cellWidget(row, 1));
      if (!fittingChoice->isChecked())
        m_unselectedFittings += temp->text();
    }
  }
}

/**
 * Applies the stored lists of which fittings and logs have been
 * selected/deselected by the user.
 */
void MuonAnalysisResultTableTab::applyUserSettings() {
  // If we're just starting the tab for the first time (and there are no user
  // choices),
  // then don't bother.
  if (m_savedLogsState.isEmpty() && m_unselectedFittings.isEmpty())
    return;

  // If any of the logs have previously been selected by the user, select them
  // again.
  for (int row = 0; row < m_uiForm.valueTable->rowCount(); ++row) {
    if (QTableWidgetItem *log = m_uiForm.valueTable->item(row, 0)) {
      if (m_savedLogsState.contains(log->text())) {
        QCheckBox *logCheckBox =
            static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(row, 1));
        logCheckBox->setCheckState(m_savedLogsState[log->text()]);
      }
    }
  }

  // If any of the fittings have previously been deselected by the user,
  // deselect them again.
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); ++row) {
    QTableWidgetItem *temp = m_uiForm.fittingResultsTable->item(row, 0);
    if (temp) {
      if (m_unselectedFittings.contains(temp->text())) {
        QCheckBox *fittingChoice = static_cast<QCheckBox *>(
            m_uiForm.fittingResultsTable->cellWidget(row, 1));
        fittingChoice->setChecked(false);
      }
    }
  }
}

/**
 * Returns a list of workspaces which should be displayed in the table,
 * depending on what user has
 * chosen to view.
 * @return List of workspace base names
 */
QStringList MuonAnalysisResultTableTab::getFittedWorkspaces() {
  if (m_uiForm.fitType->checkedButton() == m_uiForm.individualFit) {
    return getIndividualFitWorkspaces();
  } else if (m_uiForm.fitType->checkedButton() == m_uiForm.sequentialFit) {
    QString selectedLabel = m_uiForm.fitLabelCombo->currentText();
    return getMultipleFitWorkspaces(selectedLabel, true);
  } else if (m_uiForm.fitType->checkedButton() == m_uiForm.simultaneousFit) {
    return getMultipleFitWorkspaces(
        m_uiForm.cmbFitLabelSimultaneous->currentText(), false);
  } else if (m_uiForm.fitType->checkedButton() == m_uiForm.multipleSimFits) {
    // all simultaneously fitted workspaces
    QStringList wsList;
    for (int i = 0; i < m_uiForm.cmbFitLabelSimultaneous->count(); ++i) {
      const auto names = getMultipleFitWorkspaces(
          m_uiForm.cmbFitLabelSimultaneous->itemText(i), false);
      wsList.append(names);
    }
    return wsList;
  } else {
    throw std::runtime_error("Unknown fit type option");
  }
}

/**
 * Returns a list of labels user has made sequential and simultaneous fits for.
 * @return Pair of lists of labels: <sequential, simultaneous>
 */
std::pair<QStringList, QStringList> MuonAnalysisResultTableTab::getFitLabels() {
  QStringList seqLabels, simLabels;

  std::map<std::string, Workspace_sptr> items =
      AnalysisDataService::Instance().topLevelItems();

  for (auto &item : items) {
    if (item.second->id() != "WorkspaceGroup")
      continue;

    if (item.first.find(MuonSequentialFitDialog::SEQUENTIAL_PREFIX) == 0) {
      std::string label =
          item.first.substr(MuonSequentialFitDialog::SEQUENTIAL_PREFIX.size());
      seqLabels << QString::fromStdString(label);
    } else if (item.first.find(MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX) ==
               0) {
      std::string label =
          item.first.substr(MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX.size());
      simLabels << QString::fromStdString(label);
    } else {
      continue;
    }
  }

  return std::make_pair(seqLabels, simLabels);
}

/**
 * Returns a list of sequentially/simultaneously fitted workspaces names.
 * @param label :: Label to return sequential/simultaneous fits for
 * @param sequential :: true for sequential, false for simultaneous
 * @return List of workspace base names
 */
QStringList
MuonAnalysisResultTableTab::getMultipleFitWorkspaces(const QString &label,
                                                     bool sequential) {
  const AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  const std::string groupName = [&label, &sequential]() {
    if (sequential) {
      return MuonSequentialFitDialog::SEQUENTIAL_PREFIX + label.toStdString();
    } else {
      return MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX + label.toStdString();
    }
  }();

  WorkspaceGroup_sptr group;

  // Might have been accidentally deleted by user
  if (!ads.doesExist(groupName) ||
      !(group = ads.retrieveWS<WorkspaceGroup>(groupName))) {
    QMessageBox::critical(
        this, "Group not found",
        "Group with fitting results of the specified label was not found.");
    return QStringList();
  }

  std::vector<std::string> wsNames = group->getNames();

  QStringList workspaces;

  for (auto &wsName : wsNames) {
    auto wsGroup = ads.retrieveWS<WorkspaceGroup>(wsName);
    if (sequential) {
      std::vector<std::string> tmpNames = wsGroup->getNames();
      for (auto &tmpName : tmpNames) {
        if (!isFittedWs(tmpName))
          continue; // Doesn't pass basic checks

        workspaces << QString::fromStdString(wsBaseName(tmpName));
      }
    } else {
      if (!isFittedWs(wsName))
        continue; // Doesn't pass basic checks

      workspaces << QString::fromStdString(wsBaseName(wsName));
    }
  }

  return workspaces;
}

/**
 * Returns a list individually fitted workspaces names.
 * @return List of workspace base names
 */
QStringList MuonAnalysisResultTableTab::getIndividualFitWorkspaces() {
  QStringList workspaces;

  auto allWorkspaces = AnalysisDataService::Instance().getObjectNames();

  for (auto &allWorkspace : allWorkspaces) {
    if (!isFittedWs(allWorkspace))
      continue; // Doesn't pass basic checks

    // Ignore sequential fit results
    if (boost::starts_with(allWorkspace,
                           MuonSequentialFitDialog::SEQUENTIAL_PREFIX))
      continue;

    // Ignore simultaneous fit results
    if (boost::starts_with(allWorkspace,
                           MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX)) {
      continue;
    }

    workspaces << QString::fromStdString(wsBaseName(allWorkspace));
  }

  return workspaces;
}

/**
 * Returns name of the fitted workspace with WORKSPACE_POSTFIX removed.
 * @param wsName :: Name of the fitted workspace. Shoud end with
 * WORKSPACE_POSTFIX.
 * @return wsName without WORKSPACE_POSTFIX
 */
std::string MuonAnalysisResultTableTab::wsBaseName(const std::string &wsName) {
  return wsName.substr(0, wsName.size() - WORKSPACE_POSTFIX.size());
}

/**
 * Does a few basic checks for whether the workspace is a fitted workspace.
 * @param wsName :: Name of the workspace to check for
 * @return True if seems to be fitted ws, false if doesn't
 */
bool MuonAnalysisResultTableTab::isFittedWs(const std::string &wsName) {
  if (!boost::ends_with(wsName, WORKSPACE_POSTFIX)) {
    return false; // Doesn't end with WORKSPACE_POSTFIX
  }

  try {
    auto ws = retrieveWSChecked<MatrixWorkspace>(wsName);

    ws->run().startTime();
    ws->run().endTime();
  } catch (...) {
    return false; // Not found / incorrect type / doesn't have start/end time
  }

  std::string baseName = wsBaseName(wsName);

  try {
    retrieveWSChecked<ITableWorkspace>(baseName + PARAMS_POSTFIX);
  } catch (...) {
    return false; // _Parameters workspace not found / has incorrect type
  }

  return true; // All OK
}

/**
 * Refresh the label lists and re-populate the tables.
 */
void MuonAnalysisResultTableTab::refresh() {
  m_uiForm.individualFit->setChecked(true);

  const auto &labels = getFitLabels();

  m_uiForm.fitLabelCombo->clear();
  m_uiForm.fitLabelCombo->addItems(labels.first);
  m_uiForm.cmbFitLabelSimultaneous->clear();
  m_uiForm.cmbFitLabelSimultaneous->addItems(labels.second);

  // Find width of widest string in a list
  const auto &font = this->fontMetrics();
  const auto maxWidth = [&font](const QStringList &strings) {
    const QString extraSpace = "   "; // to make sure string will fit
    int maximum = 0;
    for (const auto &string : strings) {
      const auto &width = font.boundingRect(string + extraSpace).width();
      if (width > maximum) {
        maximum = width;
      }
    }
    return maximum;
  };

  // Expand the width of the drop-down (not the combobox itself) to fit the
  // longest string
  const auto &seqSize = maxWidth(labels.first);
  m_uiForm.fitLabelCombo->view()->setMinimumWidth(seqSize);
  m_uiForm.fitLabelCombo->view()->setTextElideMode(Qt::ElideNone);
  const auto &simSize = maxWidth(labels.second);
  m_uiForm.cmbFitLabelSimultaneous->view()->setMinimumWidth(simSize);
  m_uiForm.cmbFitLabelSimultaneous->view()->setTextElideMode(Qt::ElideNone);

  m_uiForm.sequentialFit->setEnabled(m_uiForm.fitLabelCombo->count() != 0);
  m_uiForm.simultaneousFit->setEnabled(
      m_uiForm.cmbFitLabelSimultaneous->count() > 0);
  m_uiForm.multipleSimFits->setEnabled(
      m_uiForm.cmbFitLabelSimultaneous->count() > 0);

  populateTables();
}

/**
 * Clear and populate both tables.
 */
void MuonAnalysisResultTableTab::populateTables() {
  storeUserSettings();

  // Clear the previous table values
  m_logValues.clear();
  m_uiForm.fittingResultsTable->setRowCount(0);
  m_uiForm.valueTable->setRowCount(0);

  QStringList fittedWsList = getFittedWorkspaces();
  fittedWsList.sort(); // sort by instrument then run (i.e. alphanumerically)

  if (!fittedWsList.isEmpty()) {
    // Populate the individual log values and fittings into their respective
    // tables.
    if (m_uiForm.fitType->checkedButton() == m_uiForm.multipleSimFits) {
      // Simultaneous fits: use labels
      auto wsFromName = [](const QString &qs) {
        const auto &wsGroup = retrieveWSChecked<WorkspaceGroup>(
            MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX + qs.toStdString());
        return boost::dynamic_pointer_cast<Workspace>(wsGroup);
      };
      populateFittings(getFitLabels().second, wsFromName);
    } else {
      // Use fitted workspace names
      auto wsFromName = [](const QString &qs) {
        const auto &tab = retrieveWSChecked<ITableWorkspace>(qs.toStdString() +
                                                             PARAMS_POSTFIX);
        return boost::dynamic_pointer_cast<Workspace>(tab);
      };
      populateFittings(fittedWsList, wsFromName);
    }

    populateLogsAndValues(fittedWsList);

    // Make sure all fittings are selected by default.
    selectAllFittings(true);

    // If we have Run Number log value, we want to select it by default.
    auto found =
        m_uiForm.valueTable->findItems("run_number", Qt::MatchFixedString);
    if (!found.empty()) {
      int r = found[0]->row();

      if (auto cb = dynamic_cast<QCheckBox *>(
              m_uiForm.valueTable->cellWidget(r, 1))) {
        cb->setCheckState(Qt::Checked);
      }
    }

    applyUserSettings();
  }
}

/**
 * Populates the items (log values) into their table.
 *
 * @param fittedWsList :: a workspace list containing ONLY the workspaces that
 * have parameter
 *                   tables associated with it.
 */
void MuonAnalysisResultTableTab::populateLogsAndValues(
    const QStringList &fittedWsList) {
  // A set of all the logs we've met in the workspaces
  QSet<QString> allLogs;

  for (int i = 0; i < fittedWsList.size(); i++) {
    QMap<QString, QVariant> wsLogValues;

    // Get log information
    std::string wsName = fittedWsList[i].toStdString();
    auto ws = retrieveWSChecked<ExperimentInfo>(wsName + WORKSPACE_POSTFIX);
    const std::vector<Property *> &logData = ws->run().getLogData();
    const TimeSeriesProperty<bool> *runningLog = nullptr;
    Property *runLog = nullptr;
    const bool foundRunning = ws->run().hasProperty(RUNNING_LOG_NAME);
    if (foundRunning) {
      runLog = ws->run().getLogData(RUNNING_LOG_NAME);
      runningLog = dynamic_cast<TimeSeriesProperty<bool> *>(runLog);

    } else {
      Mantid::Kernel::Logger g_log("MuonAnalysisResultTableTab");
      g_log.warning(
          "No running log found. Filtering will not be applied to the data.\n");
    }
    for (const auto &prop : logData) {
      // Check if is a timeseries log
      if (TimeSeriesProperty<double> *log =
              dynamic_cast<TimeSeriesProperty<double> *>(prop)) {
        QString logName = QString::fromStdString(prop->name());
        auto mylog = log->clone();
        if (foundRunning) {
          mylog->filterWith(runningLog);
        }
        QString logFile(QFileInfo(prop->name().c_str()).fileName());
        auto time_ave = mylog->timeAverageValue(); // get the time average
        // Return average
        wsLogValues[logFile] = time_ave;
      } else // Should be a non-timeseries one
      {
        QString logName = QString::fromStdString(prop->name());
        // Check if we should display it
        if (NON_TIMESERIES_LOGS.contains(logName)) {

          if (logName == RUN_NUMBER_LOG) { // special case
            wsLogValues[RUN_NUMBER_LOG] =
                MuonAnalysisHelper::runNumberString(wsName, prop->value());
          } else if (logName == RUN_START_LOG || logName == RUN_END_LOG) {
            wsLogValues[logName + " (text)"] =
                QString::fromStdString(prop->value());
            const auto seconds =
                static_cast<double>(
                    DateAndTime(prop->value()).totalNanoseconds()) *
                1.e-9;
            wsLogValues[logName + " (s)"] = seconds;
          } else if (auto stringProp =
                         dynamic_cast<PropertyWithValue<std::string> *>(prop)) {
            wsLogValues[logName] = QString::fromStdString((*stringProp)());
          } else if (auto doubleProp =
                         dynamic_cast<PropertyWithValue<double> *>(prop)) {
            wsLogValues[logName] = (*doubleProp)();
          } else {
            throw std::runtime_error("Unsupported non-timeseries log type");
          }
        }
      }
    }

    // Append log names found in the workspace to the list of all known log
    // names
    allLogs += wsLogValues.keys().toSet();

    // Add all data collected from one workspace to another map. Will be used
    // when creating table.
    m_logValues[fittedWsList[i]] = wsLogValues;

  } // End loop over all workspace's log information and param information

  // Remove the logs that don't appear in all workspaces
  QSet<QString> toRemove;
  for (auto logIt = allLogs.constBegin(); logIt != allLogs.constEnd();
       ++logIt) {
    for (auto wsIt = m_logValues.constBegin(); wsIt != m_logValues.constEnd();
         ++wsIt) {
      const auto &wsLogValues = wsIt.value();
      if (!wsLogValues.contains(*logIt)) {
        toRemove.insert(*logIt);
        break;
      }
    }
  }

  allLogs = allLogs.subtract(toRemove);

  // Sort logs
  QList<QString> allLogsSorted(allLogs.toList());
  qSort(allLogsSorted.begin(), allLogsSorted.end(),
        MuonAnalysisResultTableTab::logNameLessThan);

  // Add number of rows to the table based on number of logs to display.
  m_uiForm.valueTable->setRowCount(allLogsSorted.size());

  // Populate table with all log values available without repeating any.
  for (auto it = allLogsSorted.constBegin(); it != allLogsSorted.constEnd();
       ++it) {
    int row = static_cast<int>(std::distance(allLogsSorted.constBegin(), it));
    m_uiForm.valueTable->setItem(row, 0, new QTableWidgetItem(*it));
  }

  // Add check boxes for the include column on log table, and make text
  // uneditable.
  for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
    m_uiForm.valueTable->setCellWidget(i, 1, new QCheckBox);

    if (auto textItem = m_uiForm.valueTable->item(i, 0)) {
      textItem->setFlags(textItem->flags() & (~Qt::ItemIsEditable));
    }
  }
}

/**
 * LessThan function used to sort log names. Puts non-timeseries logs first and
 * the timeseries ones
 * sorted by named ignoring the case.
 * @param logName1
 * @param logName2
 * @return True if logName1 is less than logName2, false otherwise
 */
bool MuonAnalysisResultTableTab::logNameLessThan(const QString &logName1,
                                                 const QString &logName2) {
  int index1 = NON_TIMESERIES_LOGS.indexOf(logName1.split(' ').first());
  int index2 = NON_TIMESERIES_LOGS.indexOf(logName2.split(' ').first());

  if (index1 == -1 && index2 == -1) {
    // If both are timeseries logs - compare lexicographically ignoring the case
    return logName1.toLower() < logName2.toLower();
  } else if (index1 != -1 && index2 != -1) {
    // If both non-timeseries - keep the order of non-timeseries logs list
    if (index1 == index2) {
      // Correspond to same log, compare lexicographically
      return logName1.toLower() < logName2.toLower();
    } else {
      return index1 < index2;
    }
  } else {
    // If one is timeseries and another is not - the one which is not is always
    // less
    return index1 != -1;
  }
}

/**
 * Populates fittings table with given workspace/label names.
 * Can be used for workspace names that have associated param tables, or for
 * simultaneous fit labels, just by passing in a different function.
 *
 * @param names :: [input] list of workspace names OR label names
 * @param wsFromName :: [input] Function for getting workspaces from the given
 * names
 */
void MuonAnalysisResultTableTab::populateFittings(
    const QStringList &names,
    std::function<Workspace_sptr(const QString &)> wsFromName) {
  // Add number of rows for the amount of fittings.
  m_uiForm.fittingResultsTable->setRowCount(names.size());

  // Add check boxes for the include column on fitting table, and make text
  // uneditable.
  for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++) {
    m_uiForm.fittingResultsTable->setCellWidget(i, 1, new QCheckBox);

    if (auto textItem = m_uiForm.fittingResultsTable->item(i, 0)) {
      textItem->setFlags(textItem->flags() & (~Qt::ItemIsEditable));
    }
  }

  // Get workspace names using the provided function
  std::vector<Workspace_sptr> workspaces;
  for (const auto &name : names) {
    workspaces.push_back(wsFromName(name));
  }

  // Get colors for names in table
  const auto colors = MuonAnalysisHelper::getWorkspaceColors(workspaces);
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); row++) {
    // Fill values and delete previous old ones.
    if (row < names.size()) {
      QTableWidgetItem *item = new QTableWidgetItem(names[row]);
      item->setTextColor(colors.value(row));
      m_uiForm.fittingResultsTable->setItem(row, 0, item);
    } else
      m_uiForm.fittingResultsTable->setItem(row, 0, nullptr);
  }
}

void MuonAnalysisResultTableTab::onCreateTableClicked() {
  try {
    const bool multipleFits =
        m_uiForm.fitType->checkedButton() == m_uiForm.multipleSimFits;
    createTable(multipleFits);
  } catch (Exception::NotFoundError &e) {
    std::ostringstream errorMsg;
    errorMsg << "Workspace required to create a table was not found:\n\n"
             << e.what();
    QMessageBox::critical(this, "Workspace not found",
                          QString::fromStdString(errorMsg.str()));
    refresh(); // As something was probably deleted, refresh the tables
    return;
  } catch (std::exception &e) {
    std::ostringstream errorMsg;
    errorMsg << "Error occured when trying to create the table:\n\n"
             << e.what();
    QMessageBox::critical(this, "Error",
                          QString::fromStdString(errorMsg.str()));
    return;
  }
}

/**
 * Creates the table using the information selected by the user in the tables
 * @param multipleFits :: [input] Whether table is for multiple fits or one
 * single fit
 */
void MuonAnalysisResultTableTab::createTable(bool multipleFits) {
  if (m_logValues.size() == 0) {
    QMessageBox::information(this, "Mantid - Muon Analysis",
                             "No workspace found with suitable fitting.");
    return;
  }

  // Get the user selection
  QStringList wsSelected = getSelectedItemsToFit();
  QStringList logsSelected = getSelectedLogs();

  MuonAnalysisResultTableCreator creator(wsSelected, logsSelected, &m_logValues,
                                         multipleFits);
  ITableWorkspace_sptr table;
  try {
    table = creator.createTable();
  } catch (const std::runtime_error &err) {
    QMessageBox::information(this, "Mantid - Muon Analysis", err.what());
    return;
  }
  const std::string &tableName = getFileName();

  // Save the table to the ADS
  Mantid::API::AnalysisDataService::Instance().addOrReplace(tableName, table);

  // Python code to show a table on the screen
  std::stringstream code;
  code << "found = False\n"
       << "for w in windows():\n"
       << "  if w.windowLabel() == '" << tableName << "':\n"
       << "    found = True; w.show(); w.setFocus()\n"
       << "if not found:\n"
       << "  importTableWorkspace('" << tableName << "', True)\n";

  emit runPythonCode(QString::fromStdString(code.str()), false);
}

/**
 * Get the user selected workspaces OR labels from the table
 * @return :: list of selected workspaces/labels
 */
QStringList MuonAnalysisResultTableTab::getSelectedItemsToFit() {
  QStringList items;
  for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); ++i) {
    const auto includeCell = static_cast<QCheckBox *>(
        m_uiForm.fittingResultsTable->cellWidget(i, 1));
    if (includeCell->isChecked()) {
      items.push_back(m_uiForm.fittingResultsTable->item(i, 0)->text());
    }
  }
  return items;
}

/**
 * Get the user selected log file
 *
 * @return logsSelected :: A vector of QString's containing the logs that are
 * selected.
 */
QStringList MuonAnalysisResultTableTab::getSelectedLogs() {
  QStringList logsSelected;
  for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
    QCheckBox *includeCell =
        static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(i, 1));
    if (includeCell->isChecked()) {
      QTableWidgetItem *logParam =
          static_cast<QTableWidgetItem *>(m_uiForm.valueTable->item(i, 0));
      logsSelected.push_back(logParam->text());
    }
  }
  return logsSelected;
}

/**
 * Checks that the file name isn't been used, displays the appropriate message
 * and
 * then returns the name in which to save.
 *
 * @return name :: The name the results table should be created with.
 */
std::string MuonAnalysisResultTableTab::getFileName() {
  std::string fileName(m_uiForm.tableName->text().toStdString());

  if (Mantid::API::AnalysisDataService::Instance().doesExist(fileName)) {
    int choice = QMessageBox::question(
        this, tr("MantidPlot - Overwrite Warning"),
        QString::fromStdString(fileName) +
            tr(" already exists. Do you want to replace it?"),
        QMessageBox::Yes | QMessageBox::Default,
        QMessageBox::No | QMessageBox::Escape);
    if (choice == QMessageBox::No) {
      int versionNum(2);
      fileName += " #";
      while (Mantid::API::AnalysisDataService::Instance().doesExist(
          fileName + boost::lexical_cast<std::string>(versionNum))) {
        versionNum += 1;
      }
      return (fileName + boost::lexical_cast<std::string>(versionNum));
    }
  }
  return fileName;
}
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt
