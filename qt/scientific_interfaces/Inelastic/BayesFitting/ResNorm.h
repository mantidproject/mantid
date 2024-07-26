// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BayesFittingTab.h"
#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ui_ResNorm.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INELASTIC_DLL ResNorm : public BayesFittingTab, public IRunSubscriber {
  Q_OBJECT

public:
  ResNorm(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;

private slots:
  /// Handle completion of the algorithm
  void handleAlgorithmComplete(bool error);
  /// Handle when the vanadium input is ready
  void handleVanadiumInputReady(const QString &filename);
  /// Handle when the resolution input is ready
  void handleResolutionInputReady(const QString &filename);
  /// Slot for when the min range on the range selector changes
  void minValueChanged(double min);
  /// Slot for when the min range on the range selector changes
  void maxValueChanged(double max);
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val) override;
  /// Slot to handle the preview spectrum being changed
  void previewSpecChanged(int value);
  /// Slots to handle plot and save
  void saveClicked();
  void plotClicked();
  void plotCurrentPreview();

private:
  void setFileExtensionsByName(bool filter) override;

  void processLogs();
  void addAdditionalLogs(const Mantid::API::WorkspaceGroup_sptr &resultGroup) const;
  void addAdditionalLogs(const Mantid::API::Workspace_sptr &resultWorkspace) const;
  std::map<std::string, std::string> getAdditionalLogStrings() const;
  std::map<std::string, std::string> getAdditionalLogNumbers() const;
  double getDoubleManagerProperty(QString const &propName) const;
  void copyLogs(const Mantid::API::MatrixWorkspace_sptr &resultWorkspace,
                const Mantid::API::WorkspaceGroup_sptr &resultGroup) const;
  void copyLogs(const Mantid::API::MatrixWorkspace_sptr &resultWorkspace,
                const Mantid::API::Workspace_sptr &workspace) const;

  void setPlotResultEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setPlotResultIsPlotting(bool plotting);

  /// Current preview spectrum
  int m_previewSpec;
  /// The ui form
  Ui::ResNorm m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt
