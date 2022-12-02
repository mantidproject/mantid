// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "InelasticDataManipulationIqtTabView.h"
#include "InelasticDataManipulationTab.h"
#include "ui_InelasticDataManipulationIqtTab.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace Mantid::API;
class DLLExport InelasticDataManipulationIqtTab : public InelasticDataManipulationTab {
  Q_OBJECT

public:
  InelasticDataManipulationIqtTab(QWidget *parent = nullptr);
  ~InelasticDataManipulationIqtTab();

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void setFileExtensionsByName(bool filter) override;

  /// Retrieve the selected spectrum
  int getSelectedSpectrum() const;
  /// Sets the selected spectrum
  virtual void setSelectedSpectrum(int spectrum);

  MatrixWorkspace_sptr getPreviewPlotWorkspace();
  void setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace);
  std::weak_ptr<MatrixWorkspace> m_previewPlotWorkspace;

  /// Retrieve input workspace
  Mantid::API::MatrixWorkspace_sptr getInputWorkspace() const;
  /// Set input workspace
  void setInputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);

  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);

  std::unique_ptr<InelasticDataManipulationIqtTabView> m_view;
  bool m_iqtResFileType;
  int m_selectedSpectrum;
  Mantid::API::MatrixWorkspace_sptr m_inputWorkspace;

private slots:
  void algorithmComplete(bool error);
  void handlePreviewSpectrumChanged(int spectra);
  void plotInput(const QString &wsname);
  void runClicked();
  void saveClicked();
  void plotCurrentPreview();
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
