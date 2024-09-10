// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "FunctionBrowser/ITemplatePresenter.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "ParameterEstimation.h"

#include <QDockWidget>
#include <QList>
#include <QPair>
#include <QString>

#include <optional>
#include <unordered_map>

class QCheckBox;
class QVBoxLayout;
class QStackedWidget;

namespace MantidQt {
namespace MantidWidgets {
class FunctionBrowser;
class FitOptionsBrowser;
} // namespace MantidWidgets
namespace CustomInterfaces {
namespace Inelastic {

using namespace Mantid::API;
using namespace MantidWidgets;

class FunctionTemplateView;
class FitStatusWidget;
class IFittingPresenter;

class MANTIDQT_INELASTIC_DLL IInelasticFitPropertyBrowser {
public:
  virtual ~IInelasticFitPropertyBrowser() = default;

  virtual void subscribePresenter(IFittingPresenter *presenter) = 0;

  virtual MultiDomainFunction_sptr getFitFunction() const = 0;
  virtual std::string minimizer(bool withProperties = false) const = 0;

  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> fitProperties(FittingMode const &fittingMode) const = 0;

  virtual void setFitEnabled(bool enable) = 0;
  virtual void setCurrentDataset(FitDomainIndex i) = 0;
  virtual void setErrorsEnabled(bool enabled) = 0;
  virtual void setBackgroundA0(double value) = 0;

  virtual EstimationDataSelector getEstimationDataSelector() const = 0;
  virtual void updateParameterEstimationData(DataForParameterEstimationCollection &&data) = 0;
  virtual void estimateFunctionParameters() = 0;

  virtual FittingMode getFittingMode() const = 0;

  virtual void updateParameters(const IFunction &fun) = 0;
  virtual void updateMultiDatasetParameters(const IFunction &fun) = 0;
  virtual void updateMultiDatasetParameters(const ITableWorkspace &params) = 0;

  virtual void updateFunctionListInBrowser(const std::map<std::string, std::string> &functionStrings) = 0;
  virtual void updateFunctionBrowserData(int nData, const QList<MantidWidgets::FunctionModelDataset> &datasets,
                                         const std::vector<double> &qValues,
                                         const std::vector<std::pair<std::string, size_t>> &fitResolutions) = 0;
  virtual void updateFitStatusData(const std::vector<std::string> &status, const std::vector<double> &chiSquared) = 0;
};
class MANTIDQT_INELASTIC_DLL InelasticFitPropertyBrowser : public QDockWidget, public IInelasticFitPropertyBrowser {
  Q_OBJECT

public:
  InelasticFitPropertyBrowser(QWidget *parent = nullptr);

  void subscribePresenter(IFittingPresenter *presenter) override;

  void init();
  void setFunctionTemplatePresenter(std::unique_ptr<ITemplatePresenter> templatePresenter);
  void setFunction(std::string const &funStr);
  int getNumberOfDatasets() const;
  QString getSingleFunctionStr() const;
  MultiDomainFunction_sptr getFitFunction() const override;
  std::string minimizer(bool withProperties = false) const override;
  int maxIterations() const;
  int getPeakRadius() const;
  std::string costFunction() const;
  bool convolveMembers() const;
  bool outputCompositeMembers() const;
  std::string fitEvaluationType() const;
  std::string fitType() const;
  bool ignoreInvalidData() const;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> fitProperties(FittingMode const &fittingMode) const override;
  void updateParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const IFunction &fun) override;
  void updateMultiDatasetParameters(const ITableWorkspace &params) override;
  void updateFitStatus(const FitDomainIndex index);
  void updateFitStatusData(const std::vector<std::string> &status, const std::vector<double> &chiSquared) override;
  FittingMode getFittingMode() const override;
  void setConvolveMembers(bool convolveEnabled);
  void setOutputCompositeMembers(bool outputEnabled);
  void setFitEnabled(bool enable) override;
  void setCurrentDataset(FitDomainIndex i) override;
  FitDomainIndex currentDataset() const;
  void updateFunctionBrowserData(int nData, const QList<MantidWidgets::FunctionModelDataset> &datasets,
                                 const std::vector<double> &qValues,
                                 const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  void updatePlotGuess(const MatrixWorkspace_const_sptr &sampleWorkspace);
  void setErrorsEnabled(bool enabled) override;
  EstimationDataSelector getEstimationDataSelector() const override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;
  void setBackgroundA0(double value) override;
  void setHiddenProperties(const std::vector<std::string> &);
  void updateFunctionListInBrowser(const std::map<std::string, std::string> &functionStrings) override;

public slots:
  void fit();
  void sequentialFit();
  void setModelResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions);

protected slots:
  void clear();
  void browserVisibilityChanged(bool isVisible);
  void updateFitType();
  void showFullFunctionBrowser(bool on);

private slots:
  void notifyFunctionChanged();

signals:
  void functionChanged();
  void fitScheduled();
  void sequentialFitScheduled();
  void browserClosed();
  void localParameterEditRequested(std::string const &parameterName);
  void globalsChanged(int n);

private:
  void initFunctionBrowser();
  void initFitOptionsBrowser();
  bool isFullFunctionBrowserActive() const;
  MultiDomainFunction_sptr getGlobalFunction() const;
  IFunction_sptr getSingleFunction() const;
  std::vector<std::string> getGlobalParameters() const;
  std::vector<std::string> getLocalParameters() const;
  void syncFullBrowserWithTemplate();
  void syncTemplateBrowserWithFull();

  QVBoxLayout *m_mainLayout;
  FunctionBrowser *m_functionBrowser;
  FitOptionsBrowser *m_fitOptionsBrowser;
  std::unique_ptr<ITemplatePresenter> m_templatePresenter;
  FitStatusWidget *m_fitStatusWidget;
  QStackedWidget *m_functionWidget;
  QCheckBox *m_browserSwitcher;

  std::vector<std::string> m_fitStatus;
  std::vector<double> m_fitChiSquared;

  IFittingPresenter *m_presenter;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
