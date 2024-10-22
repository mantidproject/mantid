// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MuonPeriodInfo.h"

#include "DllConfig.h"
#include "IALCDataLoadingPresenter.h"
#include "IALCDataLoadingView.h"
#include <QFileSystemWatcher>
#include <QTimer>

#include "ui_ALCDataLoadingView.h"

namespace MantidQt {
namespace MantidWidgets {
class LogValueSelector;
} // namespace MantidWidgets
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {

/** ALCDataLoadingView : ALC Data Loading view interface implementation using Qt
  widgets
*/

class MANTIDQT_MUONINTERFACE_DLL ALCDataLoadingView : public IALCDataLoadingView {
public:
  ALCDataLoadingView(QWidget *widget);
  ~ALCDataLoadingView();

  // -- IALCDataLoadingView interface
  // ------------------------------------------------------------
  void subscribePresenter(IALCDataLoadingPresenter *presenter) override;

  void initialize() override;

  void initInstruments() override;
  std::string getInstrument() const override;
  std::string getPath() const override;
  std::string log() const override;
  std::string function() const override;
  std::string deadTimeType() const override;
  std::string deadTimeFile() const override;
  std::string detectorGroupingType() const override;
  std::string getForwardGrouping() const override;
  std::string getBackwardGrouping() const override;
  std::string redPeriod() const override;
  std::string greenPeriod() const override;
  bool subtractIsChecked() const override;
  std::string calculationType() const override;
  std::optional<std::pair<double, double>> timeRange() const override;
  std::string getRunsText() const override;
  std::string getRunsFirstRunText() const override;

  void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) override;
  void displayError(const std::string &error) override;
  bool displayWarning(const std::string &warning) override;
  void setAvailableLogs(const std::vector<std::string> &logs) override;
  void setAvailablePeriods(const std::vector<std::string> &periods) override;
  void setTimeLimits(double tMin, double tMax) override;
  void setTimeRange(double tMin, double tMax) override;
  void help() override;
  void disableAll() override;
  void enableAll() override;
  void setAvailableInfoToEmpty() override;
  void enableLoad(bool enable) override;
  void setPath(const std::string &path) override;
  void enableRunsAutoAdd(bool enable) override;
  void setInstrument(const std::string &instrument) override;
  std::string getRunsError() override;
  std::vector<std::string> getFiles() override;
  std::string getFirstFile() override;
  void setLoadStatus(const std::string &status, const std::string &colour) override;
  void runsAutoAddToggled(bool on) override;
  void setRunsTextWithoutSearch(const std::string &text) override;
  void toggleRunsAutoAdd(const bool autoAdd) override;
  void enableAlpha(const bool alpha) override;
  bool isAlphaEnabled() const override;
  void setAlphaValue(const std::string &alpha) override;
  std::string getAlphaValue() const override;
  void showAlphaMessage(const bool alpha) override;
  void setFileExtensions(const std::vector<std::string> &extensions) override;
  std::shared_ptr<MantidQt::MantidWidgets::MuonPeriodInfo> getPeriodInfo() override;

  // -- End of IALCDataLoadingView interface
  // -----------------------------------------------------

  // Slots
  void handleStartWatching(bool watch) override;
  void handleTimerEvent() override;
  void instrumentChanged(QString instrument) override;
  void notifyLoadClicked() override;
  void notifyRunsEditingChanged() override;
  void notifyRunsEditingFinished() override;
  void notifyRunsFoundFinished() override;
  void openManageDirectories() override;
  void notifyPeriodInfoClicked() override;

private:
  /// Common function to set available items in a combo box
  void setAvailableItems(QComboBox *comboBox, const std::vector<std::string> &items);

  bool setCurrentLog(const QString &log);

  /// UI form
  Ui::ALCDataLoadingView m_ui;

  /// The widget used
  QWidget *const m_widget;

  /// Watches the path for changes
  QFileSystemWatcher m_watcher;

  /// Timer of running timer
  QTimer m_timer;

  /// Period Info Widget displayed from the view
  std::shared_ptr<MantidQt::MantidWidgets::MuonPeriodInfo> m_periodInfo;

  QString m_selectedLog;
  size_t m_numPeriods;
  IALCDataLoadingPresenter *m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
