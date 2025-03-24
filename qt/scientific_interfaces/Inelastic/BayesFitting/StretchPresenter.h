// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BayesFittingTab.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "StretchData.h"
#include "StretchModel.h"
#include "StretchView.h"
#include "ui_Stretch.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IStretchPresenter : public IRunSubscriber,
                                                 public IStretchViewSubscriber,
                                                 public API::IAlgorithmRunnerSubscriber {
public:
  virtual ~IStretchPresenter() = default;
};

class MANTIDQT_INELASTIC_DLL StretchPresenter : public BayesFittingTab, public IStretchPresenter {

public:
  StretchPresenter(QWidget *parent, IStretchView *view, std::unique_ptr<IStretchModel> model,
                   std::unique_ptr<API::IAlgorithmRunner> algorithmRunner);
  ~StretchPresenter() override = default;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  const std::string getSubscriberName() const override { return "Stretch"; }

  void loadSettings(const QSettings &settings) override;
  void applySettings(std::map<std::string, QVariant> const &settings) override;

  void notifySaveClicked() override;
  void notifyPlotClicked() override;
  void notifyPlotContourClicked() override;
  void notifyPlotCurrentPreviewClicked() override;
  void notifyPreviewSpecChanged(int specNum) override;

protected:
  void runComplete(IAlgorithm_sptr const &algorithm, bool const error) override;

private:
  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;

  void setButtonsEnabled(bool enabled);
  void setPlotResultIsPlotting(bool plotting);
  void setPlotContourIsPlotting(bool plotting);

  void resetPlotContourOptions();

  int m_previewSpec;

  std::string m_fitWorkspaceName;
  std::string m_contourWorkspaceName;

  IStretchView *m_view;
  std::unique_ptr<IStretchModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt
