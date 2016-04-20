//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisResultTableTab.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidQtMantidWidgets/MuonSequentialFitDialog.h"
#include "MantidQtAPI/UserSubWindow.h"

#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QMessageBox>
#include <QDesktopServices>

#include <algorithm>

//-----------------------------------------------------------------------------

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

const std::string MuonAnalysisResultTableTab::WORKSPACE_POSTFIX("_Workspace");
const std::string MuonAnalysisResultTableTab::PARAMS_POSTFIX("_Parameters");
const QString MuonAnalysisResultTableTab::RUN_NUMBER_LOG("run_number");
const QStringList MuonAnalysisResultTableTab::NON_TIMESERIES_LOGS =
    QStringList() << MuonAnalysisResultTableTab::RUN_NUMBER_LOG << "sample_temp"
                  << "sample_magn_field";

/**
* Constructor
*/
MuonAnalysisResultTableTab::MuonAnalysisResultTableTab(Ui::MuonAnalysis &uiForm)
    : m_uiForm(uiForm), m_numLogsdisplayed(0), m_savedLogsState(),
      m_unselectedFittings() {
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

  // Enable label combox-box only when sequential fit type selected
  connect(m_uiForm.sequentialFit, SIGNAL(toggled(bool)), m_uiForm.fitLabelCombo,
          SLOT(setEnabled(bool)));

  // Re-populate tables when fit type or seq. fit label is changed
  connect(m_uiForm.fitType, SIGNAL(buttonClicked(QAbstractButton *)), this,
          SLOT(populateTables()));
  connect(m_uiForm.fitLabelCombo, SIGNAL(activated(int)), this,
          SLOT(populateTables()));
}

/**
* Muon Analysis Results Table Help (slot)
*/
void MuonAnalysisResultTableTab::helpResultsClicked() {
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
                                 "MuonAnalysisResultsTable"));
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
      if (temp != NULL) {
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
      if (temp != NULL) {
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

    return getSequentialFitWorkspaces(selectedLabel);
  } else {
    throw std::runtime_error("Uknown fit type option");
  }
}

/**
 * Returns a list of labels user has made sequential fits for.
 * @return List of labels
 */
QStringList MuonAnalysisResultTableTab::getSequentialFitLabels() {
  QStringList labels;

  std::map<std::string, Workspace_sptr> items =
      AnalysisDataService::Instance().topLevelItems();

  for (auto it = items.begin(); it != items.end(); ++it) {
    if (it->second->id() != "WorkspaceGroup")
      continue;

    if (it->first.find(MuonSequentialFitDialog::SEQUENTIAL_PREFIX) != 0)
      continue;

    std::string label =
        it->first.substr(MuonSequentialFitDialog::SEQUENTIAL_PREFIX.size());

    labels << QString::fromStdString(label);
  }

  return labels;
}

/**
 * Returns a list of sequentially fitted workspaces names.
 * @param label :: Label to return sequential fits for
 * @return List of workspace base names
 */
