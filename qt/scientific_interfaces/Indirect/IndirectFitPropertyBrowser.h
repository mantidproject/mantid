// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectFittingModel.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

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
namespace IDA {

using namespace Mantid::API;
using namespace MantidWidgets;

class FunctionTemplateBrowser;
class FitStatusWidget;

class MANTIDQT_INDIRECT_DLL IndirectFitPropertyBrowser : public QDockWidget {
  Q_OBJECT

public:
  IndirectFitPropertyBrowser(QWidget *parent = nullptr);

  void init();
  void setFunctionTemplateBrowser(FunctionTemplateBrowser *templateBrowser);
  void setFunction(const QString &funStr);
  int getNumberOfDatasets() const;
  QString getSingleFunctionStr() const;
  MultiDomainFunction_sptr getFittingFunction() const;
  std::string minimizer(bool withProperties = false) const;
  int maxIterations() const;
  int getPeakRadius() const;
  std::string costFunction() const;
  bool convolveMembers() const;
  bool outputCompositeMembers() const;
  std::string fitEvaluationType() const;
  std::string fitType() const;
  bool ignoreInvalidData() const;
  void updateParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const ITableWorkspace &params);
  void updateFitStatusData(const std::vector<std::string> &status, const std::vector<double> &chiSquared);
  void updateFitStatus(const FitDomainIndex index);
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
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters();
  void setBackgroundA0(double value);
  void setHiddenProperties(std::vector<std::string>);

public slots:
  void fit();
  void sequentialFit();
  void setModelResolution(std::string const &name, TableDatasetIndex const &index);
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
  void localParameterEditRequested(const QString &parName);
  void globalsChanged(int n);

private:
  void initFunctionBrowser();
  void initFitOptionsBrowser();
  bool isFullFunctionBrowserActive() const;
  MultiDomainFunction_sptr getGlobalFunction() const;
  IFunction_sptr getSingleFunction() const;
  QStringList getGlobalParameters() const;
  QStringList getLocalParameters() const;
  void syncFullBrowserWithTemplate();
  void syncTemplateBrowserWithFull();

  QVBoxLayout *m_mainLayout;
  FunctionBrowser *m_functionBrowser;
  FitOptionsBrowser *m_fitOptionsBrowser;
  FunctionTemplateBrowser *m_templateBrowser;
  FitStatusWidget *m_fitStatusWidget;
  QStackedWidget *m_functionWidget;
  QCheckBox *m_browserSwitcher;

  std::vector<std::string> m_fitStatus;
  std::vector<double> m_fitChiSquared;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
