// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSISRESULTTABLETAB_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSISRESULTTABLETAB_H_

//----------------------
// Includes
//----------------------
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "ui_MuonAnalysis.h"
#include <QTableWidget>
#include <functional>

namespace Ui {
class MuonAnalysis;
}

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {
using WSParameterList = QMap<QString, QMap<QString, double>>;

/**
This is a Helper class for MuonAnalysis. In particular this helper class deals
callbacks from the Plot Options tab.

@author Robert Whitley, ISIS, RAL
*/

class MuonAnalysisResultTableTab : public QWidget {
  Q_OBJECT
public:
  explicit MuonAnalysisResultTableTab(Ui::MuonAnalysis &uiForm);

  // Refresh the label list and re-populate the tables
  void refresh();

signals:
  /// Emitted to run some (usually simple) Python code
  void runPythonCode(const QString &code, bool async);

private slots:
  void helpResultsClicked();
  void selectAllLogs(bool /*state*/);
  void selectAllFittings(bool /*state*/);

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

  /// "run_number", "run_start", "run_end" log strings
  static const QString RUN_NUMBER_LOG, RUN_START_LOG, RUN_END_LOG;

  /// LessThan function used to sort log names
  static bool logNameLessThan(const QString &logName1, const QString &logName2);

  /**
   * Retrieve the workspace, checking if it is of the expected type. If
   * workspace with given and name
   * and type is not found in the ADS - NotFoundError is thrown.
   * @param wsName :: Name of the workspace to retrieve
   * @return Retrieved workspace
   */
  template <typename T>
  static boost::shared_ptr<T> retrieveWSChecked(const std::string &wsName) {
    auto ws =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<T>(wsName);

    if (!ws)
      throw Mantid::Kernel::Exception::NotFoundError("Incorrect type", wsName);

    return ws;
  }

  /// Returns name of the fitted workspace with WORKSPACE_POSTFIX removed
  static std::string wsBaseName(const std::string &wsName);

  /// Does a few basic checks for whether the workspace is a fitted workspace
  static bool isFittedWs(const std::string &wsName);

  void storeUserSettings();
  void applyUserSettings();
  void populateLogsAndValues(const QStringList &fittedWsList);
  void populateFittings(
      const QStringList &names,
      std::function<Mantid::API::Workspace_sptr(const QString &)> wsFromName);

  /// Creates the results table
  void createTable(bool multipleFits);

  /// Returns a list of workspaces which should be displayed in the table
  QStringList getFittedWorkspaces();

  /// Returns a list individually fitted workspaces names
  QStringList getIndividualFitWorkspaces();

  /// Returns a list of sequentially/simultaneously fitted workspaces names
  QStringList getMultipleFitWorkspaces(const QString &label, bool sequential);

  /// Returns a list of labels user has made sequential/simultaneous fits for
  std::pair<QStringList, QStringList> getFitLabels();

  QStringList getSelectedItemsToFit();
  QStringList getSelectedLogs();
  std::string getFileName();

  Ui::MuonAnalysis &m_uiForm;

  // Log values for all the fitted workspaces
  QMap<QString, QMap<QString, QVariant>> m_logValues;

  // Saved states of log value check-boxes. Used to remember what user has
  // chosen when
  // re-creating the table
  QMap<QString, Qt::CheckState> m_savedLogsState;

  QList<QString> m_unselectedFittings;
};
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_MUONANALYSISRESULTTABLETAB_H_