QStringList
MuonAnalysisResultTableTab::getSequentialFitWorkspaces(const QString &label) {
  const AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  std::string groupName =
      MuonSequentialFitDialog::SEQUENTIAL_PREFIX + label.toStdString();

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

  for (auto it = wsNames.begin(); it != wsNames.end(); it++) {
    if (!isFittedWs(*it))
      continue; // Doesn't pass basic checks

    workspaces << QString::fromStdString(wsBaseName(*it));
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

  for (auto it = allWorkspaces.begin(); it != allWorkspaces.end(); it++) {
    if (!isFittedWs(*it))
      continue; // Doesn't pass basic checks

    // Ignore sequential fit results
    if (boost::starts_with(*it, MuonSequentialFitDialog::SEQUENTIAL_PREFIX))
      continue;

    workspaces << QString::fromStdString(wsBaseName(*it));
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
 * Refresh the label list and re-populate the tables.
 */
void MuonAnalysisResultTableTab::refresh() {
  m_uiForm.individualFit->setChecked(true);

  QStringList labels = getSequentialFitLabels();

  m_uiForm.fitLabelCombo->clear();
  m_uiForm.fitLabelCombo->addItems(labels);

  m_uiForm.sequentialFit->setEnabled(m_uiForm.fitLabelCombo->count() != 0);

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

  if (!fittedWsList.isEmpty()) {
    // Populate the individual log values and fittings into their respective
    // tables.
    populateFittings(fittedWsList);
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

    Mantid::Kernel::DateAndTime start = ws->run().startTime();
    Mantid::Kernel::DateAndTime end = ws->run().endTime();

    const std::vector<Property *> &logData = ws->run().getLogData();
    std::vector<Property *>::const_iterator pEnd = logData.end();

    for (std::vector<Property *>::const_iterator pItr = logData.begin();
         pItr != pEnd; ++pItr) {
      // Check if is a timeseries log
      if (TimeSeriesProperty<double> *tspd =
              dynamic_cast<TimeSeriesProperty<double> *>(*pItr)) {
        QString logFile(QFileInfo((**pItr).name().c_str()).fileName());

        double value(0.0);
        int count(0);

        Mantid::Kernel::DateAndTime logTime;

        // iterate through all logs entries of a specific log
        for (int k(0); k < tspd->size(); k++) {
          // Get the log time for the specific entry
          logTime = tspd->nthTime(k);

          // If the entry was made during the run times
          if ((logTime >= start) && (logTime <= end)) {
            // add it to a total and increment the count (will be used to make
            // average entry value during a run)
            value += tspd->nthValue(k);
            count++;
          }
        }

        if (count != 0) {
          // Find average
          wsLogValues[logFile] = value / count;
        }
      } else // Should be a non-timeseries one
      {
        QString logName = QString::fromStdString((**pItr).name());

        // Check if we should display it
        if (NON_TIMESERIES_LOGS.contains(logName)) {
          QVariant value;

          if (logName == RUN_NUMBER_LOG) { // special case
            value = runNumberString(wsName, (*pItr)->value());
          } else if (auto stringProp =
                         dynamic_cast<PropertyWithValue<std::string> *>(
                             *pItr)) {
            value = QString::fromStdString((*stringProp)());
          } else if (auto doubleProp =
                         dynamic_cast<PropertyWithValue<double> *>(*pItr)) {
            value = (*doubleProp)();
          } else {
            throw std::runtime_error("Unsupported non-timeseries log type");
          }

          wsLogValues[logName] = value;
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
      auto wsLogValues = wsIt.value();
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

  // Save the number of logs displayed
  // XXX: this is redundant, as number of logs == number of rows
  m_numLogsdisplayed = m_uiForm.valueTable->rowCount();

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
  int index1 = NON_TIMESERIES_LOGS.indexOf(logName1);
  int index2 = NON_TIMESERIES_LOGS.indexOf(logName2);

  if (index1 == -1 && index2 == -1) {
    // If both are timeseries logs - compare lexicographically ignoring the case
    return logName1.toLower() < logName2.toLower();
  } else if (index1 != -1 && index2 != -1) {
    // If both timeseries - keep the order of non-timeseries logs list
    return index1 < index2;
  } else {
    // If one is timeseries and another is not - the one which is not is always
    // less
    return index1 != -1;
  }
}

/**
* Populates the items (fitted workspaces) into their table.
*
* @param fittedWsList :: a workspace list containing ONLY the workspaces that
* have parameter
*                        tables associated with it.
*/
void MuonAnalysisResultTableTab::populateFittings(
    const QStringList &fittedWsList) {
  // Add number of rows  for the amount of fittings.
  m_uiForm.fittingResultsTable->setRowCount(fittedWsList.size());

  // Add check boxes for the include column on fitting table, and make text
  // uneditable.
  for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++) {
    m_uiForm.fittingResultsTable->setCellWidget(i, 1, new QCheckBox);

    if (auto textItem = m_uiForm.fittingResultsTable->item(i, 0)) {
      textItem->setFlags(textItem->flags() & (~Qt::ItemIsEditable));
    }
  }

  // Get colors, 0=Black, 1=Red, 2=Green, 3=Blue, 4=Orange, 5=Purple. (If there
  // are more than this then use black as default.)
  QMap<int, int> colors = getWorkspaceColors(fittedWsList);
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); row++) {
    // Fill values and delete previous old ones.
    if (row < fittedWsList.size()) {
      QTableWidgetItem *item = new QTableWidgetItem(fittedWsList[row]);
      const int color(colors.find(row).value());
      switch (color) {
      case (1):
        item->setTextColor("red");
        break;
      case (2):
        item->setTextColor("green");
        break;
      case (3):
        item->setTextColor("blue");
        break;
      case (4):
        item->setTextColor("orange");
        break;
      case (5):
        item->setTextColor("purple");
        break;
      default:
        item->setTextColor("black");
      }
      m_uiForm.fittingResultsTable->setItem(row, 0, item);
    } else
      m_uiForm.fittingResultsTable->setItem(row, 0, NULL);
  }
}

/**
* Get the colors corresponding to their position in the workspace list.
*
* @param wsList :: List of all workspaces with fitted parameters.
* @return colors :: List of colors (as numbers) with the key being position in
* wsList.
*/
QMap<int, int>
MuonAnalysisResultTableTab::getWorkspaceColors(const QStringList &wsList) {
  QMap<int, int> colors; // position, color
  int posCount(0);
  int colorCount(0);

  while (wsList.size() != posCount) {
    // If a color has already been chosen for the current workspace then skip
    if (!colors.contains(posCount)) {
      std::vector<std::string> firstParams;
      // Find the first parameter table and use this as a comparison for all the
      // other tables.
      auto paramWs = retrieveWSChecked<ITableWorkspace>(
          wsList[posCount].toStdString() + PARAMS_POSTFIX);

      Mantid::API::TableRow paramRow = paramWs->getFirstRow();
      do {
        std::string key;
        paramRow >> key;
        firstParams.push_back(key);
      } while (paramRow.next());

      colors.insert(posCount, colorCount);

      // Compare to all the other parameters. +1 don't compare with self.
      for (int i = (posCount + 1); i < wsList.size(); ++i) {
        if (!colors.contains(i)) {
          std::vector<std::string> nextParams;
          auto paramWs = retrieveWSChecked<ITableWorkspace>(
              wsList[i].toStdString() + PARAMS_POSTFIX);

          Mantid::API::TableRow paramRow = paramWs->getFirstRow();
          do {
            std::string key;
            paramRow >> key;
            nextParams.push_back(key);
          } while (paramRow.next());

          if (firstParams == nextParams) {
            colors.insert(i, colorCount);
          }
        }
      }
      colorCount++;
    }
    posCount++;
  }
  return colors;
}

void MuonAnalysisResultTableTab::onCreateTableClicked() {
  try {
    createTable();
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
*/
void MuonAnalysisResultTableTab::createTable() {
  if (m_logValues.size() == 0) {
    QMessageBox::information(this, "Mantid - Muon Analysis",
                             "No workspace found with suitable fitting.");
    return;
  }

  // Get the user selection
  QStringList wsSelected = getSelectedWs();
  QStringList logsSelected = getSelectedLogs();

  if ((wsSelected.size() == 0) || logsSelected.size() == 0) {
    QMessageBox::information(this, "Mantid - Muon Analysis",
                             "Please select options from both tables.");
    return;
  }

  // Check workspaces have same parameters
  if (haveSameParameters(wsSelected)) {
    // Create the results table
    Mantid::API::ITableWorkspace_sptr table =
        Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

    // Add columns for log values
    foreach (QString log, logsSelected) {
      std::string columnTypeName;
      int columnPlotType;

      // We use values of the first workspace to determine the type of the
      // column to add. It seems reasonable to assume
      // that log values with the same name will have same types.
      QString typeName = m_logValues[wsSelected[0]][log].typeName();
      if (typeName == "double") {
        columnTypeName = "double";
        columnPlotType = 1;
      } else if (typeName == "QString") {
        columnTypeName = "str";
        columnPlotType = 6;
      } else
        throw std::runtime_error(
            "Couldn't find appropriate column type for value with type " +
            typeName.toStdString());

      Column_sptr newColumn =
          table->addColumn(columnTypeName, log.toStdString());
      newColumn->setPlotType(columnPlotType);
      newColumn->setReadOnly(false);
    }

    // Get param information
    QMap<QString, QMap<QString, double>> wsParamsList;
    QStringList paramsToDisplay;
    for (int i = 0; i < wsSelected.size(); ++i) {
      QMap<QString, double> paramsList;
      auto paramWs = retrieveWSChecked<ITableWorkspace>(
          wsSelected[i].toStdString() + PARAMS_POSTFIX);

      Mantid::API::TableRow paramRow = paramWs->getFirstRow();

      // Loop over all rows and get values and errors.
      do {
        std::string key;
        double value;
        double error;
        paramRow >> key >> value >> error;
        if (i == 0) {
          Column_sptr newValCol = table->addColumn("double", key);
          newValCol->setPlotType(2);
          newValCol->setReadOnly(false);

          Column_sptr newErrorCol = table->addColumn("double", key + "Error");
          newErrorCol->setPlotType(5);
          newErrorCol->setReadOnly(false);

          paramsToDisplay.append(QString::fromStdString(key));
          paramsToDisplay.append(QString::fromStdString(key) + "Error");
        }
        paramsList[QString::fromStdString(key)] = value;
        paramsList[QString::fromStdString(key) + "Error"] = error;
      } while (paramRow.next());

      wsParamsList[wsSelected[i]] = paramsList;
    }

    // Add data to table
    for (auto itr = m_logValues.begin(); itr != m_logValues.end(); itr++) {
      for (int i = 0; i < wsSelected.size(); ++i) {
        if (wsSelected[i] == itr.key()) {
          // Add new row
          Mantid::API::TableRow row = table->appendRow();

          // Add log values to the row
          QMap<QString, QVariant> &logValues = itr.value();

          for (int j = 0; j < logsSelected.size(); j++) {
            Mantid::API::Column_sptr c = table->getColumn(j);
            QVariant &v = logValues[logsSelected[j]];

            if (c->isType<double>())
              row << v.toDouble();
            else if (c->isType<std::string>())
              row << v.toString().toStdString();
            else
              throw std::runtime_error("Log value with name '" +
                                       logsSelected[j].toStdString() +
                                       "' in '" + wsSelected[i].toStdString() +
                                       "' has unexpected type.");
          }

          // Add param values (presume params the same for all workspaces)
          QMap<QString, double> paramsList =
              wsParamsList.find(itr.key()).value();
          for (int j = 0; j < paramsToDisplay.size(); ++j) {
            row << paramsList.find(paramsToDisplay[j]).value();
          }
        }
      }
    }

    std::string tableName = getFileName();

    // Save the table to the ADS
    Mantid::API::AnalysisDataService::Instance().addOrReplace(tableName, table);

    // Python code to show a table on the screen
    std::stringstream code;
    code << "found = False" << std::endl
         << "for w in windows():" << std::endl
         << "  if w.windowLabel() == '" << tableName << "':" << std::endl
         << "    found = True; w.show(); w.setFocus()" << std::endl
         << "if not found:" << std::endl
         << "  importTableWorkspace('" << tableName << "', True)" << std::endl;

    emit runPythonCode(QString::fromStdString(code.str()), false);
  } else {
    QMessageBox::information(
        this, "Mantid - Muon Analysis",
        "Please pick workspaces with the same fitted parameters");
  }
}

/**
* See if the workspaces selected have the same parameters.
*
* @param wsList :: A list of workspaces with fitted parameters.
* @return bool :: Whether or not the wsList given share the same fitting
* parameters.
*/
bool MuonAnalysisResultTableTab::haveSameParameters(const QStringList &wsList) {
  std::vector<std::string> firstParams;

  // Find the first parameter table and use this as a comparison for all the
  // other tables.
  auto paramWs = retrieveWSChecked<ITableWorkspace>(wsList[0].toStdString() +
                                                    PARAMS_POSTFIX);

  Mantid::API::TableRow paramRow = paramWs->getFirstRow();
  do {
    std::string key;
    paramRow >> key;
    firstParams.push_back(key);
  } while (paramRow.next());

  // Compare to all the other parameters.
  for (int i = 1; i < wsList.size(); ++i) {
    std::vector<std::string> nextParams;
    auto paramWs = retrieveWSChecked<ITableWorkspace>(wsList[i].toStdString() +
                                                      PARAMS_POSTFIX);

    Mantid::API::TableRow paramRow = paramWs->getFirstRow();
    do {
      std::string key;
      paramRow >> key;
      nextParams.push_back(key);
    } while (paramRow.next());

    if (!(firstParams == nextParams))
      return false;
  }
  return true;
}

/**
* Get the user selected workspaces with _parameters files associated with it
*
* @return wsSelected :: A vector of QString's containing the workspace that are
* selected.
*/
QStringList MuonAnalysisResultTableTab::getSelectedWs() {
  QStringList wsSelected;
  for (int i = 0; i < m_logValues.size(); i++) {
    QCheckBox *includeCell = static_cast<QCheckBox *>(
        m_uiForm.fittingResultsTable->cellWidget(i, 1));
    if (includeCell->isChecked()) {
      QTableWidgetItem *wsName = static_cast<QTableWidgetItem *>(
          m_uiForm.fittingResultsTable->item(i, 0));
      wsSelected.push_back(wsName->text());
    }
  }
  return wsSelected;
}

/**
* Get the user selected log file
*
* @return logsSelected :: A vector of QString's containing the logs that are
* selected.
*/
QStringList MuonAnalysisResultTableTab::getSelectedLogs() {
  QStringList logsSelected;
  for (int i = 0; i < m_numLogsdisplayed; i++) {
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

/**
 * Uses the format of the workspace name
 * (INST00012345-8; Pair; long; Asym; 1+2-3+4; #2)
 * to get a string in the format "run number: period"
 * @param workspaceName :: [input] Name of the workspace
 * @param firstRun :: [input] First run number - use this if tokenizing fails
 * @returns Run number/period string
 */
QString
MuonAnalysisResultTableTab::runNumberString(const std::string &workspaceName,
                                            const std::string &firstRun) {
  std::string periods = "";        // default
  std::string instRuns = firstRun; // default

  Mantid::Kernel::StringTokenizer tokenizer(
      workspaceName, ";", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  if (tokenizer.count() > 4) {
    instRuns = tokenizer[0];
    periods = tokenizer[4];
    // Remove "INST000" off the start
    // No muon instruments have numbers in their names
    size_t numPos = instRuns.find_first_of("123456789");
    instRuns = instRuns.substr(numPos, instRuns.size());
  }

  QString ret(instRuns.c_str());
  if (!periods.empty()) {
    ret.append(": ").append(periods.c_str());
  }
  return ret;
}
}
}
}
