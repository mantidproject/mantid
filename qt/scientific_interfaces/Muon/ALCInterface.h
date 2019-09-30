// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALCINTERFACE_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCINTERFACE_H_

#include "MantidKernel/System.h"

#include "DllConfig.h"

#include "MantidQtWidgets/Common/UserSubWindow.h"

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

private:
  /// UI form
  Ui::ALCInterface m_ui;

  // Step views
  ALCBaselineModellingView *m_baselineModellingView;
  ALCPeakFittingView *m_peakFittingView;

  // Step presenters
  ALCDataLoadingPresenter *m_dataLoading;
  ALCBaselineModellingPresenter *m_baselineModelling;
  ALCPeakFittingPresenter *m_peakFitting;

  // Models
  ALCBaselineModellingModel *m_baselineModellingModel;
  ALCPeakFittingModel *m_peakFittingModel;

  /// Name for every step for labels
  static const QStringList STEP_NAMES;

  /// Format of the label at the bottom
  static const QString LABEL_FORMAT;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALCINTERFACE_H_ */
