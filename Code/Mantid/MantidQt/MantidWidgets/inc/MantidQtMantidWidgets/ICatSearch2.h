#ifndef MANTIDQTWIDGETS_ICATSEARCHTWO_H_
#define MANTIDQTWIDGETS_ICATSEARCHTWO_H_

#include "ui_ICatSearch2.h"
#include "WidgetDllOption.h"
#include "MantidQtMantidWidgets/ICatHelper.h"

#include <QCalendarWidget>

namespace MantidQt
{
  namespace MantidWidgets
  {

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ICatSearch2 : public QWidget
    {
      Q_OBJECT

    public:
      /// Default constructor
      ICatSearch2(QWidget *parent = 0);
      /// Destructor
      ~ICatSearch2();

    private:
      /// Initialise the layout
      virtual void initLayout();

      ///////////////////////////////////////////////////////////////////////////////
      /// Methods for: "Catalog Search"
      ///////////////////////////////////////////////////////////////////////////////

      /// Ensures the correct text box is updated when the date is selected on the calendar.
      void dateSelected(std::string buttonName);
      /// Populate the instrument list-box.
      void populateInstrumentBox();
      /// Populate the investigation type list-box.
      void populateInvestigationTypeBox();
      /// Obtain the users' text input for each search field.
      std::map<std::string, std::string> getSearchFields();

      ///////////////////////////////////////////////////////////////////////////////
      // Methods for: "Search results"
      ///////////////////////////////////////////////////////////////////////////////

      /// Make the headers in the results table bold.
      void emboldenResultHeaders();
      /// Setup results table prior to adding data to it, such hiding vertial header.
      void setupResultTable(size_t& numOfRows, size_t& numOfColumns);
      /// Outputs the results of the query into a table.
      void populateResultTable();
      /// When an investigation is double clicked we want to call populateDataFileTable using the investigation name.
      void investigationClicked();
      /// Updates the "Displaying info" text box with relevant result info (e.g. 500 of 18,832)
      void resultInfoUpdate();
      /// Updates the page numbers (e.g. m & n in: Page m of n )
      void pageNumberUpdate();

      ///////////////////////////////////////////////////////////////////////////////
      // Methods for: "Datafile information"
      ///////////////////////////////////////////////////////////////////////////////

      /// Populates the table from the results of investigationSelected();
      void populateDataFileTable();
      /// Obtain the names of the selected files. (Used in downloadDataFiles).
      void getCheckedFileNames();
      /// Updates the dataFile text boxes with relevant info about the selected dataFile.
      void updateDataFileLabel();
      /// Filter the table to show only results by data type user wants to view.
      void filterDataFileType();

    private slots:
      /// When the facility login button is clicked
      void onFacilityLogin();
      /// When the help button is clicked.
      void helpClicked();
      /// When checked, show the Catalog search frame.
      void showCatalogSearch();
      /// Shows/Hides the "Search results" frame when search results combo box is checked.
      void showSearchResults();
      /// When checked, show the data file info frame.
      void showDataFileInfo();

      ///////////////////////////////////////////////////////////////////////////////
      /// SLOTS for: "Catalog Search"
      ///////////////////////////////////////////////////////////////////////////////

      /// Open the DateTime Calender to select date.
      void openCalendar();
      /// Update startDate text field when startDatePicker is used and date is selected.
      void updateStartDate();
      /// Update endDate text field when endDatePicker is used and date is selected.
      void updateEndDate();
      /// Only search through their data when checked.
      void onMyDataOnlyChecked();
      /// Show the advanced field when checked.
      void advancedSearchChecked();
      /// Perform the "Search" operation when the search button is clicked.
      void onSearch();
      /// When the "Search" button is clicked, display "Search results" frame.
      void searchClicked();
      /// Reset all fields when "Reset" is clicked.
      void onReset();

      ///////////////////////////////////////////////////////////////////////////////
      // SLOTS for: "Search results"
      ///////////////////////////////////////////////////////////////////////////////

      /// Populate the result table, and update the page number.
      bool nextPageClicked();
      bool prevPageClicked();
      /// Populate's result table depending page number input by user.
      bool goToInputPage();
      /// Checks that the investigation is selected and performs investigationClicked.
      bool investigationSelected();

      ///////////////////////////////////////////////////////////////////////////////
      // SLOTS for: "Datafile information"
      ///////////////////////////////////////////////////////////////////////////////

      /// Performs filterDataFileType() for specified filer type.
      void doFilter();
      /// Downloads selected datFiles to a specified location.
      void downloadDataFiles();
      /// Loads the selected dataFiles into workspaces.
      void loadDataFile();

    private:
      /// The form generated by QT Designer
      Ui::ICatSearch2 m_icatUiForm;
      /// The calendar widget that will allow the user to select start and end date/times.
      QCalendarWidget * m_calendar;
      /// The helper class that accesses ICAT algorithmic functionality.
      ICatHelper * m_icatHelper;
    };
  }
}
#endif // MANTIDQTWIDGETS_ICATSEARCHTWO_H_
