#ifndef MANTID_MANTIDWIDGETS_MUONSEQUENTIALFITDIALOG_H_
#define MANTID_MANTIDWIDGETS_MUONSEQUENTIALFITDIALOG_H_

#include "MantidKernel/System.h"

#include "ui_MuonSequentialFitDialog.h"

#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"

#include <QDialog>

namespace MantidQt
{
namespace MantidWidgets
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  /** MuonSequentialFitDialog : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MuonSequentialFitDialog : public QDialog
  {

    Q_OBJECT

  public:
    MuonSequentialFitDialog(MuonFitPropertyBrowser* fitPropBrowser, Algorithm_sptr loadAlg);
    virtual ~MuonSequentialFitDialog();

    enum DialogState 
    {
      Preparing,
      Running,
      Stopped
    };

    // Prefix added to the names of the sequential fit result workspaces and groups
    static const std::string SEQUENTIAL_PREFIX;

  signals:
    void stateChanged(DialogState newState);

  private:

    // -- FUNCTIONS -----------------------------------------------------------

    /// Check if all the input field are valid 
    bool isInputValid();

    /// Set current dialog state
    void setState(DialogState newState);

    /// Initialize diagnosis table 
    void initDiagnosisTable();

    /// Add a new entry to the diagnosis table 
    void addDiagnosisEntry(const std::string& runTitle, double fitQuality,
        IFunction_sptr fittedFunction);

    /// Helper function to create new item for Diagnosis table
    QTableWidgetItem* createTableWidgetItem(const QString& text);
    
    // -- VARIABLES -----------------------------------------------------------

    /// UI form
    Ui::MuonSequentialFitDialog m_ui;

    /// Fit properties browser used to start the dialog
    MuonFitPropertyBrowser* m_fitPropBrowser;

    /// Current state of the dialog
    DialogState m_state;

    /// Whether user requested fitting to be stopped
    bool m_stopRequested;

    /// Algorithm the dialog should use for loading 
    Algorithm_sptr m_loadAlg;

    // -- STATIC MEMBERS ------------------------------------------------------

    /// Checks if specified name is valid as a name for label. 
    static std::string isValidLabel(const std::string& label);

    /// Returns displayable title for the given workspace
    static std::string getRunTitle(Workspace_const_sptr ws);

  private slots:
    /// Updates visibility/tooltip of label error asterisk
    void updateLabelError(const QString& label);

    /// Sets control button to be start/stop depending on new dialog state 
    void updateControlButtonType(DialogState newState);

    /// Update enabled state off all the input widgets depending on new dialog state
    void updateInputEnabled(DialogState newState); 

    /// Update control button enabled status depending on the new state. 
    void updateControlEnabled(DialogState newState); 

    /// Update cursor depending on the new state of the dialog.
    void updateCursor(DialogState newState); 

    /// Start fitting process
    void startFit();

    /// Stop fitting process
    void stopFit();

  };


} // namespace MantidWidgets
} // namespace Mantid

#endif  /* MANTID_MANTIDWIDGETS_MUONSEQUENTIALFITDIALOG_H_ */
