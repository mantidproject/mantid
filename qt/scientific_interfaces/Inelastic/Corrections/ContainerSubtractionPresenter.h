// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ContainerSubtractionModel.h"
#include "ContainerSubtractionView.h"
#include "CorrectionsTab.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h"

namespace MantidQt::CustomInterfaces {

class IContainerSubtractionView;

class IContainerSubtractionPresenter {
public:
  virtual ~IContainerSubtractionPresenter() = default;

  virtual void handleRun() = 0;
  virtual void handleValidation(MantidQt::CustomInterfaces::IUserInputValidator *validator) const = 0;
  virtual void handleSampleReady(const std::string &dataName) = 0;
  virtual void handleCanReady(const std::string &dataName) = 0;
  virtual void handlePlotPreviewClicked() = 0;
  virtual void updatePlot(int specNo) = 0;
  virtual void handleSaveClicked() = 0;
  virtual void handleUpdateContainerPlot() = 0;
};

class MANTIDQT_INELASTIC_DLL ContainerSubtractionPresenter : public CorrectionsTab,
                                                             public IContainerSubtractionPresenter,
                                                             public IRunSubscriber {

public:
  ContainerSubtractionPresenter(QWidget *parent, std::unique_ptr<API::IAlgorithmRunner> algoRunner,
                                std::unique_ptr<IContainerSubtractionModel> model, IContainerSubtractionView *view);
  ~ContainerSubtractionPresenter() override = default;

  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;
  void handleSampleReady(const std::string &dataName) override;
  void handleCanReady(const std::string &dataName) override;
  void handlePlotPreviewClicked() override;
  void handleSaveClicked() override;
  void updatePlot(int specNo) override;
  void handleUpdateContainerPlot() override;
  const std::string getSubscriberName() const override { return "ContainerSubtraction"; }

protected:
  void runComplete(const Mantid::API::IAlgorithm_sptr algorithm, const bool error) override;

private:
  std::string createOutputName();
  void loadSettings(const QSettings &settings) override;
  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;

  bool requestRebinToSample() const;
  void updateOutputName();
  void updateNewDataEntry(const MatrixWorkspace_sptr &ws);

  std::unique_ptr<IContainerSubtractionModel> m_model;
  IContainerSubtractionView *m_view;
};
} // namespace MantidQt::CustomInterfaces
