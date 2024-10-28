// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/CatalogSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"

#include <Poco/ActiveResult.h>
#include <Poco/Path.h>

#include <QDesktopWidget>
#include <QFileDialog>
#include <QSettings>
#include <QStyle>
#include <QUrl>

#include <fstream>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 */
CatalogSearch::CatalogSearch(QWidget *parent)
    : QWidget(parent), m_icatHelper(new CatalogHelper()), m_catalogSelector(new CatalogSelector()),
      m_currentPageNumber(1) {
  initLayout();
  // Load saved settings from store.
  loadSettings();
}

/**
 * Destructor
 */

/**
 * Initialise the  default layout.
 */
void CatalogSearch::initLayout() {
  // Draw the GUI from .ui header generated file.
  m_icatUiForm.setupUi(this);

  // What facilities is the user logged in to?
  m_icatUiForm.facilityName->setText(QString::fromStdString(
      "Currently logged into " + Mantid::Kernel::ConfigService::Instance().getFacility().name()));

  // Only want to show labels when an error occurs.
  hideErrorLabels();

  // Hide advanced input fields until "Advanced search" is checked.
  advancedSearchChecked();

  // Show the search frame by default.
  m_icatUiForm.searchCbox->setChecked(true);
  showCatalogSearch();

  // Prevents a user seeing empty tables.
  m_icatUiForm.searchResultsCbox->setEnabled(false);
  m_icatUiForm.dataFileCbox->setEnabled(false);
  m_icatUiForm.resFrame->hide();
  m_icatUiForm.dataFileFrame->hide();

  // Disable download and load buttons until a user has selected a datafile.
  m_icatUiForm.dataFileDownloadBtn->setEnabled(false);
  m_icatUiForm.dataFileLoadBtn->setEnabled(false);

  // We create the calendar here to allow only one instance of it to occur.
  m_calendar = new QCalendarWidget(qobject_cast<QWidget *>(this->parent()));

  // When the user has selected a date from the calendar we want to set the
  // related date input field.
  connect(m_calendar, SIGNAL(clicked(QDate)), this, SLOT(dateSelected(QDate)));
  // Show related help page when a user clicks on the "Help" button.
  connect(m_icatUiForm.helpBtn, SIGNAL(clicked()), this, SLOT(helpClicked()));
  // Show "Search" frame when user clicks "Catalog search" check box.
  connect(m_icatUiForm.searchCbox, SIGNAL(clicked()), this, SLOT(showCatalogSearch()));
  // Show advanced search options if "Advanced search" is checked.
  connect(m_icatUiForm.advSearchCbox, SIGNAL(clicked()), this, SLOT(advancedSearchChecked()));
  // Open calendar when start or end date is selected
  connect(m_icatUiForm.startDatePicker, SIGNAL(clicked()), this, SLOT(openCalendar()));
  connect(m_icatUiForm.endDatePicker, SIGNAL(clicked()), this, SLOT(openCalendar()));
  // Clear all fields when reset button is pressed.
  connect(m_icatUiForm.resetBtn, SIGNAL(clicked()), this, SLOT(onReset()));
  // Show "Search results" frame when user tries to "Search".
  connect(m_icatUiForm.searchBtn, SIGNAL(clicked()), this, SLOT(searchClicked()));
  // Show "Search results" frame when user clicks related check box.
  connect(m_icatUiForm.searchResultsCbox, SIGNAL(clicked()), this, SLOT(showSearchResults()));
  // When the user has double clicked on an investigation they wish to view
  // datafiles for then load the relevant datafiles.
  connect(m_icatUiForm.searchResultsTbl, SIGNAL(itemDoubleClicked(QTableWidgetItem *)), this,
          SLOT(investigationSelected(QTableWidgetItem *)));
  // Show "DataFile frame" when the user selects an investigation.
  connect(m_icatUiForm.dataFileCbox, SIGNAL(clicked()), this, SLOT(showDataFileInfo()));
  // When the user has selected a filter type then perform the filter for the
  // specified type.
  connect(m_icatUiForm.dataFileFilterCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(doFilter(int)));
  // When the user clicks "download to..." then open a dialog and download the
  // file(s) to that location.
  connect(m_icatUiForm.dataFileDownloadBtn, SIGNAL(clicked()), this, SLOT(downloadDataFiles()));
  // When the user clicks the "load" button then load their selected datafiles
  // into a workspace.
  connect(m_icatUiForm.dataFileLoadBtn, SIGNAL(clicked()), this, SLOT(loadDataFiles()));
  // When a checkbox is selected in a row we want to select (highlight) the
  // entire row.
  connect(m_icatUiForm.dataFileResultsTbl, SIGNAL(itemClicked(QTableWidgetItem *)), this,
          SLOT(dataFileCheckboxSelected(QTableWidgetItem *)));
  // When several rows are selected we want to check the related checkboxes.
  connect(m_icatUiForm.dataFileResultsTbl, SIGNAL(itemSelectionChanged()), this, SLOT(dataFileRowSelected()));
  // When the user clicks "< Prev" populate the results table with the previous
  // 100 results.
  connect(m_icatUiForm.resPrevious, SIGNAL(clicked()), this, SLOT(prevPageClicked()));
  // When the user clicks "Next >" populate the results table with the next 100
  // results.
  connect(m_icatUiForm.resNext, SIGNAL(clicked()), this, SLOT(nextPageClicked()));
  // When the user is done editing & presses enter we retrieve the results for
  // that specific page in paging.
  connect(m_icatUiForm.pageStartNum, SIGNAL(editingFinished()), SLOT(goToInputPage()));
  // Open the catalog/facility selection widget when 'Select a catalog' is
  // clicked.
  connect(m_icatUiForm.catalogSelection, SIGNAL(clicked()), SLOT(openFacilitySelection()));

  // No need for error handling as that's dealt with in the algorithm being
  // used.
  populateInstrumentBox();
  // Although this is an advanced option performing it here allows it to be
  // performed once only.
  populateInvestigationTypeBox();

  // As the methods have been created, and elements are in GUI I have opted to
  // hide
  // these elements for testing purposes as multiple facilities or paging has
  // not yet been implemented.
  // They will be implemented in separate tickets in the next release.
  m_icatUiForm.facilityLogin->hide();

  // Limit input to: A number, 1 hyphen or colon followed by another number.
  // E.g. 444-444, -444, 444-
  QRegExp re("[0-9]*(-|:){1}[0-9]*");
  m_icatUiForm.RunRange->setValidator(new QRegExpValidator(re, this));
  // Limit the page number input field to only digits.
  m_icatUiForm.pageStartNum->setValidator(new QIntValidator(0, 999, this));

  // Resize to minimum width/height to improve UX.
  this->resize(minimumSizeHint());
  // Centre the GUI on screen.
  this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, this->window()->size(),
                                        QDesktopWidget().availableGeometry()));
}

