// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGVIEWQTWIDGET_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGVIEWQTWIDGET_H_

#include "DllConfig.h"
#include "IEnggDiffFittingPresenter.h"
#include "IEnggDiffFittingView.h"
#include "MantidAPI/IPeakFunction.h"

#include "ui_EnggDiffractionQtTabFitting.h"

// Qt classes forward declarations
class QMessageBox;
class QMutex;

class QwtPlotCurve;
class QwtPlotZoomer;

namespace MantidQt {

namespace MantidWidgets {
class PeakPicker;
}

namespace CustomInterfaces {

/**
Qt-based view of the Engineering Diffraction (EnggDiff) fitting
widget/tab. Provides a concrete view that is Qt-based and is probably
the only one that will be implemented in a foreseeable horizon. The
interface of this class is given by IEnggDiffFittingView so that it
fits in the MVP (Model-View-Presenter) design of this GUI.
*/
class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffFittingViewQtWidget
    : public QWidget,
      public IEnggDiffFittingView {
  Q_OBJECT

public:
  EnggDiffFittingViewQtWidget(
      QWidget *parent, boost::shared_ptr<IEnggDiffractionUserMsg> mainMsg,
      boost::shared_ptr<IEnggDiffractionSettings> mainSettings,
      boost::shared_ptr<IEnggDiffractionCalibration> mainCalib,
      boost::shared_ptr<IEnggDiffractionParam> mainParam,
      boost::shared_ptr<IEnggDiffractionPythonRunner> mainPyhonRunner,
      boost::shared_ptr<IEnggDiffractionParam> fileSettings);
  ~EnggDiffFittingViewQtWidget() override;

  /// From the IEnggDiffractionUserMsg interface
  void showStatus(const std::string &sts) override;

  void userWarning(const std::string &warn,
                   const std::string &description) override;

  void userError(const std::string &err,
                 const std::string &description) override;
  void enableCalibrateFocusFitUserActions(bool enable) override;

  /// From the IEnggDiffractionSettings interface
  EnggDiffCalibSettings currentCalibSettings() const override;

  /// From the IEnggDiffractionPythonRunner interface
  virtual std::string enggRunPythonCode(const std::string &pyCode) override;

  void enable(bool enable);

  std::vector<std::string> logMsgs() const override { return m_logMsgs; }

  void setFocusedFileNames(const std::string &paths) override;

  std::string getFocusedFileNames() const override;

  void enableFitAllButton(bool enable) const override;

  void clearFittingListWidget() const override;

  void enableFittingListWidget(bool enable) const override;

  int getFittingListWidgetCurrentRow() const override;

  boost::optional<std::string>
  getFittingListWidgetCurrentValue() const override;

  bool listWidgetHasSelectedRow() const override;

  void updateFittingListWidget(const std::vector<std::string> &rows) override;

  void setFittingListWidgetCurrentRow(int idx) const override;

  std::string getExpectedPeaksInput() const override;

  void setPeakList(const std::string &peakList) const override;

  void resetCanvas() override;

  void setDataVector(std::vector<boost::shared_ptr<QwtData>> &data,
                     bool focused, bool plotSinglePeaks,
                     const std::string &xAxisLabel) override;

  void addRunNoItem(std::string runNo) override;

  std::vector<std::string> getFittingRunNumVec() override;

  void setFittingRunNumVec(std::vector<std::string> assignVec) override;

  double getPeakCentre() const override;

  bool peakPickerEnabled() const override;

  std::string getPreviousDir() const override;

  void setPreviousDir(const std::string &path) override;

  std::string getOpenFile(const std::string &prevPath) override;

  std::string getSaveFile(const std::string &prevPath) override;

  void dataCurvesFactory(std::vector<boost::shared_ptr<QwtData>> &data,
                         std::vector<QwtPlotCurve *> &dataVector, bool focused);

  void setPeakPickerEnabled(bool enabled);

  void setPeakPicker(const Mantid::API::IPeakFunction_const_sptr &peak);

  void setZoomTool(bool enabled);

  void resetView();

  std::string getCurrentInstrument() const override { return m_currentInst; }

  void setCurrentInstrument(const std::string &newInstrument) override {
    m_currentInst = newInstrument;
  }

  bool plotFittedPeaksEnabled() const override;

protected:
  void initLayout();

signals:
  void getBanks();
  void setBank();

private slots:
  // slot of the fitting peaks per part of the interface
  void browseFitFocusedRun();
  void setPeakPick();
  void clearPeakList();
  void loadClicked();
  void fitClicked();
  void fitAllClicked();
  void addClicked();
  void browseClicked();
  void saveClicked();
  void plotSeparateWindow();
  void showToolTipHelp();
  void listWidget_fitting_run_num_clicked(QListWidgetItem *);
  void plotFittedPeaksStateChanged();
  void removeRunClicked();

private:
  /// Setup the interface (tab UI)
  void doSetup();

  /// Load saved/default interface settings
  void readSettings();
  /// save settings (before closing)
  void saveSettings() const override;

  /// converts QList to a vector
  std::vector<std::string> qListToVector(QStringList list,
                                         bool validator) const;

  // path/name for the persistent settings group of this interface
  static const std::string g_settingsGroup;

  static const std::string g_peaksListExt;

  // vector holding directory of focused bank file
  static std::vector<std::string> m_fitting_runno_dir_vec;

  Ui::EnggDiffractionQtTabFitting m_ui;

  // here the view puts messages before notifying the presenter to show them
  std::vector<std::string> m_logMsgs;

  /// Loaded focused workspace
  std::vector<QwtPlotCurve *> m_focusedDataVector;

  /// Loaded data curves
  std::vector<QwtPlotCurve *> m_fittedDataVector;

  /// Peak picker tool for fitting - only one on the plot at any given moment
  MantidWidgets::PeakPicker *m_peakPicker = nullptr;

  /// zoom-in/zoom-out tool for fitting
  QwtPlotZoomer *m_zoomTool = nullptr;

  /// where to go and look for, in particular, focused runs to do fitting on
  boost::shared_ptr<IEnggDiffractionParam> m_fileSettings;

  /// user messages interface provided by a main view/widget
  boost::shared_ptr<IEnggDiffractionUserMsg> m_mainMsgProvider;

  /// settings from the user
  boost::shared_ptr<IEnggDiffractionSettings> m_mainSettings;

  /// interface for the Python runner
  boost::shared_ptr<IEnggDiffractionPythonRunner> m_mainPythonRunner;

  /// presenter as in the model-view-presenter
  boost::shared_ptr<IEnggDiffFittingPresenter> m_presenter;

  /// current selected instrument
  /// updated from the EnggDiffractionPresenter processInstChange
  std::string m_currentInst = "";
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFFITTINGVIEWQTWIDGET_H_
