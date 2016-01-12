#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEWQTGUI_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEWQTGUI_H_

#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionPresenter.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionView.h"

#include "ui_EnggDiffractionQtGUI.h"
#include "ui_EnggDiffractionQtTabCalib.h"
#include "ui_EnggDiffractionQtTabFocus.h"
#include "ui_EnggDiffractionQtTabPreproc.h"
#include "ui_EnggDiffractionQtTabSettings.h"

#include <boost/scoped_ptr.hpp>

// Qt classes forward declarations
class QMutex;

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt-based view of the Engineering Diffraction (EnggDiffraction)
GUI. Provides a concrete view for the graphical interface for Engg
functionality in Mantid. This view is Qt-based and it is probably the
only one that will be implemented in a foreseeable horizon. The
interface of this class is given by IEnggDiffractionView so that it
fits in the MVP (Model-View-Presenter) design of this GUI.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class MANTIDQT_CUSTOMINTERFACES_DLL EnggDiffractionViewQtGUI
    : public MantidQt::API::UserSubWindow,
      public IEnggDiffractionView {
  Q_OBJECT

public:
  /// Default Constructor
  EnggDiffractionViewQtGUI(QWidget *parent = 0);
  /// Destructor
  virtual ~EnggDiffractionViewQtGUI();

  /// Interface name
  static std::string name() { return "Engineering Diffraction"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Diffraction"; }

  void userWarning(const std::string &warn, const std::string &description);

  void userError(const std::string &err, const std::string &description);

  std::string askNewCalibrationFilename(const std::string &suggestedFname);

  std::string askExistingCalibFilename();

  std::vector<std::string> logMsgs() const { return m_logMsgs; }

  std::string getRBNumber() const;

  EnggDiffCalibSettings currentCalibSettings() const { return m_calibSettings; }

  std::string currentInstrument() const { return m_currentInst; }

  std::string currentVanadiumNo() const;

  std::string currentCeriaNo() const;

  std::string currentCalibFile() const;

  std::vector<std::string> newVanadiumNo() const;

  std::vector<std::string> newCeriaNo() const;

  std::string outCalibFilename() const { return m_outCalibFilename; }

  int currentCropCalibBankName() const { return m_currentCropCalibBankName; }

  std::string currentCalibSpecNos() const;

  void newCalibLoaded(const std::string &vanadiumNo, const std::string &ceriaNo,
                      const std::string &fname);

  void writeOutCalibFile(const std::string &outFilename,
                         const std::vector<double> &difc,
                         const std::vector<double> &tzero);

  virtual void enableTabs(bool enable);

  virtual void enableCalibrateAndFocusActions(bool enable);

  virtual std::string focusingDir() const;

  virtual std::vector<std::string> focusingRunNo() const;

  virtual std::vector<std::string> focusingCroppedRunNo() const;

  virtual std::vector<std::string> focusingTextureRunNo() const;

  virtual std::vector<bool> focusingBanks() const;

  virtual std::string focusingCroppedSpectrumIDs() const;

  virtual std::string focusingTextureGroupingFile() const;

  virtual bool focusedOutWorkspace() const;

  virtual void resetFocus();

  virtual std::string currentPreprocRunNo() const;

  virtual double rebinningTimeBin() const;

  virtual size_t rebinningPulsesNumberPeriods() const;

  virtual double rebinningPulsesTime() const;

  virtual void plotFocusedSpectrum(const std::string &wsName);

  virtual void plotWaterfallSpectrum(const std::string &wsName);

  virtual void plotReplacingWindow(const std::string &wsName);

  virtual bool saveOutputFiles() const;

  int currentPlotType() const { return m_currentType; }

  int currentMultiRunMode() const { return m_currentRunMode; }

private slots:
  /// for buttons, do calibrate, focus, event->histo rebin, and similar
  void loadCalibrationClicked();
  void calibrateClicked();
  void CroppedCalibrateClicked();
  void focusClicked();
  void focusCroppedClicked();
  void focusTextureClicked();
  void rebinTimeClicked();
  void rebinMultiperiodClicked();
  void focusStopClicked();

  // slots of the settings tab/section of the interface
  void browseInputDirCalib();
  void browseInputDirRaw();
  void browsePixelCalibFilename();
  void browseTemplateGSAS_PRM();
  void browseDirFocusing();

  // slots for the focusing options
  void browseTextureDetGroupingFile();
  void focusResetClicked();

  // slots of the calibration tab/section of the interface

  // slots of the general part of the interface
  void instrumentChanged(int idx);

  void RBNumberChanged();

  // slot of the cropped calibration part of the interface
  void calibSpecIdChanged(int idx);

  // slots of the focus part of the interface
  void plotRepChanged(int idx);

  // slot of the multi-run mode for focus
  void multiRunModeChanged(int idx);

  // slots of plot spectrum check box status
  void plotFocusStatus();

  // updates the cropped calib run number with new ceria
  void updateCroppedCalibRun();

  // enables the text field when appropriate bank name is selected
  void enableSpecIds();

  // show the standard Mantid help window with this interface's help
  void openHelpWin();

private:
  /// Setup the interface (tab UI)
  virtual void initLayout();
  void doSetupGeneralWidgets();
  void doSetupTabCalib();
  void doSetupTabFocus();
  void doSetupTabPreproc();
  void doSetupTabSettings();

  std::string guessGSASTemplatePath() const;
  std::string guessDefaultFullCalibrationPath() const;

  /// Load default interface settings for each tab, normally on startup
  void readSettings();
  /// save settings (before closing)
  void saveSettings() const;

  // window (custom interface) close
  virtual void closeEvent(QCloseEvent *ev);

  // path/name for the persistent settings group of this interface
  const static std::string m_settingsGroup;

  // here the view puts messages before notifying the presenter to show them
  std::vector<std::string> m_logMsgs;

  /// Interface definition with widgets for the main interface window
  Ui::EnggDiffractionQtGUI m_ui;
  // And its sections/tabs. Note that for compactness they're called simply
  // 'tabs'
  // but they could be separate dialogs, widgets, etc.
  Ui::EnggDiffractionQtTabCalib m_uiTabCalib;
  Ui::EnggDiffractionQtTabFocus m_uiTabFocus;
  Ui::EnggDiffractionQtTabPreproc m_uiTabPreproc;
  Ui::EnggDiffractionQtTabSettings m_uiTabSettings;

  /// converts QList to a vector
  std::vector<std::string> qListToVector(QStringList list,
                                         bool validator) const;

  /// instrument selected (ENGIN-X, etc.)
  std::string m_currentInst;

  /// User select instrument
  void userSelectInstrument(const QString &prefix);

  /// setting the instrument prefix ahead of the run number
  void setPrefix(std::string prefix);

  // current bank number used for cropped calibration
  int static m_currentCropCalibBankName;

  // plot data representation type selected
  int static m_currentType;

  // multi-run focus mode type selected
  int static m_currentRunMode;

  /// current calibration produced in the 'Calibration' tab
  std::string m_currentCalibFilename;
  /// calibration settings - from/to the 'settings' tab
  EnggDiffCalibSettings m_calibSettings;
  std::string m_outCalibFilename;

  /// This is in principle the only settings for 'focus'
  std::string m_focusDir;

  /// for the 'Rebin' parameter of some Engg* algorithms
  static const double g_defaultRebinWidth;

  /// supported file extensions string for IPARM files (for the open
  /// file dialogs)
  static const std::string g_iparmExtStr;
  /// supported file extensions string for the pixel (full) claibration
  static const std::string g_pixelCalibExt;
  /// supported/suggested file extensions for the detector groups file
  /// (focusing)
  static const std::string g_DetGrpExtStr;

  /// presenter as in the model-view-presenter
  boost::scoped_ptr<IEnggDiffractionPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEWQTGUI_H_