/**
 * Opens the login dialog to allow the user to log into another facility.
 */
void CatalogSearch::onFacilityLogin() {}

/**
 * Sends the user to relevant search page on the Mantid project site.
 */
void CatalogSearch::helpClicked() {
  using MantidQt::API::MantidDesktopServices;
  MantidDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Catalog_Search"));
}

/**
 * Shows/hides the "Catalog search" frame when search combo box is checked.
 */
void CatalogSearch::showCatalogSearch() {
  if (m_icatUiForm.searchCbox->isChecked()) {
    m_icatUiForm.searchFrame->show();
  } else {
    m_icatUiForm.searchFrame->hide();
  }
}

/**
 * Shows/Hides the "Search results" frame when search results combo box is
 * checked.
 */
void CatalogSearch::showSearchResults() {
  if (m_icatUiForm.searchResultsCbox->isChecked()) {
    m_icatUiForm.resFrame->show();
  } else {
    m_icatUiForm.resFrame->hide();
  }
}

/**
 * Hides "Search results" frame when a result is double clicked.
 */
void CatalogSearch::showDataFileInfo() {
  if (m_icatUiForm.dataFileCbox->isChecked()) {
    m_icatUiForm.dataFileFrame->show();
  } else {
    m_icatUiForm.dataFileFrame->hide();
  }
}

/**
 * Embolden the headers in the provided table.
 */
void CatalogSearch::emboldenTableHeaders(QTableWidget *table) {
  QFont font;
  font.setBold(true);
  for (int i = 0; i < table->columnCount(); ++i) {
    table->horizontalHeaderItem(i)->setFont(font);
  }
}

/**
 * Set the table properties prior to adding data to it.
 * @param table        :: The table we want to setup.
 * @param numOfRows    :: The number of rows in the workspace.
 * @param numOfColumns :: The number of columns in the workspace.
 */
void CatalogSearch::setupTable(QTableWidget *table, const size_t &numOfRows, const size_t &numOfColumns) {
  table->setRowCount(static_cast<int>(numOfRows));
  table->setColumnCount(static_cast<int>(numOfColumns));

  // Improve the appearance of table to make it easier to read.
  table->setAlternatingRowColors(true);
  table->setStyleSheet("alternate-background-color: rgb(216, 225, 255)");
  table->setSortingEnabled(false);
  table->verticalHeader()->setVisible(false);

  // Set the height on each row to 20 for UX improvement.
  for (size_t i = 0; i < numOfRows; ++i) {
    table->setRowHeight(static_cast<int>(i), 20);
  }
}

/**
 * Populate the provided table with data from the provided workspace.
 * @param table :: The table we want to setup.
 * @param workspace :: The workspace to obtain data information from.
 */
void CatalogSearch::populateTable(QTableWidget *table, const Mantid::API::ITableWorkspace_sptr &workspace) {
  // NOTE: This method freezes up the ICAT search GUI. We will need to do this
  // adding in another thread.

  // This will contain the list of column names.
  QStringList columnHeaders;
  columnHeaders.reserve(static_cast<int>(workspace->columnCount()));

  // Add the data from the workspace to the search results table.
  for (size_t col = 0; col < workspace->columnCount(); col++) {
    // Get the column name to display as the header of table widget
    Mantid::API::Column_sptr column = workspace->getColumn(col);
    columnHeaders.push_back(QString::fromStdString(column->name()));

    for (size_t row = 0; row < workspace->rowCount(); ++row) {
      // Prints the value from the row to the ostringstream to use later.
      std::ostringstream ostr;
      column->print(row, ostr);

      // Add a result to the table.
      QTableWidgetItem *newItem = new QTableWidgetItem(QString::fromStdString(ostr.str()));
      table->setItem(static_cast<int>(row), static_cast<int>(col), newItem);

      // Allow the row to be selected, and enabled.
      newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      newItem->setToolTip(QString::fromStdString(ostr.str()));
    }
  }
  // Set the table widgets header labels from the table workspace.
  table->setHorizontalHeaderLabels(columnHeaders);
  // Make the headers of the table bold.
  emboldenTableHeaders(table);
}

/**
 * Clears data associated with previous search.
 * @param table     :: The table to modify and remove previous results from.
 * @param workspace :: The workspace to remove.
 */
void CatalogSearch::clearSearch(QTableWidget *table, const std::string &workspace) {
  // Remove workspace if it exists.
  if (Mantid::API::AnalysisDataService::Instance().doesExist(workspace)) {
    Mantid::API::AnalysisDataService::Instance().remove(workspace);
  }

  // In order to reset fields for the table
  setupTable(table, 0, 0);
}

/**
 * Clear the "search" frame when an investigation has been selected.
 */
void CatalogSearch::clearSearchFrame() {
  m_icatUiForm.searchCbox->setChecked(false);
  m_icatUiForm.searchFrame->hide();
}

/**
 * Clear the "search results" frame if no results are returned from search.
 */
void CatalogSearch::clearSearchResultFrame() {
  m_icatUiForm.searchResultsLbl->setText("0 investigations found.");
  m_icatUiForm.searchResultsCbox->setEnabled(false);
  m_icatUiForm.searchResultsCbox->setChecked(false);
  m_icatUiForm.searchResultsTbl->clear();
  m_icatUiForm.resFrame->hide();
}

/**
 * Clear "dataFileFrame" when the user tries to search again.
 */
