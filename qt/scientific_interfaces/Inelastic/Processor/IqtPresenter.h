// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataProcessor.h"
#include "IIqtView.h"
#include "IqtModel.h"
#include "IqtView.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ui_IqtTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class IIqtPresenter {
public:
  virtual ~IIqtPresenter() = default;
  virtual void handleSampDataReady(const std::string &wsname) = 0;
  virtual void handleResDataReady(const std::string &resWorkspace) = 0;
  virtual void handleIterationsChanged(int iterations) = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handlePlotCurrentPreview() = 0;
  virtual void handleErrorsClicked(int state) = 0;
  virtual void handleNormalizationClicked(int state) = 0;
  virtual void handleValueChanged(std::string const &propName, double value) = 0;
  virtual void handlePreviewSpectrumChanged(int spectra) = 0;
};

using namespace Mantid::API;
class MANTIDQT_INELASTIC_DLL IqtPresenter : public DataProcessor, public IIqtPresenter, public IRunSubscriber {

public:
  IqtPresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner, IIqtView *view,
               std::unique_ptr<IIqtModel> model);
  ~IqtPresenter() override = default;

  // runWidget
  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  const std::string getSubscriberName() const override { return "IQT Data Processor"; }

  void handleSampDataReady(const std::string &wsname) override;
  void handleResDataReady(const std::string &resWorkspace) override;
  void handleIterationsChanged(int iterations) override;
  void handleSaveClicked() override;
  void handlePlotCurrentPreview() override;
  void handleErrorsClicked(int state) override;
  void handleNormalizationClicked(int state) override;
  void handleValueChanged(std::string const &propName, double value) override;
  void handlePreviewSpectrumChanged(int spectra) override;

protected:
  void runComplete(Mantid::API::IAlgorithm_sptr algorithm, bool error) override;

private:
  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;
  /// Retrieve the selected spectrum
  int getSelectedSpectrum() const;
  /// Sets the selected spectrum
  void setSelectedSpectrum(int spectrum);
  /// Set input workspace
  void setInputWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace);

  IIqtView *m_view;
  std::unique_ptr<IIqtModel> m_model;

  int m_selectedSpectrum;

  /// Retrieve input workspace
  MatrixWorkspace_sptr getInputWorkspace() const;
  MatrixWorkspace_sptr m_inputWorkspace;
};
} // namespace CustomInterfaces
} // namespace MantidQt
