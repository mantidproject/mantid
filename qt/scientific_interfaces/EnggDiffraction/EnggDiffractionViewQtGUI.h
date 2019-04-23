// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEWQTGUI_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEWQTGUI_H_

#include "DllConfig.h"
#include "EnggDiffFittingViewQtWidget.h"
#include "EnggDiffGSASFittingViewQtWidget.h"
#include "IEnggDiffractionPresenter.h"
#include "IEnggDiffractionView.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/Plotting/Qwt/PeakPicker.h"

#include "ui_EnggDiffractionQtGUI.h"
#include "ui_EnggDiffractionQtTabCalib.h"
#include "ui_EnggDiffractionQtTabFocus.h"
#include "ui_EnggDiffractionQtTabPreproc.h"
#include "ui_EnggDiffractionQtTabSettings.h"

// Qt classes forward declarations
class QMessageBox;
class QMutex;

class QwtPlotCurve;
class QwtPlotZoomer;

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt-based view of the Engineering Diffraction (EnggDiffraction)
GUI. Provides a concrete view for the graphical interface for Engg
functionality in Mantid. This view is Qt-based and it is probably the
only one that will be implemented in a foreseeable horizon. The
interface of this class is given by IEnggDiffractionView so that it
fits in the MVP (Model-View-Presenter) design of this GUI.
*/
class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffractionViewQtGUI
    : public MantidQt::API::UserSubWindow,
      public IEnggDiffractionView {
  Q_OBJECT

public:
  /// Default Constructor
  EnggDiffractionViewQtGUI(QWidget *parent = nullptr);
  /// Interface name
  static std::string name() { return "Engineering Diffraction"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Diffraction"; }

  void splashMessage(bool visible, const std::string &shortMsg,
                     const std::string &description) override;

  void showStatus(const std::string &sts) override;

  void userWarning(const std::string &warn,
                   const std::string &description) override;

  void userError(const std::string &err,
                 const std::string &description) override;

  std::string
  askNewCalibrationFilename(const std::string &suggestedFname) override;

  std::string askExistingCalibFilename() override;

  std::vector<std::string> logMsgs() const override { return m_logMsgs; }

  std::string getRBNumber() const override;

  EnggDiffCalibSettings currentCalibSettings() const override {
    return m_calibSettings;
  }

  std::string currentInstrument() const override { return m_currentInst; }

  std::string currentVanadiumNo() const override;

  std::string currentCeriaNo() const override;

  std::string currentCalibFile() const override;

  std::vector<std::string> newVanadiumNo() const override;

  std::vector<std::string> newCeriaNo() const override;

  int currentCropCalibBankName() const override {
    return g_currentCropCalibBankName;
  }

  std::string currentCalibSpecNos() const override;

  std::string currentCalibCustomisedBankName() const override;

  void newCalibLoaded(const std::string &vanadiumNo, const std::string &ceriaNo,
                      const std::string &fname) override;

  std::string enggRunPythonCode(const std::string &pyCode) override;

  void enableTabs(bool enable) override;

  void enableCalibrateFocusFitUserActions(bool enable) override;

  std::vector<std::string> focusingRunNo() const override;

  std::vector<std::string> focusingCroppedRunNo() const override;

  std::vector<std::string> focusingTextureRunNo() const override;

  std::vector<bool> focusingBanks() const override;

  std::string focusingCroppedSpectrumNos() const override;

  std::string focusingTextureGroupingFile() const override;

  bool focusedOutWorkspace() const override;

  bool plotCalibWorkspace() const override;

  void resetFocus() override;

  std::vector<std::string> currentPreprocRunNo() const override;

  double rebinningTimeBin() const override;

  size_t rebinningPulsesNumberPeriods() const override;

  double rebinningPulsesTime() const override;

  void plotFocusedSpectrum(const std::string &wsName) override;

  void plotWaterfallSpectrum(const std::string &wsName) override;

  void plotReplacingWindow(const std::string &wsName,
                           const std::string &spectrum,
                           const std::string &type) override;

  void plotCalibOutput(const std::string &pyCode) override;

  bool saveFocusedOutputFiles() const override;

  void showInvalidRBNumber(const bool rbNumberIsValid) override;

  int currentPlotType() const override { return g_currentType; }

  int currentMultiRunMode() const override { return g_currentRunMode; }

  void updateTabsInstrument(const std::string &newInstrument) override;

signals:
  void getBanks();
  void setBank();

private slots:
  /// for buttons, do calibrate, focus, event->histo rebin, and similar
  void loadCalibrationClicked();
  void calibrateClicked();
  void CroppedCalibrateClicked();
  void focusClicked();
  void focusCroppedClicked();
  void focusTextureClicked();
  void focusStopClicked();
  void rebinTimeClicked();
  void rebinMultiperiodClicked();

  // slots of the settings tab/section of the interface
  void browseInputDirCalib();
  void browseInputDirRaw();
  void browsePixelCalibFilename();
  void browseTemplateGSAS_PRM();
  void forceRecalculateStateChanged();

  // slots for the focusing options
  void browseTextureDetGroupingFile();
  void focusResetClicked();

  // slots of the calibration tab/section of the interface

  // slots of the general part of the interface
  void instrumentChanged(int idx);

  void RBNumberChanged();

  // slot of the cropped calibration part of the interface
  void calibspecNoChanged(int idx);

  // slots of the focus part of the interface
  void plotRepChanged(int idx);

  // slot of the multi-run mode for focus
  void multiRunModeChanged(int idx);

  // slots of plot spectrum check box status
  void plotFocusStatus();

  // enables the text field when appropriate bank name is selected
  void enableSpecNos();

  // show the standard Mantid help window with this interface's help
  void openHelpWin();

private:
  /// Setup the interface (tab UI)
  void initLayout() override;
  void doSetupGeneralWidgets();
  void doSetupSplashMsg();
  void doSetupTabCalib();
  void doSetupTabFocus();
  void doSetupTabPreproc();
  void doSetupTabSettings();

  std::string guessGSASTemplatePath() const;
  std::string guessDefaultFullCalibrationPath() const;

  /// Load default interface settings for each tab, normally on startup
  void readSettings();
  /// save settings (before closing)
  void saveSettings() const override;

  // when the interface is shown
  void showEvent(QShowEvent * /*unused*/) override;

  // window (custom interface) close
  void closeEvent(QCloseEvent *ev) override;

  // path/name for the persistent settings group of this interface
  const static std::string g_settingsGroup;

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
  // Ui::EnggDiffractionQtTabFitting m_uiTabFitting;
  EnggDiffFittingViewQtWidget *m_fittingWidget;
  EnggDiffGSASFittingViewQtWidget *m_gsasWidget;
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

  // TODO: The values of these three next static members (bank name,
  // type, run mode) can be obtained from widgets when requested/required.  They
  // shouldn't need to be cached in data members. Remove them.

  // current bank number used for cropped calibration
  int static g_currentCropCalibBankName;

  // plot data representation type selected
  int static g_currentType;

  // multi-run focus mode type selected
  int static g_currentRunMode;

  /// calibration settings - from/to the 'settings' tab
  EnggDiffCalibSettings m_calibSettings;

  /// To show important non-modal messages
  QMessageBox *m_splashMsg;

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
  boost::shared_ptr<IEnggDiffractionPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONVIEWQTGUI_H_