void CatalogSearch::clearDataFileFrame() {
  m_icatUiForm.dataFileCbox->setEnabled(false);
  m_icatUiForm.dataFileCbox->setChecked(false);
  m_icatUiForm.dataFileLbl->clear();
  m_icatUiForm.dataFileFrame->hide();
}

/**
 * Obtain the index of the column in a table that contains a specified name.
 * @param table     :: The table to search the headers on.
 * @param searchFor :: The header name to search against.
 * @return The index of the column with the specified name.
 */
int CatalogSearch::headerIndexByName(QTableWidget *table, const std::string &searchFor) {
  QAbstractItemModel *model = table->model();

  // For every column in the table
  for (int col = 0; col < table->columnCount(); col++) {
    // Is the column name the same as the searchFor string?
    if (searchFor.compare(model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString().toStdString()) == 0) {
      // Yes? Return the index of the column.
      return (col);
    }
  }
  // This indicates that the column was not found.
  return (-1);
}

/**
 * Save the current state of ICAT for next time
 */
void CatalogSearch::saveSettings() {
  QSettings settings;
  settings.beginGroup("/ICatSettings");
  settings.setValue("lastDownloadPath", m_downloadSaveDir);
  settings.endGroup();
}

/**
 * Read the saved settings from the store.
 */
void CatalogSearch::loadSettings() {
  QSettings settings;
  settings.beginGroup("/ICatSettings");

  QString lastdir = settings.value("lastDownloadPath").toString();

  // The user has not previously selected a directory to save ICAT downloads to.
  if (lastdir.isEmpty()) {
    lastdir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));
  }
  // Initalise the member variable to the last saved directory.
  m_downloadSaveDir = lastdir;
  settings.endGroup();
}

///////////////////////////////////////////////////////////////////////////////
/// Methods for "Catalog Search".
///////////////////////////////////////////////////////////////////////////////

/**
void CatalogSearch::dateSelected(const std::string &buttonName)
 * Populates the "Instrument" list-box
 */
void CatalogSearch::populateInstrumentBox() {
  // Obtain the list of instruments to display in the drop-box.
  std::vector<std::string> instrumentList =
      m_icatHelper->getInstrumentList(m_catalogSelector->getSelectedCatalogSessions());

  // This option allows the user to select no instruments (thus searching over
  // them all).
  m_icatUiForm.Instrument->insertItem(-1, "");
  m_icatUiForm.Instrument->setCurrentIndex(0);

  QString userInstrument = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getInstrument().name());

  for (unsigned i = 0; i < instrumentList.size(); i++) {
    QString instrument = QString::fromStdString(instrumentList.at(i));
    m_icatUiForm.Instrument->addItem(instrument);

    if (userInstrument.compare(instrument) == 0) {
      m_icatUiForm.Instrument->setCurrentIndex(i + 1);
    }
  }
}

/**
 * Populates the "Investigation type" list-box.
 */
void CatalogSearch::populateInvestigationTypeBox() {
  // Obtain the list of investigation types to display in the list-box.
  std::vector<std::string> invesTypeList =
      m_icatHelper->getInvestigationTypeList(m_catalogSelector->getSelectedCatalogSessions());

  std::vector<std::string>::const_iterator citr;
  for (citr = invesTypeList.begin(); citr != invesTypeList.end(); ++citr) {
    // Add each instrument to the instrument box.
    m_icatUiForm.InvestigationType->addItem(QString::fromStdString(*citr));
  }

  // Sort the list-box by investigation type.
  m_icatUiForm.InvestigationType->model()->sort(0);
  // Make the default investigation type empty so the user has to select one.
  m_icatUiForm.InvestigationType->insertItem(-1, "");
  m_icatUiForm.InvestigationType->setCurrentIndex(0);
}

/**
 * Get the users' input for each search field.
 * @return A map containing all users' search fields - (key => FieldName, value
 * => FieldValue).
 */
const std::map<std::string, std::string> CatalogSearch::getSearchFields() {
  std::map<std::string, std::string> searchFieldInput;

  // Left side of form.
  searchFieldInput.emplace("InvestigationName", m_icatUiForm.InvestigationName->text().toStdString());
  searchFieldInput.emplace("Instrument", m_icatUiForm.Instrument->currentText().toStdString());
  if (m_icatUiForm.RunRange->text().size() > 2) {
    searchFieldInput.emplace("RunRange", m_icatUiForm.RunRange->text().toStdString());
  }
  searchFieldInput.emplace("InvestigatorSurname", m_icatUiForm.InvestigatorSurname->text().toStdString());
  searchFieldInput.emplace("DataFileName", m_icatUiForm.DataFileName->text().toStdString());
  searchFieldInput.emplace("InvestigationId", m_icatUiForm.InvestigationId->text().toStdString());

  // Right side of form.
  if (m_icatUiForm.StartDate->text().size() > 2) {
    searchFieldInput.emplace("StartDate", m_icatUiForm.StartDate->text().toStdString());
  }
  if (m_icatUiForm.EndDate->text().size() > 2) {
    searchFieldInput.emplace("EndDate", m_icatUiForm.EndDate->text().toStdString());
  }
  searchFieldInput.emplace("Keywords", m_icatUiForm.Keywords->text().toStdString());
  searchFieldInput.emplace("SampleName", m_icatUiForm.SampleName->text().toStdString());
  searchFieldInput.emplace("InvestigationType", m_icatUiForm.InvestigationType->currentText().toStdString());

  // Since we check if the field is empty in the algorithm, there's no need to
  // check if advanced was clicked.
  // If the "My data only" field is checked. We return the state of the checkbox
  // (1 is true, 0 is false).
  searchFieldInput.emplace("MyData", boost::lexical_cast<std::string>(m_icatUiForm.myDataCbox->isChecked()));

  return (searchFieldInput);
}

///////////////////////////////////////////////////////////////////////////////
/// SLOTS for "Catalog Search"
///////////////////////////////////////////////////////////////////////////////

/**
 * Opens the DateTime m_calendar box when start or end date selected.
 */
