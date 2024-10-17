// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidKernel/System.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/Plotting/ExternalPlotter.h"

#include "ui_ALCInterface.h"

namespace MantidQt {
namespace CustomInterfaces {

class ALCDataLoadingPresenter;

class ALCBaselineModellingView;
class ALCBaselineModellingPresenter;
class ALCBaselineModellingModel;

class ALCPeakFittingView;
class ALCPeakFittingPresenter;
class ALCPeakFittingModel;

/** ALCInterface : Custom interface for Avoided Level Crossing analysis
 */
class MANTIDQT_MUONINTERFACE_DLL ALCInterface : public API::UserSubWindow {
  Q_OBJECT

public:
  ALCInterface(QWidget *parent = nullptr);

  void closeEvent(QCloseEvent *event) override;
  static std::string name() { return "ALC"; }
  static QString categoryInfo() { return "Muon"; }

protected:
  void initLayout() override;

private slots:
  void nextStep();
  void previousStep();

  void switchStep(int newStepIndex);

  void exportResults();
  void importResults();

  void updateBaselineData();
  void updatePeakData();
  void externalPlotRequested();

private:
  void importLoadedData(const std::string &workspaceName);
  void importBaselineData(const std::string &workspaceName);
  void importPeakData(const std::string &workspaceName);
  void externallyPlotWorkspace(Mantid::API ::MatrixWorkspace_sptr &data, std::string const &workspaceName,
                               std::string const &workspaceIndices, bool errorBars,
                               std::optional<QHash<QString, QVariant>> const &kwargs);
  void externallyPlotWorkspaces(Mantid::API::MatrixWorkspace_sptr &data, std::vector<std::string> const &workspaceNames,
                                std::vector<int> const &workspaceIndices, std::vector<bool> const &errorBars,
                                std::vector<std::optional<QHash<QString, QVariant>>> const &spectraKwargs);
  void externalPlotDataLoading();
  void externalPlotBaselineModel();
  void externalPlotPeakFitting();

  /// UI form
  Ui::ALCInterface m_ui;

  // Step views
  ALCPeakFittingView *m_peakFittingView;

  // Step presenters
  ALCDataLoadingPresenter *m_dataLoading;
  ALCBaselineModellingPresenter *m_baselineModelling;
  ALCPeakFittingPresenter *m_peakFitting;

  // Models
  ALCPeakFittingModel *m_peakFittingModel;

  /// Name for every step for labels
  static const QStringList STEP_NAMES;

  /// Format of the label at the bottom
  static const QString LABEL_FORMAT;

  /// External plotter
  std::unique_ptr<Widgets::MplCpp::ExternalPlotter> m_externalPlotter;

  /// Steps of the ALC interface
  enum Steps { DataLoading = 0, BaselineModel = 1, PeakFitting = 2 };
};

} // namespace CustomInterfaces
} // namespace MantidQt
