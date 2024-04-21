// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/IndirectTab.h"
#include "DllConfig.h"
#include "FitDataPresenter.h"
#include "FitOutputOptionsPresenter.h"
#include "FitPlotPresenter.h"
#include "FittingModel.h"
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
  virtual void handleDataAdded(IAddWorkspaceDialog const *dialog) = 0;
  virtual void handleDataChanged() = 0;
  virtual void handleDataRemoved() = 0;
  virtual void handleTableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) = 0;
  virtual void handleTableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) = 0;

  // Used by FitPlotPresenter
  virtual void handleSingleFitClicked(WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) = 0;
  virtual void handleStartXChanged(double startX) = 0;
  virtual void handleEndXChanged(double endX) = 0;
  virtual void handlePlotSpectrumChanged() = 0;
  virtual void handleFwhmChanged(double fwhm) = 0;
  virtual void handleBackgroundChanged(double background) = 0;

  // Used by FitOutputOptionsPresenter
  virtual void handlePlotSelectedSpectra() = 0;

  // Used by InelasticFitPropertyBrowser
  virtual void handleFunctionChanged() = 0;
};

class MANTIDQT_INELASTIC_DLL FitTab : public IndirectTab, public IFitTab {
  Q_OBJECT

public:
  FitTab(std::string const &tabName, QWidget *parent = nullptr);
  virtual ~FitTab() override = default;

  template <typename FittingModel> void setupFittingModel() { m_fittingModel = std::make_unique<FittingModel>(); }

  template <typename TemplateBrowser, typename TemplatePresenter, typename FunctionModel>
  void setupFitPropertyBrowser(std::vector<std::string> const &hiddenProperties, bool const convolveMembers = false,
                               TemplateBrowserCustomizations customizations = TemplateBrowserCustomizations()) {
    auto templateBrowser = new TemplateBrowser(std::move(customizations));
    auto functionModel = std::make_unique<FunctionModel>();
    auto templatePresenter = std::make_unique<TemplatePresenter>(templateBrowser, std::move(functionModel));
    m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplatePresenter(std::move(templatePresenter));
    m_uiForm->dockArea->m_fitPropertyBrowser->init();
    m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(hiddenProperties);
    m_fitPropertyBrowser = m_uiForm->dockArea->m_fitPropertyBrowser;
    m_fitPropertyBrowser->setConvolveMembers(convolveMembers);
    if (convolveMembers)
      m_fitPropertyBrowser->setOutputCompositeMembers(true);
  }

  template <typename FitDataView> void setupFitDataView() {
    m_uiForm->dockArea->setFitDataView(new FitDataView(m_uiForm->dockArea, m_tabName));
  }

  template <typename FitDataPresenter> void setUpFitDataPresenter() {
    m_dataPresenter =
        std::make_unique<FitDataPresenter>(this, m_fittingModel->getFitDataModel(), m_uiForm->dockArea->m_fitDataView);
  }

  void setupOutputOptionsPresenter(bool const editResults = false);
  void setupPlotView(std::optional<std::pair<double, double>> const &xPlotBounds = std::nullopt);
  void subscribeFitBrowserToDataPresenter();

  void handleDataAdded(IAddWorkspaceDialog const *dialog) override;
  void handleDataChanged() override;
  void handleDataRemoved() override;
  void handleTableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) override;
  void handleTableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) override;

  void handleSingleFitClicked(WorkspaceID workspaceID, WorkspaceIndex workspaceIndex) override;
  void handlePlotSpectrumChanged() override;
  void handleFwhmChanged(double fwhm) override;
  void handleBackgroundChanged(double background) override;

  void handlePlotSelectedSpectra() override;

public slots:
  void handleStartXChanged(double startX) override;
  void handleEndXChanged(double endX) override;

  void handleFunctionChanged() override;

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
  void updateFitStatus();
  void updateDataReferences();
  void updateResultOptions();
  void updateFitBrowserParameterValues(const std::unordered_map<std::string, ParameterValue> &parameters =
                                           std::unordered_map<std::string, ParameterValue>());
  void updateFitBrowserParameterValuesFromAlg();

  std::unique_ptr<Ui::FitTab> m_uiForm;

  std::unique_ptr<FitDataPresenter> m_dataPresenter;
  std::unique_ptr<FittingModel> m_fittingModel;
  std::unique_ptr<FitPlotPresenter> m_plotPresenter;
  std::unique_ptr<FitOutputOptionsPresenter> m_outOptionsPresenter;
  InelasticFitPropertyBrowser *m_fitPropertyBrowser;

  std::string m_tabName;
  WorkspaceID m_activeWorkspaceID;
  WorkspaceIndex m_activeSpectrumIndex;
  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
