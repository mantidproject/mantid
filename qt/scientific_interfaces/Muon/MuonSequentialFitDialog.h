// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_MuonSequentialFitDialog.h"

#include "DllConfig.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MuonAnalysisHelper.h"

#include <QDialog>

namespace MantidQt {
namespace CustomInterfaces {

class MuonAnalysisFitDataPresenter;

/** MuonSequentialFitDialog : Dialog for running sequential fits for Muon data
 */
class MANTIDQT_MUONINTERFACE_DLL MuonSequentialFitDialog : public QDialog {

  Q_OBJECT

public:
  MuonSequentialFitDialog(MantidQt::MantidWidgets::MuonFitPropertyBrowser *fitPropBrowser,
                          MuonAnalysisFitDataPresenter *dataPresenter);
  ~MuonSequentialFitDialog() override;

  enum DialogState { Preparing, Running, Stopped };

  // Prefix added to the names of the sequential fit result workspaces and
  // groups
  static const std::string SEQUENTIAL_PREFIX;

signals:
  void stateChanged(DialogState newState);

private:
  // -- FUNCTIONS -----------------------------------------------------------

  /// Check if all the input fields are valid
  bool isInputValid();

  /// Set current dialog state
  void setState(DialogState newState);

  /// Initialize diagnosis table
  void initDiagnosisTable();

  /// Add a new entry to the diagnosis table
  void addDiagnosisEntry(const std::string &runTitle, double fitQuality,
                         const Mantid::API::IFunction_sptr &fittedFunction);

  /// Helper function to create new item for Diagnosis table
  QTableWidgetItem *createTableWidgetItem(const QString &text);

  /// Reorganise workspaces after fit of one run finished
  void finishAfterRun(const std::string &labelGroupName, const Mantid::API::IAlgorithm_sptr &fitAlg, bool simultaneous,
                      const Mantid::API::MatrixWorkspace_sptr &firstWS) const;

  // -- MEMBER VARIABLES -----------------------------------------------

  /// UI form
  Ui::MuonSequentialFitDialog m_ui;

  /// Fit properties browser used to start the dialog
  MantidQt::MantidWidgets::MuonFitPropertyBrowser *m_fitPropBrowser;

  /// Current state of the dialog
  DialogState m_state;

  /// Whether user requested fitting to be stopped
  bool m_stopRequested;

  /// Fit data presenter passed in to constructor
  MuonAnalysisFitDataPresenter *m_dataPresenter;

  // -- STATIC MEMBERS ------------------------------------------------------

  /// Checks if specified name is valid as a name for label.
  static std::string isValidLabel(const std::string &label);

  /// Returns displayable title for the given workspace
  static std::string getRunTitle(const Mantid::API::Workspace_const_sptr &ws);

  // -- SLOTS ------------------------------------------------------

private slots:
  /// Updates visibility/tooltip of label error asterisk
  void updateLabelError(const QString &label);

  /// Sets control button to be start/stop depending on new dialog state
  void updateControlButtonType(DialogState newState);

  /// Update enabled state off all the input widgets depending on new dialog
  /// state
  void updateInputEnabled(DialogState newState);

  /// Update control button enabled status depending on the new state.
  void updateControlEnabled(DialogState newState);

  /// Update cursor depending on the new state of the dialog.
  void updateCursor(DialogState newState);

  /// Start fitting process
  void startFit();

  /// Stop fitting process
  void stopFit();

  /// Run fit after getting file input
  void continueFit();
};

} // namespace CustomInterfaces
} // namespace MantidQt
