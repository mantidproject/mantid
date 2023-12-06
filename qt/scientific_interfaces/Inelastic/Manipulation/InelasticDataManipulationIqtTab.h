// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IIqtView.h"
#include "InelasticDataManipulationIqtTabModel.h"
#include "InelasticDataManipulationIqtTabView.h"
#include "InelasticDataManipulationTab.h"
#include "ui_InelasticDataManipulationIqtTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class IIqtPresenter {
public:
  virtual void handleSampDataReady(const std::string &wsname) = 0;
  virtual void handleResDataReady(const std::string &resWorkspace) = 0;
  virtual void handleIterationsChanged(int iterations) = 0;
  virtual void handleRunClicked() = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handlePlotCurrentPreview() = 0;
  virtual void handleErrorsClicked(int state) = 0;
  virtual void handleValueChanged(std::string const &propName, double value) = 0;
  virtual void handlePreviewSpectrumChanged(int spectra) = 0;
};

using namespace Mantid::API;
class MANTIDQT_INELASTIC_DLL InelasticDataManipulationIqtTab : public InelasticDataManipulationTab,
                                                               public IIqtPresenter {

public:
  InelasticDataManipulationIqtTab(QWidget *parent, IIqtView *view);
  ~InelasticDataManipulationIqtTab() = default;

  void setup() override;
  void run() override;
  bool validate() override;

  void handleSampDataReady(const std::string &wsname) override;
  void handleResDataReady(const std::string &resWorkspace) override;
  void handleIterationsChanged(int iterations) override;
  void handleRunClicked() override;
  void handleSaveClicked() override;
  void handlePlotCurrentPreview() override;
  void handleErrorsClicked(int state) override;
  void handleValueChanged(std::string const &propName, double value) override;
  void handlePreviewSpectrumChanged(int spectra) override;

protected:
  void runComplete(bool error) override;

private:
  void setFileExtensionsByName(bool filter) override;
  /// Retrieve the selected spectrum
  int getSelectedSpectrum() const;
  /// Sets the selected spectrum
  virtual void setSelectedSpectrum(int spectrum);
  void setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace);
  /// Set input workspace
  void setInputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);

  IIqtView *m_view;
  std::unique_ptr<InelasticDataManipulationIqtTabModel> m_model;

  int m_selectedSpectrum;

  /// Retrieve input workspace
  MatrixWorkspace_sptr getInputWorkspace() const;
  MatrixWorkspace_sptr getPreviewPlotWorkspace();
  std::weak_ptr<MatrixWorkspace> m_previewPlotWorkspace;
  MatrixWorkspace_sptr m_inputWorkspace;
};
} // namespace CustomInterfaces
} // namespace MantidQt