void CatalogSearch::openCalendar() {
  // Set min/max dates to prevent user selecting unusual dates.
  m_calendar->setMinimumDate(QDate(1950, 1, 1));
  m_calendar->setMaximumDate(QDate(2050, 1, 1));

  // Centre the calendar on screen.
  m_calendar->setGeometry(
      QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(445, 205), QDesktopWidget().availableGeometry()));

  // Improve UX, then display the m_calendar.
  m_calendar->setGridVisible(true);
  m_calendar->setWindowTitle("Calendar picker");
  m_calendar->show();

  // Set the name of the date button the user pressed to open the calendar with.
  m_dateButtonName = sender()->objectName();
}

/**
 * Update text field when date is selected.
 * @param date :: The date the user has selected.
 */
void CatalogSearch::dateSelected(QDate date) {
  // As openCalendar slot is used for both start and end date we need to perform
  // a check
  // to see which button was pressed, and then updated the related input field.
  if (m_dateButtonName.compare("startDatePicker") == 0) {
    m_icatUiForm.StartDate->setText(date.toString("dd/MM/yyyy"));
  } else {
    m_icatUiForm.EndDate->setText(date.toString("dd/MM/yyyy"));
  }
  m_calendar->close();
}

/**
 * Checks if start date is greater than end date.
 * @returns true if start date is greater than end date.
 */
bool CatalogSearch::validateDates() {
  std::string startDateInput = m_icatUiForm.StartDate->text().toStdString();
  std::string endDateInput = m_icatUiForm.EndDate->text().toStdString();

  // Return false if the user has not input any dates. This prevents any null
  // errors occurring.
  if (startDateInput.size() <= 2 || endDateInput.size() <= 2)
    return false;

  bool ret = m_icatHelper->getTimevalue(startDateInput) > m_icatHelper->getTimevalue(endDateInput);
  // If startDate > endDate we want to throw an error and inform the user (red
  // star(*)).
  if (ret) {
    correctedToolTip("Start date cannot be greater than end date.", m_icatUiForm.StartDate_err);
    m_icatUiForm.StartDate_err->show();
  } else {
    m_icatUiForm.StartDate_err->hide();
  }

  return ret;
}

void CatalogSearch::correctedToolTip(const std::string &text, QLabel *label) {
#ifdef Q_OS_WIN
  label->setToolTip(QString::fromStdString("<span style=\"color: black;\">" + text + "</span>"));
#else
  label->setToolTip(QString::fromStdString("<span style=\"color: white;\">" + text + "</span>"));
#endif
}

/**
 * Show or hide advanced options if "Advanced Search" checked.
 */
void CatalogSearch::advancedSearchChecked() {
  if (m_icatUiForm.advSearchCbox->isChecked()) {
    m_icatUiForm.advNameLbl->show();
    m_icatUiForm.InvestigatorSurname->show();
    m_icatUiForm.advDatafileLbl->show();
    m_icatUiForm.DataFileName->show();
    m_icatUiForm.advSampleLbl->show();
    m_icatUiForm.SampleName->show();
    m_icatUiForm.advTypeLbl->show();
    m_icatUiForm.InvestigationType->show();
  } else {
    m_icatUiForm.advNameLbl->hide();
    m_icatUiForm.InvestigatorSurname->hide();
    m_icatUiForm.advDatafileLbl->hide();
    m_icatUiForm.DataFileName->hide();
    m_icatUiForm.advSampleLbl->hide();
    m_icatUiForm.SampleName->hide();
    m_icatUiForm.advTypeLbl->hide();
    m_icatUiForm.InvestigationType->hide();
  }
}

/**
 * Hides the search frame, and shows search results frame when "Search" button
 * pressed.
 */
void CatalogSearch::searchClicked() {
  auto name = sender()->objectName();
  // This allows us to perform paging on each search separately
  // as we call this method in three separate SLOTS (two paging & search
  // button).
  if (name.compare("searchBtn") == 0)
    m_currentPageNumber = 1;

  clearDataFileFrame();

  std::map<std::string, std::string> inputFields = getSearchFields();
  // Contains the error label names, and the related error message.
  std::map<std::string, std::string> errors = m_icatHelper->validateProperties(inputFields);

  // Has any errors occurred?
  if (!errors.empty() || validateDates()) {
    // Clear form to prevent previous search results showing if an error occurs.
    clearSearchResultFrame();
    showErrorLabels(errors);
    m_icatUiForm.searchResultsLbl->setText("An error has occurred in the search form.");
    // Stop here to prevent the search being carried out below.
    return;
  }

  // Performed here to allow the user to login while the search GUI is open,
  // and see results of all facilities/catalogs that they are logged in to.
  m_catalogSelector->populateFacilitySelection();

  // Since there are no longer errors we hide the error labels.
  hideErrorLabels();

  // We want to disable/hide these as a search is in progress, but no results
  // have been obtained.
  m_icatUiForm.resFrame->hide();
  m_icatUiForm.searchResultsCbox->setEnabled(false);
  m_icatUiForm.searchResultsCbox->setChecked(false);

  // Update the label to inform the user that searching is in progress.
  m_icatUiForm.searchResultsLbl->setText("searching investigations...");

  // Get previous sorting parameters
  QTableWidget *resultsTable = m_icatUiForm.searchResultsTbl;
  int sort_section = resultsTable->horizontalHeader()->sortIndicatorSection();
  Qt::SortOrder sort_order = resultsTable->horizontalHeader()->sortIndicatorOrder();

  // Remove previous search results.
  std::string searchResults = "searchResults";

  clearSearch(resultsTable, searchResults);

  auto sessionIDs = m_catalogSelector->getSelectedCatalogSessions();
  // Obtain the number of results for paging.
  int64_t numrows = m_icatHelper->getNumberOfSearchResults(inputFields, sessionIDs);

  // Setup values used for paging.
  int limit = 100;
  // Have to cast either numrows or limit to double for ceil to work correctly.
  double totalNumPages = ceil(static_cast<double>(numrows) / limit);
  int offset = (m_currentPageNumber - 1) * limit;

  // Set paging labels.
  m_icatUiForm.pageStartNum->setText(QString::number(m_currentPageNumber));
  m_icatUiForm.resPageEndNumTxt->setText(QString::number(totalNumPages));

  // Perform a search using paging (E.g. return only n from m).
  m_icatHelper->executeSearch(inputFields, offset, limit, sessionIDs);

  // Update the label to inform the user of how many investigations have been
  // returned from the search.
  m_icatUiForm.searchResultsLbl->setText(QString::number(numrows) + " investigations found.");

  // Populate the result table from the searchResult workspace.
  populateResultTable(sort_section, sort_order);
}

