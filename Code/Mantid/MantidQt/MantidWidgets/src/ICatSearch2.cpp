#include "MantidQtMantidWidgets/ICatSearch2.h"
#include "MantidAPI/AnalysisDataService.h"

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
     * @return :: A map containing all users' search fields - (key => FieldName, value => FieldValue).
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
      m_calendar->setWindowTitle("m_calendar picker");
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
     * Perform the search operation when the "Search" button is clicked.
     */
    void ICatSearch2::onSearch()
    {

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
        // Hide the "Investigations found" label
        m_icatUiForm.searchResultsLbl->setText("searching investigations...");
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
      // Clear normal search fields.
      m_icatUiForm.InvestigationName->clear();
      m_icatUiForm.StartDate->clear();
      m_icatUiForm.Instrument->clear();
      m_icatUiForm.EndDate->clear();
      m_icatUiForm.runRangeTxt->clear();
      m_icatUiForm.Keywords->clear();
      // Clear advanced options as well.
      m_icatUiForm.InvestigatorSurname->clear();
      m_icatUiForm.InvestigationAbstract->clear();
      m_icatUiForm.SampleName->clear();
      m_icatUiForm.InvestigationType->clear();
    }


    ///////////////////////////////////////////////////////////////////////////////
    // Methods for "Search results"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Embolden the headers in the results table.
     */
    void ICatSearch2::emboldenResultHeaders()
    {
      QFont font;
      font.setBold(true);
      for (int i = 0; i < m_icatUiForm.searchResultsTbl->columnCount(); ++i)
      {
        // Embolden the headers of the table.
        m_icatUiForm.searchResultsTbl->horizontalHeaderItem(i)->setFont(font);
      }
    }

    /**
     * Set the table properties prior to adding data to it.
     * @param numOfRows    :: The number of rows in the workspace.
     * @param numOfColumns :: The number of columns in the workspace.
     */
    void ICatSearch2::setupResultTable(size_t& numOfRows, size_t& numOfColumns)
    {
      m_icatUiForm.searchResultsTbl->setRowCount(static_cast<int>(numOfRows));
      m_icatUiForm.searchResultsTbl->setColumnCount(static_cast<int>(numOfColumns));

      // Improve the appearance of table to make it easier to read.
      m_icatUiForm.searchResultsTbl->setAlternatingRowColors(true);
      m_icatUiForm.searchResultsTbl->setStyleSheet("alternate-background-color: rgb(216, 225, 255)");
      m_icatUiForm.searchResultsTbl->setSortingEnabled(false);
      m_icatUiForm.searchResultsTbl->verticalHeader()->setVisible(false);

      // Update the label to inform the user of how many investigations have been returned from the search.
      m_icatUiForm.searchResultsLbl->setText(QString::number(numOfRows) + " Investigation found.");

      // Sort by endDate with the most recent being first.
      m_icatUiForm.searchResultsTbl->sortByColumn(4,Qt::DescendingOrder);
      m_icatUiForm.searchResultsTbl->setSortingEnabled(true);

      // Set the height on each row to 20 for UX improvement.
      for (size_t i = 0; i < numOfRows; ++i)
      {
        m_icatUiForm.searchResultsTbl->setRowHeight(static_cast<int>(i),20);
      }
    }

    /**
     * Outputs the results of the search into the "Search results" table.
     */
    void ICatSearch2::populateResultTable()
    {
      // Obtain a pointer to the "searchResults" workspace where the search results are saved if it exists.
      Mantid::API::ITableWorkspace_sptr workspace;
      // Check to see if the workspace exists...
      if(Mantid::API::AnalysisDataService::Instance().doesExist("searchResults"))
      {
        // If it does, then let's use it!
        workspace = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("searchResults"));
      }
      else
      {
        // Otherwise an error will be thrown (in ICat4Catalog). We will reproduce that error on the ICAT form for the user.
        m_icatUiForm.searchResultsLbl->setText("You have not input any terms to search for.");
        return;
      }

      size_t numOfRows = workspace->rowCount();
      size_t numOfColumns = workspace->columnCount();

      // Set the result's table properties prior to adding data.
      setupResultTable(numOfRows, numOfColumns);

      // This will contain the list of column names.
      QStringList columnHeaders;

      // Add the data from the workspace to the search results table.
      for(size_t col = 0 ; col < numOfColumns; col++)
      {
        // Get the column name to display as the header of table widget
        Mantid::API::Column_sptr column = workspace->getColumn(col);
        columnHeaders.push_back(QString::fromStdString(column->name()));

        for(size_t row = 0; row < numOfRows; ++row)
        {
          // Prints the value from the row to the ostringstream to use later.
          std::ostringstream ostr;
          column->print(row,ostr);

          // Add a result to the table.
          QTableWidgetItem *newItem  = new QTableWidgetItem(QString::fromStdString(ostr.str()));
          m_icatUiForm.searchResultsTbl->setItem(static_cast<int>(row),static_cast<int>(col), newItem);

          // Allow the row to be selected, and enabled.
          newItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
          newItem->setToolTip(QString::fromStdString(ostr.str()));
        }
      }
      // Hide the "Investigation id" column (It's used by the CatalogGetDataFiles algorithm).
      m_icatUiForm.searchResultsTbl->setColumnHidden(0, true);

      // Set the table widgets header labels from the table workspace.
      m_icatUiForm.searchResultsTbl->setHorizontalHeaderLabels(columnHeaders);

      // Embolden the headers of the table.
      emboldenResultHeaders();

      // Resize the label to improve the viewing experience.
      m_icatUiForm.searchResultsTbl->resizeColumnsToContents();
    }

    /**
     * When an investigation is double clicked open we want to call populateDataFileTable using the investigation name.
     */
    void ICatSearch2::investigationClicked()
    {

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
     * Checks that the investigation is selected and performs investigationClicked.
     */
    bool ICatSearch2::investigationSelected()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    // Methods for "DataFile information"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Outputs related dataFiles (from selected investigation) into the "DataFile information" table.
     */
    void ICatSearch2::populateDataFileTable()
    {

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
    void ICatSearch2::updateDataFileLabel()
    {

    }

    /**
     * Filters the "DataFile information" table to display user specified files (based on file extension).
     */
    void ICatSearch2::filterDataFileType()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    // SLOTS for: "DataFile information"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Performs filter option for specified filer type.
     */
    void ICatSearch2::doFilter()
    {

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

  }
}
