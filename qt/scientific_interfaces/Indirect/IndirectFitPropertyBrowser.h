// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECTFITPROPERTYBROWSER_H_
#define INDIRECTFITPROPERTYBROWSER_H_

#include "DllConfig.h"
#include "IndexTypes.h"
#include "IndirectFittingModel.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QDockWidget>

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
  bool isHistogramFit() const;
  bool ignoreInvalidData() const;
  void updateParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const IFunction &fun);
  void updateMultiDatasetParameters(const ITableWorkspace &params);
  QString selectedFitType() const;
  void setConvolveMembers(bool convolveMembers);
  void setFitEnabled(bool enable);
  void setCurrentDataset(SpectrumRowIndex i);
  SpectrumRowIndex currentDataset() const;
  void updateFunctionBrowserData(SpectrumRowIndex nData,
                                 const QStringList &datasetNames);
  void updatePlotGuess(MatrixWorkspace_const_sptr sampleWorkspace);
  void setErrorsEnabled(bool enabled);
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void setBackgroundA0(double value);

public slots:
  void fit();
  void sequentialFit();
  void setModelResolution(std::string const &name, DatasetIndex const &index);

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
  QStackedWidget *m_functionWidget;
  QCheckBox *m_browserSwitcher;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECTFITPROPERTYBROWSER_H_*/
