// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "FunctionBrowser/ITemplatePresenter.h"
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

#include <boost/optional.hpp>
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

class MANTIDQT_INELASTIC_DLL IInelasticFitPropertyBrowser {
public:
  virtual ~IInelasticFitPropertyBrowser() = default;
  virtual void updateFunctionListInBrowser(const std::map<std::string, std::string> &functionStrings) = 0;
};
class MANTIDQT_INELASTIC_DLL InelasticFitPropertyBrowser : public QDockWidget, public IInelasticFitPropertyBrowser {
  Q_OBJECT

public:
  InelasticFitPropertyBrowser(QWidget *parent = nullptr);

  void init();
  void setFunctionTemplatePresenter(std::unique_ptr<ITemplatePresenter> templatePresenter);
  void setFunction(std::string const &funStr);
  int getNumberOfDatasets() const;
  QString getSingleFunctionStr() const;
  MultiDomainFunction_sptr getFitFunction() const;
  std::string minimizer(bool withProperties = false) const;
  int maxIterations() const;
  int getPeakRadius() const;
  std::string costFunction() const;
  bool convolveMembers() const;
  bool outputCompositeMembers() const;
  std::string fitEvaluationType() const;
  std::string fitType() const;
  bool ignoreInvalidData() const;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> fitProperties(FittingMode const &fittingMode) const;
  void updateParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const ITableWorkspace &params);
  void updateFitStatus(const FitDomainIndex index);
  void updateFitStatusData(const std::vector<std::string> &status, const std::vector<double> &chiSquared);
  FittingMode getFittingMode() const;
  void setConvolveMembers(bool convolveEnabled);
  void setOutputCompositeMembers(bool outputEnabled);
  void setFitEnabled(bool enable);
  void setCurrentDataset(FitDomainIndex i);
  FitDomainIndex currentDataset() const;
  void updateFunctionBrowserData(int nData, const QList<MantidWidgets::FunctionModelDataset> &datasets,
                                 const std::vector<double> &qValues,
                                 const std::vector<std::pair<std::string, size_t>> &fitResolutions);
  void updatePlotGuess(const MatrixWorkspace_const_sptr &sampleWorkspace);
  void setErrorsEnabled(bool enabled);
  EstimationDataSelector getEstimationDataSelector() const;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters();
  void setBackgroundA0(double value);
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
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
