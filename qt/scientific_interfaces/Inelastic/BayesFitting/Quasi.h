// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BayesFittingTab.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ui_Quasi.h"

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_INELASTIC_DLL Quasi : public BayesFittingTab {
  Q_OBJECT

public:
  Quasi(QWidget *parent = nullptr);

  // Inherited methods from BayesFittingTab
  void setup() override;
  bool validate() override;
  void run() override;
  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

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

  void runClicked();
  void plotClicked();
  void plotCurrentPreview();
  void saveClicked();

private:
  void displayMessageAndRun(std::string const &saveDirectory);
  int displaySaveDirectoryMessage();

  void setFileExtensionsByName(bool filter) override;

  void setRunEnabled(bool enabled);
  void setPlotResultEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);
  void setPlotResultIsPlotting(bool plotting);

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
