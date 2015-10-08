#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISRESULTTABLETAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISRESULTTABLETAB_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"
#include <QTableWidget>

namespace Ui
{
  class MuonAnalysis;
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{


/** 
This is a Helper class for MuonAnalysis. In particular this helper class deals
callbacks from the Plot Options tab.    

@author Robert Whitley, ISIS, RAL

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

class MuonAnalysisResultTableTab : public QWidget
{
 Q_OBJECT
public:
  MuonAnalysisResultTableTab(Ui::MuonAnalysis& uiForm);

  // Refresh the label list and re-populate the tables
  void refresh();

signals:
  /// Emitted to run some (usually simple) Python code
  void runPythonCode(const QString& code, bool async);

private slots:
  void helpResultsClicked();
  void selectAllLogs(bool);
  void selectAllFittings(bool);

  /// Executed when "Create table" button is clicked
  void onCreateTableClicked();

  /// Clear and populate both tables
  void populateTables();

private:
  /// Postfix used by Fit for result workspaces
  static const std::string WORKSPACE_POSTFIX;

  /// Postfix used by Fit for tables with fitted parameters
  static const std::string PARAMS_POSTFIX;

  /// Names of the non-timeseries logs we should display
  static const QStringList NON_TIMESERIES_LOGS;

  /// LessThan function used to sort log names
  static bool logNameLessThan(const QString& logName1, const QString& logName2);

  /**
   * Retrieve the workspace, checking if it is of the expected type. If workspace with given and name
   * and type is not found in the ADS - NotFoundError is thrown.
   * @param wsName :: Name of the workspace to retrieve
   * @return Retrieved workspace
   */
  template <typename T>
  static boost::shared_ptr<T> retrieveWSChecked(const std::string& wsName)
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<T>(wsName);

    if ( ! ws )
      throw Mantid::Kernel::Exception::NotFoundError("Incorrect type", wsName);

    return ws;
  }

  /// Returns name of the fitted workspace with WORKSPACE_POSTFIX removed
  static std::string wsBaseName(const std::string& wsName);

  /// Does a few basic checks for whether the workspace is a fitted workspace
  static bool isFittedWs(const std::string& wsName);

  void storeUserSettings();
  void applyUserSettings();
  void populateLogsAndValues(const QStringList& fittedWsList);
  void populateFittings(const QStringList& fittedWsList);

  /// Creates the results table
  void createTable();

  /// Returns a list of workspaces which should be displayed in the table
  QStringList getFittedWorkspaces();

  /// Returns a list individually fitted workspaces names
  QStringList getIndividualFitWorkspaces();

  /// Returns a list of sequentially fitted workspaces names
  QStringList getSequentialFitWorkspaces(const QString& label);

  /// Returns a list of labels user has made sequential fits for
  QStringList getSequentialFitLabels();

  bool haveSameParameters(const QStringList& wsList);
  QStringList getSelectedWs();
  QStringList getSelectedLogs();
  std::string getFileName();
  QMap<int,int> getWorkspaceColors(const QStringList& wsList);
  
  Ui::MuonAnalysis& m_uiForm;
  int m_numLogsdisplayed;
  
  // Log values for all the fitted workspaces  
  QMap<QString, QMap<QString, QVariant> > m_logValues;
  
  // Saved states of log value check-boxes. Used to remember what user has chosen when
  // re-creating the table
  QMap<QString, Qt::CheckState> m_savedLogsState;

  QList<QString> m_unselectedFittings;

};

}
}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSISRESULTTABLETAB_H_