/**
 * Show the error message labels, including the error message on the tooltips.
 * @param errors :: A map containing the error label names, and the related
 * error message.
 */
void CatalogSearch::showErrorLabels(std::map<std::string, std::string> &errors) {
  for (auto &error : errors) {
    QLabel *label = m_icatUiForm.searchFrame->findChild<QLabel *>(QString::fromStdString(error.first));

    if (label) {
      // Update the tooltip of the element and then show it.
      correctedToolTip(error.second, label);
      label->show();
    }
  }
}

/**
 * Hides the error message labels on the GUI.
 */
void CatalogSearch::hideErrorLabels() {
  // Left side of form.
  m_icatUiForm.InvestigationName_err->setVisible(false);
  m_icatUiForm.Instrument_err->setVisible(false);
  m_icatUiForm.RunRange_err->setVisible(false);
  m_icatUiForm.InvestigationId_err->setVisible(false);
  m_icatUiForm.InvestigatorSurname_err->setVisible(false);
  m_icatUiForm.InvestigationAbstract_err->setVisible(false);
  // Right side of form.
  m_icatUiForm.StartDate_err->setVisible(false);
  m_icatUiForm.EndDate_err->setVisible(false);
  m_icatUiForm.Keywords_err->setVisible(false);
  m_icatUiForm.SampleName_err->setVisible(false);
  m_icatUiForm.InvestigationType_err->setVisible(false);
}

/**
 * Reset all fields when the "Reset" button is pressed.
 */
void CatalogSearch::onReset() {
  // Clear the QLineEdit boxes.
  foreach (QLineEdit *widget, this->findChildren<QLineEdit *>()) {
    widget->clear();
  }
  // Clear all other elements.
  m_icatUiForm.Instrument->setCurrentIndex(0);
  m_icatUiForm.InvestigationType->setCurrentIndex(0);
  m_icatUiForm.advSearchCbox->setChecked(false);
  m_icatUiForm.myDataCbox->setChecked(false);
}

/**
 *  Allows a user to select specific facilities that they want to search the
 * catalogs of.
 */
void CatalogSearch::openFacilitySelection() {
  m_catalogSelector->populateFacilitySelection();
  m_catalogSelector->show();
  m_catalogSelector->raise();
}

///////////////////////////////////////////////////////////////////////////////
// Methods for "Search results"
///////////////////////////////////////////////////////////////////////////////

/**
 * Outputs the results of the search into the "Search results" table.
 * @param sort_section :: An int giving the column number by which to sort the
 * data (0 will sort by StartDate)
 * @param sort_order :: A Qt::SortOrder giving the order of sorting
 */
void CatalogSearch::populateResultTable(int sort_section, Qt::SortOrder sort_order) {
  // Obtain a pointer to the "searchResults" workspace where the search results
  // are saved if it exists.
  Mantid::API::ITableWorkspace_sptr workspace;

  // Check to see if the workspace exists...
  if (Mantid::API::AnalysisDataService::Instance().doesExist("__searchResults")) {
    workspace = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("__searchResults"));
  } else {
    // Otherwise an error will be thrown (in ICat4Catalog). We will reproduce
    // that error on the ICAT form for the user.
    m_icatUiForm.searchResultsLbl->setText("You have not input any terms to search for.");
    return;
  }

  // If there are no results then clear search form and don't try to setup
  // table.
  if (workspace->rowCount() == 0) {
    clearSearchResultFrame();
    return;
  }

  // Create local variable for convenience and reusability.
  QTableWidget *resultsTable = m_icatUiForm.searchResultsTbl;

  // Set the result's table properties prior to adding data.
  setupTable(resultsTable, workspace->rowCount(), workspace->columnCount());

  // Update the label to inform the user of how many investigations have been
  // returned from the search.

  // We want to show this now as we are certain that search results exist, and
  // not display a blank frame (bad UX).
  m_icatUiForm.resFrame->show();
  m_icatUiForm.searchResultsCbox->setEnabled(true);
  m_icatUiForm.searchResultsCbox->setChecked(true);

  // Add data from the workspace to the results table.
  populateTable(resultsTable, workspace);

  // Show only a portion of the title as they can be quite long.
  resultsTable->setColumnWidth(headerIndexByName(resultsTable, "Title"), 210);
  resultsTable->setColumnHidden(headerIndexByName(resultsTable, "DatabaseID"), true);
  resultsTable->setColumnHidden(headerIndexByName(resultsTable, "SessionID"), true);

  // Resize InvestigationID column to fit contents
  resultsTable->resizeColumnToContents(headerIndexByName(resultsTable, "InvestigationID"));

  // Sort by specified column or by descending StartDate if none specified
  resultsTable->setSortingEnabled(true);
  if (sort_section == 0) {
    resultsTable->sortByColumn(headerIndexByName(resultsTable, "Start date"), Qt::DescendingOrder);
  } else {
    resultsTable->sortByColumn(sort_section, sort_order);
  }
}

/**
 * Obtain the sessionID for the specific row selected in the search results
 * table.
 * (only one row can be selected at a time)
 * @return The sessionID from the selected row.
 */
std::string CatalogSearch::selectedInvestigationSession() {
  auto searchResultsTable = m_icatUiForm.searchResultsTbl;
  return searchResultsTable
      ->item(searchResultsTable->selectionModel()->selectedRows().at(0).row(),
             headerIndexByName(searchResultsTable, "SessionID"))
      ->text()
      .toStdString();
}

///////////////////////////////////////////////////////////////////////////////
// SLOTS for "Search results"
///////////////////////////////////////////////////////////////////////////////

/**
 * Populate the result table, and update the page number.
 */
