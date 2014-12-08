#ifndef MANTIDQTWIDGETS_CATALOGSEARCH_H_
#define MANTIDQTWIDGETS_CATALOGSEARCH_H_

#include "ui_CatalogSearch.h"
#include "WidgetDllOption.h"
#include "MantidQtMantidWidgets/CatalogHelper.h"
#include "MantidQtMantidWidgets/CatalogSelector.h"
#include "MantidQtMantidWidgets/CheckboxHeader.h"

#include <QCalendarWidget>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
      This class defines the interface and functionality for the cataloging system within Mantid.

      @author Jay Rainey
      @date 08/10/2013

      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

      This file is part of Mantid.

      Mantid is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3 of the License, or
      (at your option) any later version.

      Mantid is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.

      File change history is stored at: <https://github.com/mantidproject/mantid>
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS CatalogSearch : public QWidget
    {
      Q_OBJECT

    public:
      /// Default constructor
      CatalogSearch(QWidget *parent = 0);
      /// Destructor
      ~CatalogSearch();

    private:
      /// Initialise the layout
      virtual void initLayout();
      /// Make the headers in the provided table bold.
      void emboldenTableHeaders(QTableWidget* table);
      /// Setup table prior to adding data to it, such hiding vertical header.
      void setupTable(QTableWidget* table, const size_t &numOfRows, const size_t &numOfColumns);
      /// Populate the provided table with data from the provided workspace.
      void populateTable(QTableWidget* table, const Mantid::API::ITableWorkspace_sptr &workspace);
      /// Removes data associated with previous search.
      void clearSearch(QTableWidget* table, const std::string &workspace);
      ///  Clear the "search" frame when an investigation has been selected.
      void clearSearchFrame();
      /// Clear the "search results" frame if no results are returned from search.
      void clearSearchResultFrame();
      /// Clear "dataFileFrame" when the user tries to search again.
      void clearDataFileFrame();
      /// Obtain the index of the column in a table that contains a specified name.
      int headerIndexByName(QTableWidget* table, const std::string &searchFor);
      /// Save the current state of ICAT for next time.
      void saveSettings();
      /// Read settings from store.
      void loadSettings();

      ///////////////////////////////////////////////////////////////////////////////
      // Methods for: "Catalog Search"
      ///////////////////////////////////////////////////////////////////////////////

      void dateSelected(const std::string &buttonName);
      /// Populate the instrument list-box.
      void populateInstrumentBox();
      /// Populate the investigation type list-box.
      void populateInvestigationTypeBox();
      /// Obtain the users' text input for each search field.
      const std::map<std::string, std::string> getSearchFields();
      /// Checks if start date is greater than end date.
      bool validateDates();

      ///////////////////////////////////////////////////////////////////////////////
      // Methods for: "Search results"
      ///////////////////////////////////////////////////////////////////////////////

      /// Outputs the results of the query into a table.
      void populateResultTable();
      /// Obtain the sessionID for the selected investigation.
      std::string selectedInvestigationSession();

      ///////////////////////////////////////////////////////////////////////////////
      // Methods for: "Datafile information"
      ///////////////////////////////////////////////////////////////////////////////

      /// Populates the table from the results of investigationSelected();
      void populateDataFileTable();
      /// Add a row of checkboxes to the first column of a table.
      void addCheckBoxColumn(QTableWidget* table);
      /// Obtain the file details (file ID and name) for the file to download. (Used in downloadDataFiles).
      const std::vector<std::pair<int64_t, std::string>> selectedDataFileNames();
      /// Updates the dataFile text boxes with relevant info about the selected dataFile.
      void updateDataFileLabels(QTableWidgetItem* item);
      /// Obtain all file extensions from the provided column (dataFileResults -> File name).
      std::set<std::string> getDataFileExtensions(Mantid::API::Column_sptr column);
      /// Add the list of file extensions to the "Filter type..." drop-down.
      void populateDataFileType(const std::set<std::string> &extensions);
      /// Disable the download button if user can access the files locally from the archives.
      void disableDownloadButtonIfArchives(int row);

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
      // SLOTS for: "Catalog Search"
      ///////////////////////////////////////////////////////////////////////////////

      /// Open the DateTime Calendar to select date.
      void openCalendar();
      /// Update text field when date is selected.
      void dateSelected(QDate date);
      /// Show the advanced field when checked.
      void advancedSearchChecked();
      /// When the "Search" button is clicked, display "Search results" frame.
      void searchClicked();
      /// Show the error message labels, including the error message on the tooltips.
      void showErrorLabels(std::map<std::string, std::string> &errors);
      /// Hide the error message labels.
      void hideErrorLabels();
      /// Reset all fields when "Reset" is clicked.
      void onReset();
      /// Enables user to select specific facilities that they want to search the catalogs of.
      void openFacilitySelection();

      ///////////////////////////////////////////////////////////////////////////////
      // SLOTS for: "Search results"
      ///////////////////////////////////////////////////////////////////////////////

      /// Populate the result table, and update the page number.
      void nextPageClicked();
      void prevPageClicked();
      /// Populate's result table depending page number input by user.
      void goToInputPage();
      /// Checks that the investigation is selected and performs investigationClicked.
      void investigationSelected(QTableWidgetItem* item);

      ///////////////////////////////////////////////////////////////////////////////
      // SLOTS for: "Datafile information"
      ///////////////////////////////////////////////////////////////////////////////

      /// Disable load/download buttons if no datafile is selected.
      void disableDatafileButtons();
      /// Performs filterDataFileType() for specified filer type.
      void doFilter(const int &index);
      /// Downloads selected datFiles to a specified location.
      void downloadDataFiles();
      /// Loads the selected dataFiles into workspaces.
      void loadDataFiles();
      /// Selects/deselects ALL rows in dataFile table.
      void selectAllDataFiles(const bool &toggled);
      /// Select/Deselect row when a checkbox is selected
      void dataFileCheckboxSelected(QTableWidgetItem* item);
      /// Select/Deselect row & check-box when a row is selected.
      void dataFileRowSelected();
      /// Sort table by file size when certain column is clicked (E.g. "File size" row).
      void sortByFileSize(int column);

    private:
      /// The name of the date button the user pressed to open the calendar.
      std::string m_dateButtonName;
      /// The custom table header with checkbox functionality.
      CheckboxHeader * m_customHeader;
      /// The form generated by QT Designer
      Ui::CatalogSearch m_icatUiForm;
      /// The calendar widget that will allow the user to select start and end date/times.
      QCalendarWidget * m_calendar;
      /// The helper class that accesses ICAT algorithmic functionality.
      CatalogHelper * m_icatHelper;
      /// Access methods of catalog selector GUI, e.g. selected facilities.
      CatalogSelector * m_catalogSelector;
      /// The directory to save the downloaded dataFiles.
      QString m_downloadSaveDir;
      /// The current page the user is on in the results window. Used for paging.
      int m_currentPageNumber;
    };
  }
}
#endif // MANTIDQTWIDGETS_CATALOGSEARCH_H_
