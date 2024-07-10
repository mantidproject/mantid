// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BayesFittingTab.h"
#include "Common/RunWidget/IRunSubscriber.h"
#include "Common/RunWidget/RunPresenter.h"
#include "DllConfig.h"
#include "ui_Quasi.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INELASTIC_DLL Quasi : public BayesFittingTab, public IRunSubscriber {
  Q_OBJECT

public:
  Quasi(QWidget *parent = nullptr);

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;

private slots:
  /// Slot for when the min range on the range selector changes
  void minValueChanged(double min);
  /// Slot for when the min range on the range selector changes
  void maxValueChanged(double max);
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val) override;
  /// Slot to handle when a new sample file is available
  void handleSampleInputReady(const QString &filename);
  /// Slot to handle when a new resolution file is available
  void handleResolutionInputReady(const QString &wsName);
  /// slot to handle when the user changes the program to be used
  void handleProgramChange(int index);
  /// Slot to handle setting a new preview spectrum
  void previewSpecChanged(int value);
  /// Handles updating spectra in mini plot
  void updateMiniPlot();
  /// Handles what happen after the algorithm is run
  void algorithmComplete(bool error);

  void plotClicked();
  void plotCurrentPreview();
  void saveClicked();

private:
  int displaySaveDirectoryMessage();

  void setFileExtensionsByName(bool filter) override;

  void setPlotResultEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setPlotResultIsPlotting(bool plotting);

  std::unique_ptr<IRunPresenter> m_runPresenter;

  std::string m_outputBaseName;
  /// Current preview spectrum
  int m_previewSpec;
  /// The ui form
  Ui::Quasi m_uiForm;
  /// alg
  Mantid::API::IAlgorithm_sptr m_QuasiAlg;
};
} // namespace CustomInterfaces
} // namespace MantidQt
