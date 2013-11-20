#ifndef MANTID_MANTIDWIDGETS_MUONSEQUENTIALFITDIALOG_H_
#define MANTID_MANTIDWIDGETS_MUONSEQUENTIALFITDIALOG_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"

#include "ui_MuonSequentialFitDialog.h"

#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"

#include <QDialog>

namespace MantidQt
{
namespace MantidWidgets
{

  /** MuonSequentialFitDialog : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport MuonSequentialFitDialog : public QDialog
  {

    Q_OBJECT

  public:
    MuonSequentialFitDialog(MuonFitPropertyBrowser* parent);
    virtual ~MuonSequentialFitDialog();

  private:
    enum ControlButtonType {
      Start,
      Stop
    };

    // -- FUNCTIONS -----------------------------------------------------------

    /// Check if all the input field are valid 
    bool isInputValid();

    /// Set the type of the control button
    void setControlButtonType(ControlButtonType type);

    /// Update enabled state off all the input widgets (except for control ones) 
    void setInputEnabled(bool enabled);

    /// Initialize diagnosis table 
    void initDiagnosisTable();

    // -- VARIABLES -----------------------------------------------------------

    /// UI form
    Ui::MuonSequentialFitDialog m_ui;

    /// Fit properties browser used to start the dialog
    MuonFitPropertyBrowser* m_fitPropBrowser;

    // -- STATIC MEMBERS ------------------------------------------------------

    /// Checks if specified name is valid as a name for label. 
    static std::string isValidLabel(const std::string& label);

    /// Instance used to print log messages
    static Mantid::Kernel::Logger& g_log;

  private slots:
    /// Updates visibility/tooltip of label error asterisk
    void updateLabelError(const QString& label);

    /// Enables/disables start button depending on wether we are allowed to start
    void updateControlButtonState();

    /// Start fitting process
    void startFit();

    /// Stop fitting process
    void stopFit();
  };


} // namespace MantidWidgets
} // namespace Mantid

#endif  /* MANTID_MANTIDWIDGETS_MUONSEQUENTIALFITDIALOG_H_ */
