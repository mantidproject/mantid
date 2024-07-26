// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BayesFittingTab.h"
#include "DllConfig.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ui_Stretch.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INELASTIC_DLL Stretch : public BayesFittingTab, public IRunSubscriber {
  Q_OBJECT

public:
  Stretch(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  // Slot for when settings are changed
  void applySettings(std::map<std::string, QVariant> const &settings) override;

  // Setup fit options, property browser, and plot options ui elements
  void setupFitOptions();
  void setupPropertyBrowser();
  void setupPlotOptions();

private slots:
  /// Slot for when the min range on the range selector changes
  void minValueChanged(double min);
  /// Slot for when the min range on the range selector changes
  void maxValueChanged(double max);
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val) override;
  /// Slot to handle when a new sample file is available
  void handleSampleInputReady(const QString &filename);
  /// Save the workspaces produces from the algorithm
  void saveWorkspaces();

  void plotWorkspaces();
  void plotContourClicked();
  void algorithmComplete(const bool &error);
  void plotCurrentPreview();
  void previewSpecChanged(int value);

private:
  void setFileExtensionsByName(bool filter) override;

  void populateContourWorkspaceComboBox();
  int displaySaveDirectoryMessage();

  void setPlotResultEnabled(bool enabled);
  void setPlotContourEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setPlotResultIsPlotting(bool plotting);
  void setPlotContourIsPlotting(bool plotting);

  /// Current preview spectrum
  int m_previewSpec;
  // The ui form
  Ui::Stretch m_uiForm;
  // Output Names
  std::string m_fitWorkspaceName;
  std::string m_contourWorkspaceName;
  // state of plot and save when algorithm is run
  bool m_save;
};
} // namespace CustomInterfaces
} // namespace MantidQt
