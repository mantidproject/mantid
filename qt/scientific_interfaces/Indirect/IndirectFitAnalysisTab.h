// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IIndirectFitDataModel.h"
#include "IndirectDataAnalysisTab.h"
#include "IndirectFitDataModel.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFitOutputOptionsPresenter.h"
#include "IndirectFitOutputOptionsView.h"
#include "IndirectFitPlotPresenter.h"
#include "IndirectFitPropertyBrowser.h"
#include "IndirectFittingModel.h"

#include "MantidQtWidgets/Common/FunctionModelDataset.h"

#include <boost/optional.hpp>

#include <QtCore>

#include <memory>
#include <type_traits>

#include <QList>
#include <QPair>
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

size_t getNumberOfSpecificFunctionContained(const std::string &functionName, const IFunction *compositeFunction);

class MANTIDQT_INDIRECT_DLL IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectFitAnalysisTab(IndirectFittingModel *model, QWidget *parent = nullptr);
  virtual ~IndirectFitAnalysisTab() override = default;

  void setFitDataPresenter(std::unique_ptr<IndirectFitDataPresenter> presenter);
  void setPlotView(IIndirectFitPlotView *view);
  void setOutputOptionsView(IIndirectFitOutputOptionsView *view);
  void setFitPropertyBrowser(IndirectFitPropertyBrowser *browser);
  WorkspaceID getSelectedDataIndex() const;
  WorkspaceIndex getSelectedSpectrum() const;
  bool isRangeCurrentlySelected(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  size_t getNumberOfCustomFunctions(const std::string &functionName) const;
  void setConvolveMembers(bool convolveMembers);

  static size_t getNumberOfSpecificFunctionContained(const std::string &functionName,
                                                     const IFunction *compositeFunction);

  virtual std::string getTabName() const = 0;
  virtual bool hasResolution() const = 0;

protected:
  IndirectFittingModel *getFittingModel() const;
  void run() override;
  void setSampleWSSuffixes(const QStringList &suffices);
  void setSampleFBSuffixes(const QStringList &suffices);
  void setResolutionWSSuffixes(const QStringList &suffices);
  void setResolutionFBSuffixes(const QStringList &suffices);
  void setFileExtensionsByName(bool filter) override;
  void setSampleSuffixes(std::string const &tab, bool filter);
  void setResolutionSuffixes(std::string const &tab, bool filter);

  void setAlgorithmProperties(const Mantid::API::IAlgorithm_sptr &fitAlgorithm) const;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void runSingleFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  virtual void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  virtual void setRunIsRunning(bool running) = 0;
  virtual void setRunEnabled(bool enable) = 0;
  void setEditResultVisible(bool visible);
  std::unique_ptr<IndirectFitDataPresenter> m_dataPresenter;
  std::unique_ptr<IndirectFitPlotPresenter> m_plotPresenter;
  IndirectFitPropertyBrowser *m_fitPropertyBrowser{nullptr};
  WorkspaceID m_activeWorkspaceID;
  WorkspaceIndex m_activeSpectrumIndex;

private:
  void setup() override;
  bool validate() override;
  virtual void setupFitTab() = 0;
  virtual EstimationDataSelector getEstimationDataSelector() const = 0;
  void connectPlotPresenter();
  void connectFitPropertyBrowser();
  void connectDataPresenter();
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
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog) = 0;

  std::unique_ptr<IndirectFittingModel> m_fittingModel;
  std::unique_ptr<IndirectFitOutputOptionsPresenter> m_outOptionsPresenter;
  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;

protected slots:
  void setModelFitFunction();
  void setModelStartX(double startX);
  void setModelEndX(double endX);
  void updateDataInTable();
  void tableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum);
  void tableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum);
  void startXChanged(double startX);
  void endXChanged(double endX);
  void updateFitOutput(bool error);
  void updateSingleFitOutput(bool error);
  void fitAlgorithmComplete(bool error);
  void singleFit();
  void singleFit(WorkspaceID workspaceID, WorkspaceIndex spectrum);
  void executeFit();
  void updateParameterValues();
  void updateParameterValues(const std::unordered_map<std::string, ParameterValue> &parameters);
  void updateFitBrowserParameterValues(
      std::unordered_map<std::string, ParameterValue> parameters = std::unordered_map<std::string, ParameterValue>());
  void updateFitBrowserParameterValuesFromAlg();
  void updateFitStatus();
  void updateDataReferences();
  void updateResultOptions();
  void respondToFunctionChanged();

private slots:
  void plotSelectedSpectra();
  void respondToSingleResolutionLoaded();
  void respondToDataChanged();
  void respondToDataAdded(IAddWorkspaceDialog const *dialog);
  void respondToDataRemoved();
  void respondToPlotSpectrumChanged();
  void respondToFwhmChanged(double);
  void respondToBackgroundChanged(double value);

signals:
  void functionChanged();
  void parameterChanged(const Mantid::API::IFunction *fun);
  void customBoolChanged(const QString &key, bool value);
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
