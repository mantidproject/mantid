// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/IndirectTab.h"
#include "DllConfig.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFitOutputOptionsPresenter.h"
#include "IndirectFitPlotPresenter.h"
#include "IndirectFitPropertyBrowser.h"
#include "IndirectFittingModel.h"
#include "ui_IndirectFitTab.h"

#include "MantidQtWidgets/Common/FunctionModelDataset.h"

#include <boost/optional.hpp>

#include <QtCore>

#include <memory>
#include <optional>
#include <type_traits>

#include <QList>
#include <QPair>
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INELASTIC_DLL IIndirectDataAnalysisTab {
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
};

class MANTIDQT_INELASTIC_DLL IndirectDataAnalysisTab : public IndirectTab, public IIndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisTab(std::string const &tabName, bool const hasResolution, QWidget *parent = nullptr);
  virtual ~IndirectDataAnalysisTab() override = default;

  template <typename FittingModel> void setupFittingModel() { m_fittingModel = std::make_unique<FittingModel>(); }

  template <typename TemplateBrowser, typename FunctionModel>
  void setupFitPropertyBrowser(std::vector<std::string> const &hiddenProperties, bool const convolveMembers = false) {
    auto functionModel = std::make_unique<FunctionModel>();
    m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplateBrowser(new TemplateBrowser(std::move(functionModel)));
    m_uiForm->dockArea->m_fitPropertyBrowser->init();
    m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(hiddenProperties);
    m_fitPropertyBrowser = m_uiForm->dockArea->m_fitPropertyBrowser;
    setConvolveMembers(convolveMembers);
  }

  template <typename FitDataView> void setupFitDataView() {
    m_uiForm->dockArea->setFitDataView(new FitDataView(m_uiForm->dockArea));
  }

  template <typename FitDataPresenter> void setUpFitDataPresenter() {
    m_dataPresenter =
        std::make_unique<FitDataPresenter>(this, m_fittingModel->getFitDataModel(), m_uiForm->dockArea->m_fitDataView);
  }

  void setupOutputOptionsPresenter(bool const editResults = false);
  void setupPlotView(std::optional<std::pair<double, double>> const &xPlotBounds = std::nullopt);

  WorkspaceID getSelectedDataIndex() const;
  WorkspaceIndex getSelectedSpectrum() const;
  bool isRangeCurrentlySelected(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  size_t getNumberOfCustomFunctions(const std::string &functionName) const;
  void setConvolveMembers(bool convolveMembers);

  static size_t getNumberOfSpecificFunctionContained(const std::string &functionName,
                                                     const IFunction *compositeFunction);

  std::string getTabName() const noexcept { return m_tabName; }
  bool hasResolution() const noexcept { return m_hasResolution; }
  void setFileExtensionsByName(bool filter);

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

protected:
  IndirectFittingModel *getFittingModel() const;
  void run() override;
  void setSampleWSSuffixes(const QStringList &suffices);
  void setSampleFBSuffixes(const QStringList &suffices);
  void setResolutionWSSuffixes(const QStringList &suffices);
  void setResolutionFBSuffixes(const QStringList &suffices);
  void setSampleSuffixes(std::string const &tab, bool filter);
  void setResolutionSuffixes(std::string const &tab, bool filter);

  void setAlgorithmProperties(const Mantid::API::IAlgorithm_sptr &fitAlgorithm) const;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void runSingleFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  void setRunIsRunning(bool running);
  void setRunEnabled(bool enable);
  void setEditResultVisible(bool visible);
  std::unique_ptr<IndirectFitDataPresenter> m_dataPresenter;
  std::unique_ptr<IndirectFitPlotPresenter> m_plotPresenter;
  std::unique_ptr<IndirectFittingModel> m_fittingModel;
  IndirectFitPropertyBrowser *m_fitPropertyBrowser{nullptr};
  WorkspaceID m_activeWorkspaceID;
  WorkspaceIndex m_activeSpectrumIndex;

  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;

private:
  void setup() override;
  bool validate() override;
  void connectFitPropertyBrowser();
  void plotSelectedSpectra(std::vector<SpectrumToPlot> const &spectra);
  void plotSpectrum(std::string const &workspaceName, std::size_t const &index);
  std::string getOutputBasename() const;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  std::vector<std::string> getFitParameterNames() const;
  QList<MantidWidgets::FunctionModelDataset> getDatasets() const;
  void enableFitButtons(bool enable);
  void enableOutputOptions(bool enable);
  void setPDFWorkspace(std::string const &workspaceName);
  void updateParameterEstimationData();
  std::string getFitTypeString() const;

  std::string m_tabName;
  bool m_hasResolution;

  std::unique_ptr<IndirectFitOutputOptionsPresenter> m_outOptionsPresenter;
  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;

protected slots:
  void setModelFitFunction();
  void setModelStartX(double startX);
  void setModelEndX(double endX);
  void updateFitOutput(bool error);
  void updateSingleFitOutput(bool error);
  void fitAlgorithmComplete(bool error);
  void singleFit();
  void executeFit();
  void updateParameterValues();
  void updateParameterValues(const std::unordered_map<std::string, ParameterValue> &parameters);
  void updateFitBrowserParameterValues(const std::unordered_map<std::string, ParameterValue> &parameters =
                                           std::unordered_map<std::string, ParameterValue>());
  void updateFitBrowserParameterValuesFromAlg();
  void updateFitStatus();
  void updateDataReferences();
  void updateResultOptions();
  void respondToFunctionChanged();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
