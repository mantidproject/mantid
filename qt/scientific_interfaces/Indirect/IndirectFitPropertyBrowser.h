// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECTFITPROPERTYBROWSER_H_
#define INDIRECTFITPROPERTYBROWSER_H_

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <QDockWidget>

#include <boost/optional.hpp>
#include <unordered_map>

class QVBoxLayout;
class QStackedWidget;

namespace MantidQt {
namespace MantidWidgets {
  class FunctionBrowser;
  class FitOptionsBrowser;
}
namespace CustomInterfaces {
namespace IDA {

class FunctionTemplateBrowser;

class MANTIDQT_INDIRECT_DLL IndirectFitPropertyBrowser
  : public QDockWidget {
  Q_OBJECT

public:
  /// Constructor.
  IndirectFitPropertyBrowser(QWidget *parent = nullptr);
  /// Initialise the layout.
  void init();
  /// Set the browser for the function template
  void setFunctionTemplateBrowser(FunctionTemplateBrowser *templateBrowser);

  /// Set the function to the browser
  Q_INVOKABLE void setFunction(const QString &funStr);
  /// Get the number of datasets to fit to
  Q_INVOKABLE int getNumberOfDatasets() const;
  /// Get the string for a single-domain function currently displayed in the function browser
  Q_INVOKABLE QString getSingleFunctionStr() const;

  /// Get the global multi-domain function. Even if there is only 1 dataset.
  Mantid::API::MultiDomainFunction_sptr getFittingFunction() const;
  /// Get the minimizer
  std::string minimizer(bool withProperties = false) const;
  /// Get the max number of iterations
  int maxIterations() const;
  /// Get the peak radius for peak functions
  int getPeakRadius() const;
  /// Get the cost function
  std::string costFunction() const;
  /// Get the "ConvolveMembers" option
  bool convolveMembers() const;
  /// Get "HistogramFit" option
  bool isHistogramFit() const;
  /// Get the ignore invalid data option
  bool ignoreInvalidData() const;

  void updateParameters(const Mantid::API::IFunction &fun);
  void updateMultiDatasetParameters(const Mantid::API::IFunction &fun);
  void updateMultiDatasetParameters(const Mantid::API::IFunction & fun, const Mantid::API::ITableWorkspace &params);
  QString selectedFitType() const;
  void setConvolveMembers(bool convolveMembers);
  void setFitEnabled(bool enable);
  void setWorkspaceIndex(int i);
  int workspaceIndex() const;
  void updateFunctionBrowserData(size_t nData, const QStringList &datasetNames);
  void updatePlotGuess(Mantid::API::MatrixWorkspace_const_sptr sampleWorkspace);

public slots:
  void fit();
  void sequentialFit();

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

  QVBoxLayout *m_mainLayout;
  MantidWidgets::FunctionBrowser *m_functionBrowser;
  MantidWidgets::FitOptionsBrowser *m_fitOptionsBrowser;
  FunctionTemplateBrowser *m_templateBrowser;
  QStackedWidget *m_functionWidget;

};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*INDIRECTFITPROPERTYBROWSER_H_*/
