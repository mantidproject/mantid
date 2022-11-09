// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "InelasticDataManipulationTab.h"
#include "ui_InelasticDataManipulationIqtTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport InelasticDataManipulationIqtTab : public InelasticDataManipulationTab {
  Q_OBJECT

public:
  InelasticDataManipulationIqtTab(QWidget *parent = nullptr);
  ~InelasticDataManipulationIqtTab();

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadTabSettings(const QSettings &settings);
  void setFileExtensionsByName(bool filter) override;

  /// Retrieve the selected spectrum
  int getSelectedSpectrum() const;
  /// Sets the selected spectrum
  virtual void setSelectedSpectrum(int spectrum);

  /// Retrieve input workspace
  Mantid::API::MatrixWorkspace_sptr getInputWorkspace() const;
  /// Set input workspace
  void setInputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);

  bool isErrorsEnabled();

  void setRunEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);
  void setPreviewSpectrumMaximum(int value);

  Ui::InelasticDataManipulationIqtTab m_uiForm;
  QtTreePropertyBrowser *m_iqtTree;
  bool m_iqtResFileType;
  int m_selectedSpectrum;
  Mantid::API::MatrixWorkspace_sptr m_inputWorkspace;

private slots:
  void algorithmComplete(bool error);
  void handlePreviewSpectrumChanged(int spectra);
  void plotInput(const QString &wsname);
  void plotInput(MantidQt::MantidWidgets::PreviewPlot *previewPlot);
  void rangeChanged(double min, double max);
  void updateRangeSelector(QtProperty *prop, double val);
  void updateDisplayedBinParameters();
  void runClicked();
  void saveClicked();
  void errorsClicked();
  void updateEnergyRange(int state);
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
