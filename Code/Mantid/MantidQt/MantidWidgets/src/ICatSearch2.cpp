#include "MantidQtMantidWidgets/ICatSearch2.h"
#include "MantidAPI/AnalysisDataService.h"
#include <Poco/Path.h>

#include <QDesktopServices>
#include <QUrl>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
     * Constructor
     */
    ICatSearch2::ICatSearch2(QWidget* parent) : QWidget(parent)
    {
      // Verify if the user has logged in.
      // if they have: continue...
      initLayout();
    }

    /**
     * Destructor
     */
    ICatSearch2::~ICatSearch2(){}

    /**
     * Initialise the  default layout.
     */
    void ICatSearch2::initLayout()
    {
      // Draw the GUI from .ui header generated file.
      m_icatUiForm.setupUi(this);

      // Hide the three frames
      m_icatUiForm.searchFrame->hide();
      m_icatUiForm.resFrame->hide();
      m_icatUiForm.dataFileFrame->hide();

      // Hide advanced input fields until "Advanced search" is checked.
      advancedSearchChecked();

      // Disable buttons except search by default.
      m_icatUiForm.searchResultsCbox->setEnabled(false);
      m_icatUiForm.dataFileCbox->setEnabled(false);

      // Show related help page when a user clicks on the "Help" button.
      connect(m_icatUiForm.helpBtn,SIGNAL(clicked()),this,SLOT(helpClicked()));
      // Show "Search" frame when user clicks "Catalog search" check box.
      connect(m_icatUiForm.searchCbox,SIGNAL(clicked()),this,SLOT(showCatalogSearch()));
      // Show advanced search options if "Advanced search" is checked.
      connect(m_icatUiForm.advSearchCbox,SIGNAL(clicked()),this,SLOT(advancedSearchChecked()));
      // Open calendar when start or end date is selected
      connect(m_icatUiForm.startDatePicker,SIGNAL(clicked()),this, SLOT(openCalendar()));
      connect(m_icatUiForm.endDatePicker,SIGNAL(clicked()),this, SLOT(openCalendar()));
      // Clear all fields when reset button is pressed.
      connect(m_icatUiForm.resetBtn,SIGNAL(clicked()),this,SLOT(onReset()));
      // Show "Search results" frame when user tries to "Search".
      connect(m_icatUiForm.searchBtn,SIGNAL(clicked()),this,SLOT(searchClicked()));
      // Show "Search results" frame when user clicks related check box.
      connect(m_icatUiForm.searchResultsCbox,SIGNAL(clicked()),this,SLOT(showSearchResults()));
      // When the user has double clicked on an investigation they wish to view datafiles for then load the relevant datafiles.
      connect(m_icatUiForm.searchResultsTbl,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(investigationSelected(QTableWidgetItem*)));
      // Show "DataFile frame" when the user selects an investigation.
      connect(m_icatUiForm.dataFileCbox,SIGNAL(clicked()),this,SLOT(showDataFileInfo()));
      // When the user has selected a filter type then perform the filter for the specified type.
      connect(m_icatUiForm.dataFileFilterCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(doFilter(int)));

      // No need for error handling as that's dealt with in the algorithm being used.
      populateInstrumentBox();
      // Although this is an advanced option performing it here allows it to be performed once only.
      populateInvestigationTypeBox();

      // Resize to minimum width/height to improve UX.
      this->resize(minimumSizeHint());
      // Auto contract GUI to improve UX.
      this->layout()->setSizeConstraint(QLayout::SetFixedSize);
    }

    /**
     * Opens the login dialog to allow the user to log into another facility.
     */
    void ICatSearch2::onFacilityLogin()
    {

    }

    /**
     * Sends the user to relevant search page on the Mantid project site.
     */
    void ICatSearch2::helpClicked()
    {
      QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Search"));
    }

    /**
     * Shows/hides the "Catalog search" frame when search combo box is checked.
     */
    void ICatSearch2::showCatalogSearch()
    {
      if (m_icatUiForm.searchCbox->isChecked())
      {
        m_icatUiForm.searchFrame->show();
      }
      else
      {
        m_icatUiForm.searchFrame->hide();
      }
    }

    /**
     * Shows/Hides the "Search results" frame when search results combo box is checked.
     */
    void ICatSearch2::showSearchResults()
    {
      if (m_icatUiForm.searchResultsCbox->isChecked())
      {
        m_icatUiForm.resFrame->show();
      }
      else
      {
        m_icatUiForm.resFrame->hide();
      }
    }

    /**
     * Hides "Search results" frame when a result is double clicked.
     */
    void ICatSearch2::showDataFileInfo()
    {
      if (m_icatUiForm.dataFileCbox->isChecked())
      {
        m_icatUiForm.dataFileFrame->show();
      }
      else
      {
        m_icatUiForm.dataFileFrame->hide();
      }
    }

    /**
     * Embolden the headers in the provided table.
     */
    void ICatSearch2::emboldenTableHeaders(QTableWidget* table)
    {
      QFont font;
      font.setBold(true);
      for (int i = 0; i < table->columnCount(); ++i)
      {
        table->horizontalHeaderItem(i)->setFont(font);
      }
    }

    /**
     * Set the table properties prior to adding data to it.
     * @param table        :: The table we want to setup.
     * @param numOfRows    :: The number of rows in the workspace.
     * @param numOfColumns :: The number of columns in the workspace.
     */
    void ICatSearch2::setupTable(QTableWidget* table, size_t numOfRows, size_t numOfColumns)
    {
      table->setRowCount(static_cast<int>(numOfRows));
      table->setColumnCount(static_cast<int>(numOfColumns));

      // Improve the appearance of table to make it easier to read.
      table->setAlternatingRowColors(true);
      table->setStyleSheet("alternate-background-color: rgb(216, 225, 255)");
      table->setSortingEnabled(false);
      table->verticalHeader()->setVisible(false);

      // Sort by endDate with the most recent being first.
      table->sortByColumn(4,Qt::DescendingOrder);
      table->setSortingEnabled(true);

      // Set the height on each row to 20 for UX improvement.
      for (size_t i = 0; i < numOfRows; ++i)
      {
        table->setRowHeight(static_cast<int>(i),20);
      }
    }

    /**
     * Populate the provided table with data from the provided workspace.
     * @param table :: The table we want to setup.
     */
    void ICatSearch2::populateTable(QTableWidget* table, Mantid::API::ITableWorkspace_sptr workspace)
    {
      //NOTE: This method freezes up the ICAT search GUI. We will need to do this adding in another thread.

      // This will contain the list of column names.
      QStringList columnHeaders;

      // Add the data from the workspace to the search results table.
      for(size_t col = 0 ; col < workspace->columnCount(); col++)
      {
        // Get the column name to display as the header of table widget
        Mantid::API::Column_sptr column = workspace->getColumn(col);
        columnHeaders.push_back(QString::fromStdString(column->name()));

        for(size_t row = 0; row < workspace->rowCount(); ++row)
        {
          // Prints the value from the row to the ostringstream to use later.
          std::ostringstream ostr;
          column->print(row,ostr);

          // Add a result to the table.
          QTableWidgetItem *newItem  = new QTableWidgetItem(QString::fromStdString(ostr.str()));
          table->setItem(static_cast<int>(row),static_cast<int>(col), newItem);

          // Allow the row to be selected, and enabled.
          newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
          newItem->setToolTip(QString::fromStdString(ostr.str()));
        }
      }

      // Set the table widgets header labels from the table workspace.
      table->setHorizontalHeaderLabels(columnHeaders);

      // Resize the label to improve the viewing experience.
      table->resizeColumnsToContents();
    }

    /**
     * Clears data associated with previous search.
     * @param table     :: The table to modify and remove previous results from.
     * @param workspace :: The workspace to remove.
     */
    void ICatSearch2::clearSearch(QTableWidget* table, std::string & workspace)
    {
      // Remove workspace if it exists.
      if(Mantid::API::AnalysisDataService::Instance().doesExist(workspace))
      {
        Mantid::API::AnalysisDataService::Instance().remove(workspace);
      }

      // In order to reset fields for the table
      setupTable(table, 0, 0);

    }
    ///////////////////////////////////////////////////////////////////////////////
    /// Methods for "Catalog Search".
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Updates text field depending on button picker selected.
     * @param buttonName :: The name of the text field is derived from the buttonName.
     */
    void ICatSearch2::dateSelected(std::string buttonName)
    {
      if (buttonName.compare("startDatePicker") == 0)
      {
        // Since the user wants to select a startDate we disable the endDate button to prevent any issues.
        m_icatUiForm.endDatePicker->setEnabled(false);
        // Update the text field and re-enable the button.
        connect(m_calendar, SIGNAL(selectionChanged()),this, SLOT(updateStartDate()));
      }
      else
      {
        m_icatUiForm.startDatePicker->setEnabled(false);
        connect(m_calendar, SIGNAL(selectionChanged()),this, SLOT(updateEndDate()));
      }
    }

    /**
     * Populates the "Instrument" list-box
     */
    void ICatSearch2::populateInstrumentBox()
    {
      // Obtain the list of instruments to display in the drop-box.
      std::vector<std::string> instrumentList = m_icatHelper->getInstrumentList();

      std::vector<std::string>::const_iterator citr;
      for (citr = instrumentList.begin(); citr != instrumentList.end(); ++citr)
      {
        // Add each instrument to the instrument box.
        m_icatUiForm.Instrument->addItem(QString::fromStdString(*citr));
      }
      // Sort the drop-box by instrument name.
      m_icatUiForm.Instrument->model()->sort(0);
      // Make the default instrument empty so the user has to select one.
      m_icatUiForm.Instrument->insertItem(-1,"");
      m_icatUiForm.Instrument->setCurrentIndex(0);
    }

    /**
     * Populates the "Investigation type" list-box.
     */
    void ICatSearch2::populateInvestigationTypeBox()
    {
      // Obtain the list of investigation types to display in the list-box.
      std::vector<std::string> invesTypeList = m_icatHelper->getInvestigationTypeList();

      std::vector<std::string>::const_iterator citr;
      for (citr = invesTypeList.begin(); citr != invesTypeList.end(); ++citr)
      {
        // Add each instrument to the instrument box.
        m_icatUiForm.InvestigationType->addItem(QString::fromStdString(*citr));
      }

      // Sort the list-box by investigation type.
      m_icatUiForm.InvestigationType->model()->sort(0);
      // Make the default investigation type empty so the user has to select one.
      m_icatUiForm.InvestigationType->insertItem(-1,"");
      m_icatUiForm.InvestigationType->setCurrentIndex(0);
    }

    /**
     * Get the users' input for each search field.
     * @return A map containing all users' search fields - (key => FieldName, value => FieldValue).
     */
    std::map<std::string, std::string> ICatSearch2::getSearchFields()
    {
      std::map<std::string, std::string> searchFieldInput;

      // Left side of form.
      searchFieldInput.insert(std::pair<std::string, std::string>("InvestigationName", m_icatUiForm.InvestigationName->text().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("Instrument", m_icatUiForm.Instrument->currentText().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("runRange", m_icatUiForm.runRangeTxt->text().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("InvestigatorSurname", m_icatUiForm.InvestigatorSurname->text().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("InvestigationAbstract", m_icatUiForm.InvestigationAbstract->text().toStdString()));

      // Right side of form.
      searchFieldInput.insert(std::pair<std::string, std::string>("StartDate", m_icatUiForm.StartDate->text().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("EndDate", m_icatUiForm.EndDate->text().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("Keywords", m_icatUiForm.Keywords->text().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("SampleName", m_icatUiForm.SampleName->text().toStdString()));
      searchFieldInput.insert(std::pair<std::string, std::string>("InvestigationType", m_icatUiForm.InvestigationType->currentText().toStdString()));

      // Since we check if the field is empty in the algorithm, there's no need to check if advanced was clicked.
      // If the "My data only" field is checked. We return the state of the checkbox (1 is true, 0 is false).
      searchFieldInput.insert(std::pair<std::string, std::string>("myData", boost::lexical_cast<std::string>(m_icatUiForm.myDataCbox->isChecked())));

      return (searchFieldInput);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// SLOTS for "Catalog Search"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Opens the DateTime m_calendar box when start or end date selected.
     */
    void ICatSearch2::openCalendar()
    {
      // Pop the m_calendar out into it's own window.
      QWidget* parent = qobject_cast<QWidget*>(this->parent());
      m_calendar = new QCalendarWidget(parent);

      // Set min/max dates to prevent user selecting unusual dates.
      m_calendar->setMinimumDate(QDate(1950, 1, 1));
      m_calendar->setMaximumDate(QDate(2050, 1, 1));

      // Make the m_calendar wide and tall enough to see all dates.
      m_calendar->setGeometry(QRect(180, 0, 445, 210));

      // Improve UX, then display the m_calendar.
      m_calendar->setGridVisible(true);
      m_calendar->setHeaderVisible(false);
      m_calendar->setWindowTitle("Calendar picker");
      m_calendar->show();

      // Uses the previously clicked button (startDatePicker or endDatePicker) to determine which
      // text field that the opened m_calendar is coordinating with (e.g. the one we want to write date to).
      dateSelected(sender()->name());
    }

    /**
     * Update startDate text field when startDatePicker is used and date is selected.
     */
    void ICatSearch2::updateStartDate()
    {
      // Update the text field with the user selected date then close the m_calendar.
      m_icatUiForm.StartDate->setText(m_calendar->selectedDate().toString("dd/MM/yyyy"));
      m_calendar->close();
      // Re-enable the button to allow the user to select an endDate if they wish.
      m_icatUiForm.endDatePicker->setEnabled(true);
    }

    /**
     * Update endDate text field when endDatePicker is used and date is selected.
     */
    void ICatSearch2::updateEndDate()
    {
      m_icatUiForm.EndDate->setText(m_calendar->selectedDate().toString("dd/MM/yyyy"));
      m_calendar->close();
      m_icatUiForm.startDatePicker->setEnabled(true);
    }

    /**
     * Show or hide advanced options if "Advanced Search" checked.
     */
    void ICatSearch2::advancedSearchChecked()
    {
      if (m_icatUiForm.advSearchCbox->isChecked())
      {
        m_icatUiForm.advNameLbl->show();
        m_icatUiForm.InvestigatorSurname->show();
        m_icatUiForm.advAbstractLbl->show();
        m_icatUiForm.InvestigationAbstract->show();
        m_icatUiForm.advSampleLbl->show();
        m_icatUiForm.SampleName->show();
        m_icatUiForm.advTypeLbl->show();
        m_icatUiForm.InvestigationType->show();
      }
      else
      {
        m_icatUiForm.advNameLbl->hide();
        m_icatUiForm.InvestigatorSurname->hide();
        m_icatUiForm.advAbstractLbl->hide();
        m_icatUiForm.InvestigationAbstract->hide();
        m_icatUiForm.advSampleLbl->hide();
        m_icatUiForm.SampleName->hide();
        m_icatUiForm.advTypeLbl->hide();
        m_icatUiForm.InvestigationType->hide();
      }
    }

    /**
     * Hides the search frame, and shows search results frame when "Search" button pressed.
     */
    void ICatSearch2::searchClicked()
    {
      if (m_icatUiForm.searchBtn)
      {
        m_icatUiForm.searchResultsCbox->setEnabled(true);
        m_icatUiForm.searchResultsCbox->setChecked(true);
        m_icatUiForm.resFrame->show();
        m_icatUiForm.searchResultsLbl->setText("searching investigations...");

        // Remove previous search results.
        std::string searchResults = "searchResults";
        clearSearch(m_icatUiForm.searchResultsTbl, searchResults);

        // Perform the search using the values the user has input.
        m_icatHelper->executeSearch(getSearchFields());

        // Populate the result table from the searchResult workspace.
        populateResultTable();
      }
    }

    /**
     * Reset all fields when the "Reset" button is pressed.
     */
    void ICatSearch2::onReset()
    {
      // Clear the QLineEdit boxes.
      foreach(QLineEdit *widget, this->findChildren<QLineEdit*>())
      {
        widget->clear();
      }
      // Clear all other elements.
      m_icatUiForm.Instrument->clear();
      m_icatUiForm.InvestigationType->clear();
      m_icatUiForm.advSearchCbox->setChecked(false);
      m_icatUiForm.myDataCbox->setChecked(false);
    }


    ///////////////////////////////////////////////////////////////////////////////
    // Methods for "Search results"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Outputs the results of the search into the "Search results" table.
     */
    void ICatSearch2::populateResultTable()
    {
      // Obtain a pointer to the "searchResults" workspace where the search results are saved if it exists.
      Mantid::API::ITableWorkspace_sptr workspace;

      // Check to see if the workspace exists...
      if(Mantid::API::AnalysisDataService::Instance().doesExist("__searchResults"))
      {
        workspace = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("__searchResults"));
      }
      else
      {
        // Otherwise an error will be thrown (in ICat4Catalog). We will reproduce that error on the ICAT form for the user.
        m_icatUiForm.searchResultsLbl->setText("You have not input any terms to search for.");
        return;
      }

      // Create local variable for convenience and reusability.
      QTableWidget* resultsTable = m_icatUiForm.searchResultsTbl;

      // Set the result's table properties prior to adding data.
      setupTable(resultsTable, workspace->rowCount(), workspace->columnCount());

      // Update the label to inform the user of how many investigations have been returned from the search.
      m_icatUiForm.searchResultsLbl->setText(QString::number(workspace->rowCount()) + " Investigation found.");

      // Add data from the workspace to the results table.
      populateTable(resultsTable, workspace);

      // Make the headers of the table bold.
      emboldenTableHeaders(resultsTable);

      // Hide the "Investigation id" column (It's used by the CatalogGetDataFiles algorithm).
      resultsTable->setColumnHidden(0, true);
    }

    /**
     * Updates the "Displaying info" text box with relevant result info (e.g. 500 of 18,832)
     */
    void ICatSearch2::resultInfoUpdate()
    {

    }

    /**
     * Updates the page numbers (e.g. m & n in: Page m of n )
     */
    void ICatSearch2::pageNumberUpdate()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    // SLOTS for "Search results"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Populate the result table, and update the page number.
     */
    bool ICatSearch2::nextPageClicked()
    {

    }

    /**
     * Populate the result table, and update the page number.
     */
    bool ICatSearch2::prevPageClicked()
    {

    }

    /**
     * Populate's result table depending page number input by user.
     */
    bool ICatSearch2::goToInputPage()
    {

    }

    /**
     * Hides the "search results" frame, and shows the "dataFiles" frame when an investigation is selected.
     */
    void ICatSearch2::investigationSelected(QTableWidgetItem* item)
    {
      //
      m_icatUiForm.dataFileCbox->setEnabled(true);
      m_icatUiForm.dataFileCbox->setChecked(true);
      m_icatUiForm.dataFileFrame->show();
      // Have to clear the combo-box in order to prevent the user from seeing the extensions of previous search.
      m_icatUiForm.dataFileFilterCombo->clear();
      m_icatUiForm.dataFileFilterCombo->addItem("Filter type...");

      // Inform the user that the search is in progress.
      m_icatUiForm.dataFileLbl->setText("searching for related datafiles...");

      // Obtain the investigation id from the selected
      QTableWidgetItem* investigationId = m_icatUiForm.searchResultsTbl->item(item->row(),0);

      // Remove previous dataFile search results.
      std::string dataFileResults = "dataFileResults";
      clearSearch(m_icatUiForm.dataFileResultsTbl, dataFileResults);

      // Update the labels in the dataFiles information frame to show relevant info to use.
      updateDataFileLabels(item);

      // Perform the "search" and obtain the related data files for the selected investigation.
      m_icatHelper->executeGetDataFiles(investigationId->text().toLongLong());

      // Populate the dataFile table from the "dataFileResults" workspace.
      populateDataFileTable();
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Methods for "DataFile information"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Outputs related dataFiles (from selected investigation) into the "DataFile information" table.
     */
    void ICatSearch2::populateDataFileTable()
    {
      // Obtain a pointer to the "dataFileResults" workspace where the related datafiles for the user selected invesitgation exist.
      Mantid::API::ITableWorkspace_sptr workspace;

      // Check to see if the workspace exists...
      if(Mantid::API::AnalysisDataService::Instance().doesExist("__dataFileResults"))
      {
        workspace = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("__dataFileResults"));
      }
      else
      {
        return;
      }

      // Create local variable for convenience and reusability.
      QTableWidget* dataFileTable = m_icatUiForm.dataFileResultsTbl;

      // Set the result's table properties prior to adding data.
      setupTable(dataFileTable, workspace->rowCount(), workspace->columnCount());

      // Update the label to inform the user of how many dataFiles relating to the selected investigation have been found.
      m_icatUiForm.dataFileLbl->setText(QString::number(workspace->rowCount()) + " datafiles found.");

      // Add data from the workspace to the results table.
      populateTable(dataFileTable, workspace);

      // Make the headers of the table bold.
      emboldenTableHeaders(dataFileTable);

      // Obtain the list of extensions of all dataFiles for the chosen investigation.
      // "File name" is the first column of "dataFileResults" so we make use of it.
      std::set<std::string> extensions = getDataFileExtensions(workspace.get()->getColumn(0));

      // Populate the "Filter type..." combo-box with all possible file extensions.
      populateDataFileType(extensions);
    }

    /**
     * Obtains the names of the selected dataFiles, in preparation for download.
     */
    void ICatSearch2::getCheckedFileNames()
    {

    }

    /**
     * Updates the dataFile text boxes with relevant info regarding the selected dataFile.
     */
    void ICatSearch2::updateDataFileLabels(QTableWidgetItem* item)
    {
      // Set the "title" label using the data from the investigation results workspace.
      m_icatUiForm.dataFileTitleRes->setText(m_icatUiForm.searchResultsTbl->item(item->row(),1)->text());

      // Set the instrument label using data from the investigation results workspace.
      m_icatUiForm.dataFileInstrumentRes->setText(m_icatUiForm.searchResultsTbl->item(item->row(),2)->text());

      // Show the related "run-range" for the specific dataFiles.
      m_icatUiForm.dataFileRunRangeRes->setText(m_icatUiForm.searchResultsTbl->item(item->row(),3)->text());
    }

    /**
     * Obtain all file extensions from the provided column.
     * @param column :: The fileName column in the dataFile workspace.
     * @return A set containing all file extensions.
     */
    std::set<std::string> ICatSearch2::getDataFileExtensions(Mantid::API::Column_sptr column)
    {
      std::set<std::string> extensions;

      // For every filename in the column...
      for (unsigned row = 0; row < column->size(); row++)
      {
        // Add the file extension to the set if it does not exist.
        extensions.insert(Poco::Path(column->cell<std::string>(row)).getExtension());
      }

      return (extensions);
    }

    /**
     * Add the list of file extensions to the "Filter type..." drop-down.
     */
    void ICatSearch2::populateDataFileType(std::set<std::string> extensions)
    {
      for( std::set<std::string>::const_iterator iter = extensions.begin(); iter != extensions.end(); ++iter)
      {
        m_icatUiForm.dataFileFilterCombo->addItem(QString::fromStdString("." + *iter));
      }
    }

    /**
     * Filters the "DataFile information" table to display user specified files (based on file extension).
     */
    void ICatSearch2::filterDataFileType(int index)
    {
      QTableWidget* table = m_icatUiForm.dataFileResultsTbl;

      for (int row = 0; row < table->rowCount(); ++row)
      {
        // Hide row by default, in order to show only relevant ones.
        table->setRowHidden(row,true);
        // We only need to check the first column to compare file extensions as it's the file name.
        for (int col = 0; col < 1; ++col)
        {
          QTableWidgetItem *item = table->item(row,col);
          // Show the relevant rows depending on file extension.
          // 0 index is "Filter type..." so all will be shown.
          if (index == 0 || (item->text().contains(m_icatUiForm.dataFileFilterCombo->text(index))))
          {
            table->setRowHidden(row,false);
            break;
          }
        }
      }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // SLOTS for: "DataFile information"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Performs filter option for specified filer type.
     */
    void ICatSearch2::doFilter(int index)
    {
      // Filter by the user selected file extension.
      filterDataFileType(index);
    }

    /**
     * Downloads selected datFiles to a specified location.
     */
    void ICatSearch2::downloadDataFiles()
    {

    }

    /**
     * Loads the selected dataFiles into workspaces.
     */
    void ICatSearch2::loadDataFile()
    {

    }

  } // namespace MantidWidgets
} // namespace MantidQt
