// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BayesFittingTab.h"
#include "DllConfig.h"
#include "QuasiModel.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"

#include <memory>
#include <string>

namespace MantidQt::CustomInterfaces {
class IQuasiView;

class IQuasiPresenter {
public:
  virtual ~IQuasiPresenter() = default;

  virtual void handleSampleInputReady(std::string const &workspaceName) = 0;
  virtual void handleResolutionInputReady(std::string const &workspaceName) = 0;
  virtual void handleFileAutoLoaded() = 0;

  virtual void handlePreviewSpectrumChanged() = 0;

  virtual void handlePlotCurrentPreview() = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handlePlotClicked() = 0;
};

class MANTIDQT_INELASTIC_DLL QuasiPresenter final : public BayesFittingTab,
                                                    public IQuasiPresenter,
                                                    public IRunSubscriber {

public:
  QuasiPresenter(QWidget *parent, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner,
                 std::unique_ptr<IQuasiModel> model, IQuasiView *view);
  ~QuasiPresenter() override = default;

  const std::string getSubscriberName() const override { return "Quasi"; }

  void handleSampleInputReady(std::string const &workspaceName) override;
  void handleResolutionInputReady(std::string const &workspaceName) override;
  void handleFileAutoLoaded() override;

  void handlePreviewSpectrumChanged() override;

  void handlePlotCurrentPreview() override;
  void handleSaveClicked() override;
  void handlePlotClicked() override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;

  void setFileExtensionsByName(bool const filter) override;
  void setLoadHistory(bool const loadHistory) override;

  void loadSettings(const QSettings &settings) override;

protected:
  void runComplete(IAlgorithm_sptr const &algorithm, bool const error) override;

private:
  void updateMiniPlot();
  void addSpectrum(std::string const &label, Mantid::API::MatrixWorkspace_sptr const &workspace,
                   std::size_t const spectrumIndex, std::string const &colour = "");

  std::unique_ptr<IQuasiModel> m_model;
  IQuasiView *m_view;
};

} // namespace MantidQt::CustomInterfaces
