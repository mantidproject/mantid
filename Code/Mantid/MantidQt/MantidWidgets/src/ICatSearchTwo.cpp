#include "MantidQtMantidWidgets/ICatSearchTwo.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
     * Constructor
     */
    ICatSearchTwo::ICatSearchTwo(QWidget* parent) : QWidget(parent)
    {
      initLayout();
    }

    /**
     * Destructor
     */
    ICatSearchTwo::~ICatSearchTwo(){}

    /**
     * Initialise the  default layout.
     */
    void ICatSearchTwo::initLayout()
    {

    }

    /**
     * Opens the login dialog to allow the user to log into another facility.
     */
    void ICatSearchTwo::onFacilityLogin()
    {

    }

    /**
     * Sends the user to relevant search page on the Mantid project site.
     */
    void ICatSearchTwo::onHelp()
    {

    }

    /**
     * Shows/hides the Catalog search frame when search combo box is checked.
     */
    void ICatSearchTwo::showCatalogSearch()
    {

    }

    /**
     * Hides the search frame, and shows search results frame when "Search" button pressed.
     */
    void ICatSearchTwo::showSearchResults()
    {

    }

    /**
     * Hides "Search results" frame when a result is double clicked.
     */
    void ICatSearchTwo::showDataFileInfo()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    /// Methods for "Catalog Search".
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Populates the "Instrument" drop-box
     */
    void ICatSearchTwo::populateInstrumentBox()
    {

    }

    /**
     * Populates the "Investigation type" drop-box.
     */
    void ICatSearchTwo::populateInvestigationTypeBox()
    {

    }

    /**
     * Perform the search.
     */
    bool ICatSearchTwo::executeSearch()
    {

    }

    /**
     * Hide the advanced options by default.
     */
    void ICatSearchTwo::hideAdvancedOptions()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    /// SLOTS for "Catalog Search"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Opens the DateTime Calender box when spe.
     */
    void ICatSearchTwo::openCalendar()
    {

    }

    /**
     * Search through the user's data only when "My data" is checked.
     */
    void ICatSearchTwo::onMyDataOnlyChecked()
    {

    }

    /**
     * Show or hide advanced options if "Advanced Search" checked.
     */
    void ICatSearchTwo::advancedSearchChecked()
    {

    }

    /**
     * Perform the search operation when the "Search" button is clicked.
     */
    void ICatSearchTwo::onSearch()
    {

    }

    /**
     * Reset all fields when the "Reset" button is pressed.
     */
    void ICatSearchTwo::onReset()
    {

    }


    ///////////////////////////////////////////////////////////////////////////////
    // Methods for "Search results"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Outputs the results of the search into the "Search results" table.
     */
    void ICatSearchTwo::populateResultTable()
    {

    }

    /**
     * When an investigation is double clicked open we want to call populateDataFileTable using the investigation name.
     */
    void ICatSearchTwo::investigationClicked()
    {

    }

    /**
     * Updates the "Displaying info" text box with relevant result info (e.g. 500 of 18,832)
     */
    void ICatSearchTwo::resultInfoUpdate()
    {

    }

    /**
     * Updates the page numbers (e.g. m & n in: Page m of n )
     */
    void ICatSearchTwo::pageNumberUpdate()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    // SLOTS for "Search results"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Populate the result table, and update the page number.
     */
    bool ICatSearchTwo::nextPageClicked()
    {

    }

    /**
     * Populate the result table, and update the page number.
     */
    bool ICatSearchTwo::prevPageClicked()
    {

    }

    /**
     * Populate's result table depending page number input by user.
     */
    bool ICatSearchTwo::goToInputPage()
    {

    }

    /**
     * Checks that the investigation is selected and performs investigationClicked.
     */
    bool ICatSearchTwo::investigationSelected()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    // Methods for "DataFile information"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Outputs related dataFiles (from selected investigation) into the "DataFile information" table.
     */
    void ICatSearchTwo::populateDataFileTable()
    {

    }

    /**
     * Obtains the names of the selected dataFiles, in preparation for download.
     */
    void ICatSearchTwo::getCheckedFileNames()
    {

    }

    /**
     * Updates the dataFile text boxes with relevant info regarding the selected dataFile.
     */
    void ICatSearchTwo::updateDataFileLabel()
    {

    }

    /**
     * Filters the "DataFile information" table to display user specified files (based on file extension).
     */
    void ICatSearchTwo::filterDataFileType()
    {

    }

    ///////////////////////////////////////////////////////////////////////////////
    // SLOTS for: "DataFile information"
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Performs filter option for specified filer type.
     */
    void ICatSearchTwo::doFilter()
    {

    }

    /**
     * Downloads selected datFiles to a specified location.
     */
    void ICatSearchTwo::downloadDataFiles()
    {

    }

    /**
     * Loads the selected dataFiles into workspaces.
     */
    void ICatSearchTwo::loadDataFile()
    {

    }

  }
}
