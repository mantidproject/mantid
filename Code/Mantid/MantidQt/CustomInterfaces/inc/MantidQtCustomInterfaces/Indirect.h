#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECT_H_

#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectDataReductionTab.h"
#include "ui_IndirectDataReduction.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "MantidQtMantidWidgets/RangeSelector.h"
#include "MantidAPI/MatrixWorkspace.h"

//----------------------------------------------------
// Forward declarations
//-----------------------------------------------------

class QtProperty;
class QtBoolPropertyManager;
class QtDoublePropertyManager;
class QtGroupPropertyManager;
class QtTreePropertyBrowser;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Forward Declarations
    class Background;

    /** 
    This class defines handles the ConvertToEnergy interface for indirect instruments (IRIS/OSIRIS).    

    @author Michael Whitty
    @author Martyn Gigg

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    class Indirect : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// explicit constructor, not to allow any implicit conversion of types
      explicit Indirect(QWidget *parent, Ui::IndirectDataReduction & uiForm);
      /// Initialize the layout
      virtual void initLayout();
      /// run Python-based initialisation commands
      virtual void initLocalPython();
      /// open the wiki page for this interface in a web browser
      void helpClicked();
      /// perform whatever operations needed for analysis
      void runClicked();
      /// gather necessary information from Instument Definition Files
      virtual void setIDFValues(const QString & prefix);
      /// perform any instrument-specific changes to layout
      void performInstSpecific();

    private:
      virtual void closeEvent(QCloseEvent* close);
      void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf); ///< handle POCO event
      bool validateInput(); ///< validate input of "Energy Transfer" tab
      QString validateCalib(); ///< validate input of "Calibration" tab
      bool validateSofQw(); ///< validate input of "S(Q, w)" tab
      QString validateSlice(); ///< validate input of "Slice" tab
      void loadSettings();
      void saveSettings();

      void setupCalibration(); ///< set up the miniplots on calibration tab
      void setupSlice(); ///< setup the slice miniplot section

    private slots:
      void pbRunEditing();  //< Called when a user starts to type / edit the runs to load.
      void pbRunFinding();  //< Called when the FileFinder starts finding the files.
      void pbRunFinished(); //< Called when the FileFinder has finished finding the files.
      /// Slot showing a message box to the user
      void showMessageBox(const QString& message);

      void useCalib(bool state); ///< whether to use calib file
      void calibFileChanged(const QString & calib); ///< sets m_uiForm.ckUseCalib to appropriate value
      void intensityScaleMultiplierCheck(bool state); /// Toggle the intensity scale multiplier box
      void calibValidateIntensity(const QString & text); /// Check that the scale multiplier is valid

    private:
      /// set and show an instrument-specific widget
      void setInstSpecificWidget(const std::string & parameterName, QCheckBox * checkBox, QCheckBox::ToggleState defaultState);

      Ui::IndirectDataReduction m_uiForm; ///< user interface form object
      Poco::NObserver<Indirect, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver; ///< Poco observer for changes in user directory settings
      QString m_dataDir; ///< default data search directory
      QString m_saveDir; ///< default data save directory
      QString m_settingsGroup;

      //All indirect tabs
      IndirectDataReductionTab* m_tab_convert_to_energy;
      IndirectDataReductionTab* m_tab_sqw;
      IndirectDataReductionTab* m_tab_diagnostics;
      IndirectDataReductionTab* m_tab_calibration;
      IndirectDataReductionTab* m_tab_trans;
      IndirectDataReductionTab* m_tab_moments;
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
