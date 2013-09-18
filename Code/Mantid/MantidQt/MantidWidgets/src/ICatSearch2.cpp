#include "MantidQtMantidWidgets/ICatSearch2.h"

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
      icatUiForm.setupUi(this);

      // Hide the three frames
      icatUiForm.searchFrame->hide();
      icatUiForm.resFrame->hide();
      icatUiForm.dataFileFrame->hide();

      // Hide the "investigation found" label until user has searched.
      icatUiForm.searchResultsLbl->hide();

      // Hide advanced input fields until "Advanced search" is checked.
      advancedSearchChecked();

      // Disable buttons except search by default.
      icatUiForm.searchResultsCbox->setEnabled(false);
      icatUiForm.dataFileCbox->setEnabled(false);

      // Show "Search" frame when user clicks "Catalog search" check box.
      connect(icatUiForm.searchCbox,SIGNAL(clicked()),this,SLOT(showCatalogSearch()));
      // Show advanced search options if "Advanced search" is checked.
      connect(icatUiForm.advSearchCbox,SIGNAL(clicked()),this,SLOT(advancedSearchChecked()));
      // Clear all fields when reset button is pressed.
      connect(icatUiForm.resetBtn,SIGNAL(clicked()),this,SLOT(onReset()));
      // Show "Search results" frame when user tries to "Search".
      connect(icatUiForm.searchBtn,SIGNAL(clicked()),this,SLOT(searchClicked()));
      // Show "Search results" frame when user clicks related check box.
      connect(icatUiForm.searchResultsCbox,SIGNAL(clicked()),this,SLOT(showSearchResults()));

      // No need for error handling as that's dealt with in the algorithm being used.
      populateInstrumentBox();

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
    void ICatSearch2::onHelp()
    {

    }

    /**
     * Shows/hides the "Catalog search" frame when search combo box is checked.
     */
    void ICatSearch2::showCatalogSearch()
    {
      if (icatUiForm.searchCbox->isChecked())
      {
        icatUiForm.searchFrame->show();
      }
      else
      {
        icatUiForm.searchFrame->hide();
      }
    }

    /**
     * Hides the search frame, and shows search results frame when "Search" button pressed.
     */
    void ICatSearch2::searchClicked()
    {
      if (icatUiForm.searchBtn)
      {
        icatUiForm.resFrame->show();
        icatUiForm.searchResultsCbox->setEnabled(true);
        icatUiForm.searchResultsCbox->setChecked(true);
      }
    }

    /**
     * Shows/Hides the "Search results" frame when search results combo box is checked.
     */
    void ICatSearch2::showSearchResults()
    {
      if (icatUiForm.searchResultsCbox->isChecked())
      {
        icatUiForm.resFrame->show();
      }
      else
      {
        icatUiForm.resFrame->hide();
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
     * Populates the "Instrument" list-box
     */
    void ICatSearch2::populateInstrumentBox()
    {
      // Obtain the list of instruments to display in the drop-box.
      std::vector<std::string> instrumentList = icatHelper->getInstrumentList();

      std::vector<std::string>::const_iterator citr;
      for (citr = instrumentList.begin(); citr != instrumentList.end(); ++citr)
      {
        // Add each instrument to the instrument box.
        icatUiForm.instrumentLbox->addItem(QString::fromStdString(*citr));
      }

      // Sort the drop-box by instrument name.
      icatUiForm.instrumentLbox->model()->sort(0);
      // Make the default instrument empty so the user has to select one.
      icatUiForm.instrumentLbox->insertItem(-1,"");
      icatUiForm.instrumentLbox->setCurrentIndex(0);
    }

    /**
     * Populates the "Investigation type" drop-box.
     */
    void ICatSearch2::populateInvestigationTypeBox()
    {

    }

    /**
     * Perform the search.
     */
    bool ICatSearch2::executeSearch()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    /// SLOTS for "Catalog Search"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Opens the DateTime Calendar box when start or end date selected.
     */
    void ICatSearch2::openCalendar()
    {
    }

    /**
     * Search through the user's data only when "My data" is checked.
     */
    void ICatSearch2::onMyDataOnlyChecked()
    {

    }

    /**
     * Show or hide advanced options if "Advanced Search" checked.
     */
    void ICatSearch2::advancedSearchChecked()
    {
      if (icatUiForm.advSearchCbox->isChecked())
      {
        icatUiForm.advNameLbl->show();
        icatUiForm.advNameTxt->show();
        icatUiForm.advAbstractLbl->show();
        icatUiForm.advAbstractTxt->show();
        icatUiForm.advSampleLbl->show();
        icatUiForm.advSampleTxt->show();
        icatUiForm.advTypeLbl->show();
        icatUiForm.advTypeLbox->show();
      }
      else
      {
        icatUiForm.advNameLbl->hide();
        icatUiForm.advNameTxt->hide();
        icatUiForm.advAbstractLbl->hide();
        icatUiForm.advAbstractTxt->hide();
        icatUiForm.advSampleLbl->hide();
        icatUiForm.advSampleTxt->hide();
        icatUiForm.advTypeLbl->hide();
        icatUiForm.advTypeLbox->hide();
      }
    }

    /**
     * Perform the search operation when the "Search" button is clicked.
     */
    void ICatSearch2::onSearch()
    {

    }

    /**
     * Reset all fields when the "Reset" button is pressed.
     */
    void ICatSearch2::onReset()
    {
      // Clear normal search fields.
      icatUiForm.invesNameTxt->clear();
      icatUiForm.startDateTxt->clear();
      icatUiForm.instrumentLbox->clear();
      icatUiForm.endDateTxt->clear();
      icatUiForm.runRangeTxt->clear();
      icatUiForm.keywordsTxt->clear();
      // Clear advanced options as well.
      icatUiForm.advNameTxt->clear();
      icatUiForm.advAbstractTxt->clear();
      icatUiForm.advSampleTxt->clear();
      icatUiForm.advTypeLbox->clear();
    }


    ///////////////////////////////////////////////////////////////////////////////
    // Methods for "Search results"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Outputs the results of the search into the "Search results" table.
     */
    void ICatSearch2::populateResultTable()
    {

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
