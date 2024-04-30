// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/InelasticTab.h"
#include "DllConfig.h"
#include "FitDataPresenter.h"
#include "FitOutputOptionsPresenter.h"
#include "FitPlotPresenter.h"
#include "FittingPresenter.h"
#include "FunctionBrowser/TemplateSubType.h"
#include "InelasticFitPropertyBrowser.h"
#include "ui_FitTab.h"

#include <memory>
#include <optional>

#include <QString>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL IFitTab {
public:
  // Used by FitDataPresenter
  virtual std::string tabName() const = 0;
  virtual void handleDataAdded(IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleDataChanged() = 0;
  virtual void handleDataRemoved() = 0;
  virtual void handleTableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) = 0;
  virtual void handleTableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) = 0;
  virtual void handleFunctionListChanged(const std::map<std::string, std::string> &functionStrings) = 0;

  // Used by FitPlotPresenter
  virtual void handleSingleFitClicked(WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) = 0;
  virtual void handleStartXChanged(double startX) = 0;
  virtual void handleEndXChanged(double endX) = 0;
  virtual void handlePlotSpectrumChanged() = 0;
  virtual void handleFwhmChanged(double fwhm) = 0;
  virtual void handleBackgroundChanged(double background) = 0;

  // Used by FitOutputOptionsPresenter
  virtual void handlePlotSelectedSpectra() = 0;

  // Used by FittingModel
  virtual void handleFunctionChanged() = 0;
};

class MANTIDQT_INELASTIC_DLL FitTab : public InelasticTab, public IFitTab {
  Q_OBJECT

public:
  FitTab(QWidget *parent, std::string const &tabName);
  virtual ~FitTab() override = default;

  template <typename TemplateBrowser, typename TemplatePresenter, typename FunctionModel>
  void setupFitPropertyBrowser(std::vector<std::string> const &hiddenProperties, bool const convolveMembers = false,
                               TemplateBrowserCustomizations customizations = TemplateBrowserCustomizations()) {
    auto templateBrowser = new TemplateBrowser(std::move(customizations));
    auto functionModel = std::make_unique<FunctionModel>();
    auto templatePresenter = std::make_unique<TemplatePresenter>(templateBrowser, std::move(functionModel));
    m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplatePresenter(std::move(templatePresenter));
    m_uiForm->dockArea->m_fitPropertyBrowser->init();
    m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(hiddenProperties);
    m_uiForm->dockArea->m_fitPropertyBrowser->setConvolveMembers(convolveMembers);
    if (convolveMembers)
      m_uiForm->dockArea->m_fitPropertyBrowser->setOutputCompositeMembers(true);
  }

  template <typename FittingModel> void setupFittingPresenter() {
    auto model = std::make_unique<FittingModel>();
    m_fittingPresenter =
        std::make_unique<FittingPresenter>(this, m_uiForm->dockArea->m_fitPropertyBrowser, std::move(model));
  }

  template <typename FitDataView> void setupFitDataView() {
    m_uiForm->dockArea->setFitDataView(new FitDataView(m_uiForm->dockArea));
  }

  template <typename FitDataPresenter> void setUpFitDataPresenter() {
    m_dataPresenter = std::make_unique<FitDataPresenter>(this, m_fittingPresenter->getFitDataModel(),
                                                         m_uiForm->dockArea->m_fitDataView);
  }

  void setupOutputOptionsPresenter(bool const editResults = false);
  void setupPlotView(std::optional<std::pair<double, double>> const &xPlotBounds = std::nullopt);

  std::string tabName() const override;

  void handleDataAdded(IAddWorkspaceDialog const *dialog) override;
  void handleDataChanged() override;
  void handleDataRemoved() override;
  void handleTableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) override;
  void handleTableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) override;
  void handleFunctionListChanged(const std::map<std::string, std::string> &functionStrings) override;

  void handleSingleFitClicked(WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) override;
  void handlePlotSpectrumChanged() override;
  void handleFwhmChanged(double fwhm) override;
  void handleBackgroundChanged(double background) override;

  void handlePlotSelectedSpectra() override;

  void handleFunctionChanged() override;

public slots:
  void handleStartXChanged(double startX) override;
  void handleEndXChanged(double endX) override;

private slots:
  void updateFitOutput(bool error);
  void updateSingleFitOutput(bool error);
  void fitAlgorithmComplete(bool error);

private:
  void setup() override;
  bool validate() override;
  void run() override;

  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void runSingleFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  void enableFitButtons(bool enable);
  void enableOutputOptions(bool enable);
  void setPDFWorkspace(std::string const &workspaceName);
  void setModelFitFunction();

  void updateParameterEstimationData();
  void updateDataReferences();
  void updateResultOptions();

  std::unique_ptr<Ui::FitTab> m_uiForm;

  std::unique_ptr<FitDataPresenter> m_dataPresenter;
  std::unique_ptr<FittingPresenter> m_fittingPresenter;
  std::unique_ptr<FitPlotPresenter> m_plotPresenter;
  std::unique_ptr<FitOutputOptionsPresenter> m_outOptionsPresenter;

  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
