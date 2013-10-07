#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECT_H_

#include "MantidQtAPI/UserSubWindow.h"
#include "ui_ConvertToEnergy.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include "MantidQtMantidWidgets/RangeSelector.h"

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
      explicit Indirect(QWidget *parent, Ui::ConvertToEnergy & uiForm);
      /// Initialize the layout
      virtual void initLayout();
      /// run Python-based initialisation commands
      virtual void initLocalPython();
      /// open the wiki page for this interface in a web browser
      void helpClicked();
      /// perform whatever operations needed for analysis
      void runClicked();
      void runConvertToEnergy();
      /// gather necessary information from Instument Definition Files
      virtual void setIDFValues(const QString & prefix);
      /// perform any instrument-specific changes to layout
      void performInstSpecific();

    private:
      virtual void closeEvent(QCloseEvent* close);
      void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf); ///< handle POCO event
      void clearReflectionInfo(); ///< clear various line edit boxes
      QString createMapFile(const QString& groupType); ///< create the mapping file with which to group results
      QString savePyCode(); ///< create python code as string to save files
      void createRESfile(const QString& file); ///< create a RES file for use in Fury
      bool validateInput(); ///< validate input of "Energy Transfer" tab
      QString validateCalib(); ///< validate input of "Calibration" tab
      bool validateSofQw(); ///< validate input of "S(Q, w)" tab
      QString validateSlice(); ///< validate input of "Slice" tab
      void loadSettings();
      void saveSettings();

      void setupCalibration(); ///< set up the miniplots on calibration tab
      void setupSlice(); ///< setup the slice miniplot section

    private slots:
      void validateSofQ(int);
      void pbRunEditing();  //< Called when a user starts to type / edit the runs to load.
      void pbRunFinding();  //< Called when the FileFinder starts finding the files.
      void pbRunFinished(); //< Called when the FileFinder has finished finding the files.

      void analyserSelected(int index); ///< set up cbReflection based on Analyser selection
      void reflectionSelected(int index); ///< set up parameter file values based on reflection
      void mappingOptionSelected(const QString& groupType); ///< change ui to display appropriate options
      void tabChanged(int index); ///< handles enabling/disabling the "Run" button
      void backgroundClicked(); ///< handles showing and hiding m_backgroundDialog
      void backgroundRemoval(); ///< handles data from BG
      void plotRaw(); ///< plot raw data from instrument
      void rebinCheck(bool state); ///< handle checking/unchecking of "Do Not Rebin"
      void detailedBalanceCheck(bool state); ///< handle checking/unchecking of "Detailed Balance"

      void scaleMultiplierCheck(bool state); ///< handle checking/unchecking of "Scale: Multiply by"

      void resCheck(bool state); ///< handles checking/unchecking of "Create RES File" checkbox
      void useCalib(bool state); ///< whether to use calib file
      void calibCreate(); ///< create calibration file
      void calibFileChanged(const QString & calib); ///< sets m_uiForm.ckUseCalib to appropriate value

      void calPlotRaw();
      void calPlotEnergy();
      void calMinChanged(double);
      void calMaxChanged(double);
      void calUpdateRS(QtProperty*, double);

      void sOfQwClicked(); ///< S(Q,w) tab run button clicked
      void sOfQwRebinE(bool state);
      void sOfQwPlotInput();

      void sliceRun();
      void slicePlotRaw();
      void sliceTwoRanges(QtProperty*, bool);
      void sliceCalib(bool state);
      void sliceMinChanged(double val);
      void sliceMaxChanged(double val);
      void sliceUpdateRS(QtProperty*, double);

    private:
      /// set and show an instrument-specific widget
      void setInstSpecificWidget(const std::string & parameterName, QCheckBox * checkBox, QCheckBox::ToggleState defaultState);

      Ui::ConvertToEnergy m_uiForm; ///< user interface form object
      Background *m_backgroundDialog; ///< background removal dialog
      Poco::NObserver<Indirect, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver; ///< Poco observer for changes in user directory settings
      QString m_dataDir; ///< default data search directory
      QString m_saveDir; ///< default data save directory
      QString m_settingsGroup;
      bool m_bgRemoval; ///< whether user has set values for BG removal

      /* Validators */
      QIntValidator *m_valInt; ///< validator for int inputs
      QDoubleValidator *m_valDbl; ///< validator for double inputs
      QDoubleValidator *m_valPosDbl; ///< validator for positive double inputs

      // CALIBRATION MINIPLOTS (prefix: 'm_calCal' (calibration) and 'm_calRes' (resolution))
      QwtPlot* m_calCalPlot;
      QwtPlot* m_calResPlot;
      MantidWidgets::RangeSelector* m_calCalR1;
      MantidWidgets::RangeSelector* m_calCalR2;
      MantidWidgets::RangeSelector* m_calResR1;
      MantidWidgets::RangeSelector* m_calResR2;
      QwtPlotCurve* m_calCalCurve;
      QwtPlotCurve* m_calResCurve;
      QtTreePropertyBrowser* m_calCalTree;
      QtTreePropertyBrowser* m_calResTree;
      QMap<QString, QtProperty*> m_calCalProp;
      QMap<QString, QtProperty*> m_calResProp;
      QtDoublePropertyManager* m_calDblMng;
      QtGroupPropertyManager* m_calGrpMng;

      // SLICE MINIPLOT (prefix: 'm_slt')
      QwtPlot* m_sltPlot;
      MantidWidgets::RangeSelector* m_sltR1;
      MantidWidgets::RangeSelector* m_sltR2;
      QwtPlotCurve* m_sltDataCurve;
      QtTreePropertyBrowser* m_sltTree;
      QMap<QString, QtProperty*> m_sltProp;
      QtDoublePropertyManager* m_sltDblMng;
      QtBoolPropertyManager* m_sltBlnMng;
      QtGroupPropertyManager* m_sltGrpMng;
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECT_H_