void CatalogSearch::nextPageClicked() {
  int totalNumPages = m_icatUiForm.resPageEndNumTxt->text().toInt();
  // Prevent user from pressing "next" when no more investigations exist.
  if (m_currentPageNumber >= totalNumPages) {
    m_currentPageNumber = totalNumPages;
    return;
  }
  // Increment here as we need to validate page number above.
  m_currentPageNumber++;
  // Perform the search, and update the table with the new results using the
  // current page num.
  searchClicked();
}

/**
 * Populate the result table, and update the page number.
 */
void CatalogSearch::prevPageClicked() {
  m_currentPageNumber--;
  // Prevent user from pressing "Previous" when no investigations exist.
  if (m_currentPageNumber <= 0) {
    m_currentPageNumber = 1;
    return;
  }
  searchClicked();
}

/**
 * Populate's result table depending page number input by user.
 */
void CatalogSearch::goToInputPage() {
  int pageNum = m_icatUiForm.pageStartNum->text().toInt();
  // If the user inputs a page number larger than the total
  // amount of page numbers we do not want to do anything.
  if (pageNum > m_icatUiForm.resPageEndNumTxt->text().toInt() || pageNum <= 0) {
    m_icatUiForm.pageStartNum->setText(QString::number(m_currentPageNumber));
    return;
  }
  m_currentPageNumber = pageNum;
  searchClicked();
}

/**
 * Hides the "search results" frame, and shows the "dataFiles" frame when an
 * investigation is selected.
 */
void CatalogSearch::investigationSelected(QTableWidgetItem *item) {
  clearSearchFrame();
  //
  m_icatUiForm.dataFileCbox->setEnabled(true);
  m_icatUiForm.dataFileCbox->setChecked(true);
  m_icatUiForm.dataFileFrame->show();
  // Have to clear the combo-box in order to prevent the user from seeing the
  // extensions of previous search.
  m_icatUiForm.dataFileFilterCombo->clear();
  m_icatUiForm.dataFileFilterCombo->addItem("No filter");

  // Inform the user that the search is in progress.
  m_icatUiForm.dataFileLbl->setText("searching for related datafiles...");

  QTableWidget *searchResultsTable = m_icatUiForm.searchResultsTbl;

  // Obtain the investigationID from the selected
  QTableWidgetItem *investigationId =
      searchResultsTable->item(item->row(), headerIndexByName(searchResultsTable, "InvestigationID"));

  // Remove previous dataFile search results.
  std::string dataFileResults = "dataFileResults";
  clearSearch(m_icatUiForm.dataFileResultsTbl, dataFileResults);

  // Update the labels in the dataFiles information frame to show relevant info
  // to use.
  updateDataFileLabels(item);

  // Perform the "search" and obtain the related data files for the selected
  // investigation.
  m_icatHelper->executeGetDataFiles(
      investigationId->text().toStdString(),
      searchResultsTable->item(item->row(), headerIndexByName(searchResultsTable, "SessionID"))->text().toStdString());

  // Populate the dataFile table from the "dataFileResults" workspace.
  populateDataFileTable();
}

///////////////////////////////////////////////////////////////////////////////
// Methods for "DataFile information"
///////////////////////////////////////////////////////////////////////////////

/**
 * Outputs related dataFiles (from selected investigation) into the "DataFile
 * information" table.
 */
void CatalogSearch::populateDataFileTable() {
  // Obtain a pointer to the "dataFileResults" workspace where the related
  // datafiles for the user selected investigation exist.
  Mantid::API::ITableWorkspace_sptr workspace;

  // Check to see if the workspace exists...
  if (Mantid::API::AnalysisDataService::Instance().doesExist("__dataFileResults")) {
    workspace = std::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("__dataFileResults"));
  } else {
    return;
  }

  // If there are no results then don't try to setup table.
  if (workspace->rowCount() == 0) {
    clearDataFileFrame();
    m_icatUiForm.dataFileLbl->setText(QString::number(workspace->rowCount()) + " datafiles found.");
    return;
  }

  // Create local variable for convenience and reusability.
  QTableWidget *dataFileTable = m_icatUiForm.dataFileResultsTbl;

  // Set the result's table properties prior to adding data.
  setupTable(dataFileTable, workspace->rowCount(), workspace->columnCount());

  // Update the label to inform the user of how many dataFiles relating to the
  // selected investigation have been found.
  m_icatUiForm.dataFileLbl->setText(QString::number(workspace->rowCount()) + " datafiles found.");

  // Create the custom header with checkbox ability.
  m_customHeader = new CheckboxHeader(Qt::Horizontal, dataFileTable);

  // There is no simple way to override default QTableWidget sort.
  // Instead, connecting header to obtain column clicked, and sorting by
  connect(m_customHeader, SIGNAL(sectionClicked(int)), this, SLOT(sortByFileSize(int)));

  // Set it prior to adding labels in populateTable.
  dataFileTable->setHorizontalHeader(m_customHeader);

  // Add data from the workspace to the results table.
  populateTable(dataFileTable, workspace);

  // As a new column is being added we do this after populateTable to prevent
  // null errors.
  addCheckBoxColumn(dataFileTable);

  // Resize the columns to improve the viewing experience.
  // Has been called here since we added the checkbox column after populating
  // table.
  dataFileTable->resizeColumnsToContents();

  // Hide these columns as they're not useful for the user, but are used by the
  // algorithms.
  dataFileTable->setColumnHidden(headerIndexByName(dataFileTable, "Id"), true);
  dataFileTable->setColumnHidden(headerIndexByName(dataFileTable, "Location"), true);
  dataFileTable->setColumnHidden(headerIndexByName(dataFileTable, "File size(bytes)"), true);

  // Obtain the list of extensions of all dataFiles for the chosen
  // investigation.
  // "File name" is the first column of "dataFileResults" so we make use of it.
  auto extensions = getDataFileExtensions(workspace.get()->getColumn(headerIndexByName(dataFileTable, "Name")));

  // Populate the "Filter type..." combo-box with all possible file extensions.
  populateDataFileType(extensions);

  // Sort by create time with the most recent being first.
  dataFileTable->setSortingEnabled(true);
  dataFileTable->sortByColumn(headerIndexByName(dataFileTable, "Name"), Qt::DescendingOrder);
}

