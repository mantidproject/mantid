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
#include "ResNormModel.h"
#include "ResNormView.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IResNormPresenter {
public:
  virtual ~IResNormPresenter() = default;
  virtual void handlePreviewSpecChanged(int value) = 0;
  virtual void handleVanadiumInputReady(std::string const &filename) = 0;
  virtual void handleResolutionInputReady() = 0;
  virtual void handleDoubleValueChanged(std::string const &propertyName, double value) = 0;
  virtual void handlePlotCurrentPreview() = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handlePlotClicked(std::string const &plotOptions) = 0;
};

class MANTIDQT_INELASTIC_DLL ResNormPresenter : public BayesFittingTab,
                                                public IResNormPresenter,
                                                public IRunSubscriber {
public:
  ResNormPresenter(QWidget *parent, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner,
                   std::unique_ptr<IResNormModel> model, IResNormView *view);
  ~ResNormPresenter() override = default;

  /// Load default settings into the interface
  void loadSettings(const QSettings &settings) override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  void handlePreviewSpecChanged(int value) override;
  void handleVanadiumInputReady(std::string const &filename) override;
  void handleResolutionInputReady() override;
  void handleDoubleValueChanged(std::string const &propertyName, double value) override;
  void handlePlotCurrentPreview() override;
  void handleSaveClicked() override;
  void handlePlotClicked(std::string const &plotOptions) override;

  const std::string getSubscriberName() const override { return "ResNorm"; }

protected:
  void runComplete(IAlgorithm_sptr const &algorithm, bool const error) override;

private:
  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;
  void setSampleLogs() const;
  void updateFitPlot() const;

  std::unique_ptr<IResNormModel> m_model;
  IResNormView *m_view;
  int m_previewSpec;
};
} // namespace CustomInterfaces
} // namespace MantidQt
