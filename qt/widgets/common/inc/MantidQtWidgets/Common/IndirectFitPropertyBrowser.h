// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECTFITPROPERTYBROWSER_H_
#define INDIRECTFITPROPERTYBROWSER_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QDockWidget>

#include <QSet>

#include <boost/optional.hpp>

#include <unordered_map>

namespace MantidQt {
namespace MantidWidgets {

class FunctionBrowser;
class FitOptionsBrowser;

class EXPORT_OPT_MANTIDQT_COMMON IndirectFitPropertyBrowser
    : public QDockWidget {
  Q_OBJECT

public:
  /// Constructor.
  IndirectFitPropertyBrowser(QWidget *parent = nullptr,
                             QObject *mantidui = nullptr);
  /// Initialise the layout.
  void init();

  Q_INVOKABLE void setFunction(const QString &funStr);
  Mantid::API::IFunction_sptr getFittingFunction() const;
  Mantid::API::IFunction_sptr compositeFunction() const;
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
  QString selectedFitType() const;
  void setConvolveMembers(bool convolveMembers);
  void setFitEnabled(bool enable);
  void setWorkspaceIndex(int i);
  int workspaceIndex() const;
  void updateFunctionBrowserData(size_t nData);
  void editLocalParameter(const QString &parName, const QStringList &wsNames,
                          const std::vector<size_t> &wsIndices);
  void updatePlotGuess(Mantid::API::MatrixWorkspace_const_sptr sampleWorkspace);

public slots:
  void fit();
  void sequentialFit();

protected slots:
  void clear();
  void browserVisibilityChanged(bool isVisible);


signals:
  void functionChanged();
  void fitScheduled();
  void sequentialFitScheduled();
  void browserClosed();
  void localParameterEditRequested(const QString &parName);

private:
  void initFunctionBrowser();
  void iniFitOptionsBrowser();
  
  FunctionBrowser *m_functionBrowser;
  FitOptionsBrowser *m_fitOptionsBrowser;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INDIRECTFITPROPERTYBROWSER_H_*/