/**
 * Add a row of checkboxes to the first column of a table.
 * @param table :: The table to add the checkboxes to.
 */
void CatalogSearch::addCheckBoxColumn(QTableWidget *table) {
  // Add a new column checkbox column.
  table->insertColumn(0);
  // Add a new header item to this column. This allows us to overwrite the
  // default text!
  table->setHorizontalHeaderItem(0, new QTableWidgetItem());
  // Set this here (rather than on initialisation) as the customer header would
  // be null otherwise.
  connect(m_customHeader, SIGNAL(toggled(bool)), this, SLOT(selectAllDataFiles(bool)));

  // Add a checkbox to all rows in the first column.
  for (int row = 0; row < table->rowCount(); row++) {
    auto *newItem = new QTableWidgetItem();
    // Allow the widget to take on checkbox functionality.
    newItem->setCheckState(Qt::Unchecked);
    // Allow the user to select and check the box.
    newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    // Add a result to the table.
    table->setItem(row, 0, newItem);
  }
}

/**
 * Obtains the names of the selected dataFiles, in preparation for download.
 * @return A vector containing the fileID and fileName of the datafile(s) to
 * download.
 */
const std::vector<std::pair<int64_t, std::string>> CatalogSearch::selectedDataFileNames() {
  QTableWidget *table = m_icatUiForm.dataFileResultsTbl;

  // Holds the FileID, and fileName in order to perform search to download
  // later.
  std::vector<std::pair<int64_t, std::string>> fileInfo;

  for (int row = 0; row < table->rowCount(); row++) {
    if (table->item(row, 0)->checkState()) {
      fileInfo.emplace_back(table->item(row, headerIndexByName(table, "Id"))->text().toLongLong(),
                            table->item(row, headerIndexByName(table, "Name"))->text().toStdString());
    }
  }
  return (fileInfo);
}

/**
 * Updates the dataFile text boxes with relevant info regarding the selected
 * dataFile.
 */
void CatalogSearch::updateDataFileLabels(QTableWidgetItem *item) {
  QTableWidget *searchResultsTable = m_icatUiForm.searchResultsTbl;

  // Set the "title" label using the data from the investigation results
  // workspace.
  m_icatUiForm.dataFileTitleRes->setText(
      searchResultsTable->item(item->row(), headerIndexByName(searchResultsTable, "Title"))->text());

  // Set the instrument label using data from the investigation results
  // workspace.
  m_icatUiForm.dataFileInstrumentRes->setText(
      searchResultsTable->item(item->row(), headerIndexByName(searchResultsTable, "Instrument"))->text());

  // Show the related "run-range" for the specific dataFiles.
  m_icatUiForm.dataFileRunRangeRes->setText(
      searchResultsTable->item(item->row(), headerIndexByName(searchResultsTable, "Run range"))->text());
}

/**
 * Obtain all file extensions from the provided column.
 * @param column :: The fileName column in the dataFile workspace.
 * @return A set containing all file extensions.
 */
std::unordered_set<std::string> CatalogSearch::getDataFileExtensions(const Mantid::API::Column_sptr &column) {
  std::unordered_set<std::string> extensions;

  // For every filename in the column...
  for (unsigned row = 0; row < column->size(); row++) {
    // Add the file extension to the set if it does not exist.
    QString extension = QString::fromStdString(Poco::Path(column->cell<std::string>(row)).getExtension());
    extensions.insert(extension.toLower().toStdString());
  }

  return (extensions);
}

/**
 * Add the list of file extensions to the "Filter type..." drop-down.
 */
void CatalogSearch::populateDataFileType(const std::unordered_set<std::string> &extensions) {
  for (const auto &extension : extensions) {
    m_icatUiForm.dataFileFilterCombo->addItem(QString::fromStdString("." + extension));
  }
}

/**
 * Disable the download button if user can access the files locally from the
 * archives.
 * @param row :: The row the user has selected from the table.
 */
void CatalogSearch::disableDownloadButtonIfArchives(int row) {
  using namespace Mantid::Kernel;

  QTableWidget *table = m_icatUiForm.dataFileResultsTbl;
  // The location of the file selected in the archives.
  std::string location = table->item(row, headerIndexByName(table, "Location"))->text().toStdString();

  // Create and use the user-set ConfigInformation.
  std::unique_ptr<CatalogConfigService> catConfigService(makeCatalogConfigServiceAdapter(ConfigService::Instance()));
  UserCatalogInfo catalogInfo(ConfigService::Instance().getFacility().catalogInfo(), *catConfigService);

  std::string fileLocation = catalogInfo.transformArchivePath(location);

  std::ifstream hasAccessToArchives(fileLocation.c_str());
  if (hasAccessToArchives) {
    m_icatUiForm.dataFileDownloadBtn->setEnabled(false);
  } else {
    m_icatUiForm.dataFileDownloadBtn->setEnabled(true);
  }
  // Allow the user to load the datafile regardless.
  m_icatUiForm.dataFileLoadBtn->setEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////
// SLOTS for: "DataFile information"
///////////////////////////////////////////////////////////////////////////////

/**
 * Disable the load/download button to prevent the user from downloading/loading
 * nothing.
 */
void CatalogSearch::disableDatafileButtons() {
  if (m_icatUiForm.dataFileResultsTbl->selectionModel()->selection().indexes().empty()) {
    m_icatUiForm.dataFileDownloadBtn->setEnabled(false);
    m_icatUiForm.dataFileLoadBtn->setEnabled(false);
  }
}

/**
 * Performs filter option for specified filer type.
 */
void CatalogSearch::doFilter(const int &index) {
  QTableWidget *table = m_icatUiForm.dataFileResultsTbl;

  for (int row = 0; row < table->rowCount(); ++row) {
    // Hide row by default, in order to show only relevant ones.
    table->setRowHidden(row, true);

    // As we use checkboxes in "selectedDataFileNames" to obtain the row of the
    // file the user wants to download
    // the state of these checkboxes remain when filtered. Thus we un-check them
    // here to prevent accidently downloading.
    table->item(row, 0)->setCheckState(Qt::Unchecked);

    QTableWidgetItem *item = table->item(row, headerIndexByName(table, "Name"));
    // Show the relevant rows depending on file extension. 0 index is "Filter
    // type..." so all will be shown.
    // Have to convert to lowercase as ".TXT", and ".txt" should be filtered as
    // the same.
    if (index == 0 || (item->text().toLower().contains(m_icatUiForm.dataFileFilterCombo->itemText(index).toLower()))) {
      table->setRowHidden(row, false);
    }
  }
}

/**
 * Downloads selected datFiles to a specified location.
 */
void CatalogSearch::downloadDataFiles() {
  QString downloadSavePath = QFileDialog::getExistingDirectory(this, tr("Select a directory to save data files."),
                                                               m_downloadSaveDir, QFileDialog::ShowDirsOnly);

  // The user has clicked "Open" and changed the path (and not clicked cancel).
  if (!downloadSavePath.isEmpty()) {
    // Set setting prior to saving.
    m_downloadSaveDir = downloadSavePath;
    // Save settings to store for use next time.
    saveSettings();
    // Download the selected dataFiles to the chosen directory.
    m_icatHelper->downloadDataFiles(selectedDataFileNames(), m_downloadSaveDir.toStdString(),
                                    selectedInvestigationSession());
  }
}

/**
 * Loads the selected dataFiles into workspaces.
 */
void CatalogSearch::loadDataFiles() {
  // Get the path(s) to the file that was downloaded (via HTTP) or is stored in
  // the archive.
  std::vector<std::string> filePaths = m_icatHelper->downloadDataFiles(
      selectedDataFileNames(), m_downloadSaveDir.toStdString(), selectedInvestigationSession());

  // Create & initialize the load algorithm we will use to load the file by path
  // to a workspace.
  auto loadAlgorithm = Mantid::API::AlgorithmManager::Instance().create("Load");
  loadAlgorithm->initialize();

  // For all the files downloaded (or in archive) we want to load them.
  for (auto &filePath : filePaths) {
    if (filePath.empty())
      return;
    // Set the filename (path) of the algorithm to load from.
    loadAlgorithm->setPropertyValue("Filename", filePath);
    // Sets the output workspace to be the name of the file.
    loadAlgorithm->setPropertyValue("OutputWorkspace", Poco::Path(Poco::Path(filePath).getFileName()).getBaseName());

    Poco::ActiveResult<bool> result(loadAlgorithm->executeAsync());
    while (!result.available()) {
      QCoreApplication::processEvents();
    }
  }
}

/**
 * If the user has checked "check all", then check and select ALL rows.
 * Otherwise, deselect all.
 * @param toggled :: True if user has checked the checkbox in the dataFile table
 * header.
 */
void CatalogSearch::selectAllDataFiles(const bool &toggled) {
  QTableWidget *table = m_icatUiForm.dataFileResultsTbl;

  // Used to gain easier access to table selection.
  QItemSelectionModel *selectionModel = table->selectionModel();

  // Select or deselect all rows depending on toggle.
  if (toggled)
    table->selectAll();
  else
    selectionModel->select(selectionModel->selection(), QItemSelectionModel::Deselect);

  // Check/un-check the checkboxes of each row.
  for (int row = 0; row < table->rowCount(); ++row) {
    if (toggled)
      table->item(row, 0)->setCheckState(Qt::Checked);
    else
      table->item(row, 0)->setCheckState(Qt::Unchecked);
  }
}

/**
 * Select/Deselect row when related checkbox is selected.
 * @param item :: The item from the table the user has selected.
 */
void CatalogSearch::dataFileCheckboxSelected(QTableWidgetItem *item) {
  QTableWidget *table = m_icatUiForm.dataFileResultsTbl;

  QTableWidgetItem *checkbox = table->item(item->row(), 0);

  for (int col = 0; col < table->columnCount(); col++) {
    for (int row = 0; row < table->rowCount(); ++row) {
      if (checkbox->checkState()) {
        table->item(item->row(), 0)->setCheckState(Qt::Checked);
        table->item(item->row(), col)->setSelected(true);
      } else {
        table->item(item->row(), 0)->setCheckState(Qt::Unchecked);
        // There is no easier way to deselect an item...
        table->item(item->row(), col)->setSelected(false);
      }
    }
  }
}

/**
 * Select/Deselect row & check-box when a row is selected.
 */
void CatalogSearch::dataFileRowSelected() {
  QTableWidget *table = m_icatUiForm.dataFileResultsTbl;

  for (int row = 0; row < table->rowCount(); ++row) {
    // We Uncheck here to prevent previously selected items staying selected
    // and to allow dataFileCheckboxSelected() to function correctly.
    if (!table->item(row, 0)->isSelected()) {
      table->item(row, 0)->setCheckState(Qt::Unchecked);
    }
  }

  QModelIndexList indexes = table->selectionModel()->selectedRows();

  for (int i = 0; i < indexes.count(); ++i) {
    int row = indexes.at(i).row();
    table->item(row, 0)->setCheckState(Qt::Checked);
    /// Disable/Enable download button if user has access to the archives.
    disableDownloadButtonIfArchives(row);
  }
  // Disable load/download buttons if no datafile is selected.
  disableDatafileButtons();
}

/**
 * When the user clicks "File size" column sort the table by "File size(bytes)".
 * @param column :: The column that was clicked by the user.
 */
void CatalogSearch::sortByFileSize(int column) {
  QTableWidget *table = m_icatUiForm.dataFileResultsTbl;
  int byteColumn = headerIndexByName(table, "File size(bytes)");

  if (column == headerIndexByName(table, "File size")) {
    // Convert cell value to int within the datamodel.
    // This allows us to sort by the specific column.
    for (int row = 0; row < table->rowCount(); row++) {
      auto *item = new QTableWidgetItem;
      item->setData(Qt::EditRole, table->item(row, byteColumn)->text().toInt());
      table->setItem(row, byteColumn, item);
    }
    table->sortByColumn(byteColumn);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
