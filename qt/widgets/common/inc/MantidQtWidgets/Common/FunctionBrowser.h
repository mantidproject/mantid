// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONBROWSER_H_
#define MANTIDWIDGETS_FUNCTIONBROWSER_H_

#include "DllOption.h"

#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/IFunctionBrowser.h"

#include <QMap>
#include <QWidget>

#include <boost/optional.hpp>
#include <memory>

namespace Mantid {
namespace API {
class CompositeFunction;
class Workspace;
class ParameterTie;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

class FunctionTreeView;
class FunctionMultiDomainPresenter;

using namespace Mantid::API;

/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display
 * and control fitting function parameters and settings.
 */
class EXPORT_OPT_MANTIDQT_COMMON FunctionBrowser : public QWidget,
                                                   public IFunctionBrowser {
  Q_OBJECT
public:
  /// Constructor
  FunctionBrowser(QWidget *parent = nullptr, bool multi = false, const std::vector<std::string>& categories = std::vector<std::string>());
  /// Destructor
  virtual ~FunctionBrowser() override;
  /// Clear the contents
  void clear() override;
  /// Set the function in the browser
  void setFunction(const QString &funStr) override;
  /// Set the function in the browser
  void setFunction(IFunction_sptr fun);
  /// Return FunctionFactory function string
  QString getFunctionString() override;
  /// Return the function
  IFunction_sptr getFunction();
  /// Check if a function is set
  bool hasFunction() const;
  /// Return a function with specified index
  IFunction_sptr getFunctionByIndex(const QString &index);
  /// Return index of the current function, if one is selected
  boost::optional<QString> currentFunctionIndex();
  /// Update the function parameter value
  void setParameter(const QString &paramName, double value);
  /// Update the function parameter error
  void setParamError(const QString &paramName, double error);
  /// Get a value of a parameter
  double getParameter(const QString &paramName) const;
  /// Update parameter values in the browser to match those of a function.
  void updateParameters(const IFunction &fun) override;
  /// Get a list of names of global parameters
  QStringList getGlobalParameters() const;
  void setGlobalParameters(QStringList &globals);
  /// Get a list of names of local parameters
  QStringList getLocalParameters() const;
  /// Get the number of datasets
  int getNumberOfDatasets() const override;
  /// Get value of a local parameter
  double getLocalParameterValue(const QString &parName, int i) const override;
  /// Set value of a local parameter
  void setLocalParameterValue(const QString &parName, int i,
                              double value) override;
  /// Set value and error of a local parameter
  void setLocalParameterValue(const QString &parName, int i, double value,
                              double error);
  /// Get error of a local parameter
  double getLocalParameterError(const QString &parName, int i) const;
  /// Check if a local parameter is fixed
  bool isLocalParameterFixed(const QString &parName, int i) const override;
  /// Fix/unfix local parameter
  void setLocalParameterFixed(const QString &parName, int i,
                              bool fixed) override;
  /// Get the tie for a local parameter.
  QString getLocalParameterTie(const QString &parName, int i) const override;
  /// Set a tie for a local parameter.
  void setLocalParameterTie(const QString &parName, int i,
                            QString tie) override;
  /// Return the multidomain function if number of datasets is greater than 1
  IFunction_sptr getGlobalFunction() override;
  /// Update parameter values in the browser to match those of a function.
  void updateMultiDatasetParameters(const IFunction &fun) override;
  /// Get the index of the current dataset.
  int getCurrentDataset() const override;
  /// Resize the browser's columns
  void setColumnSizes(int s0, int s1, int s2 = -1);
  /// Set error display on/off
  void setErrorsEnabled(bool enabled) override;
  /// Clear all errors
  void clearErrors() override;

signals:
  void parameterChanged(const QString &funcIndex,
                        const QString &paramName);
  void functionStructureChanged();
  /// User selects a different function (or one of it's sub-properties)
  void currentFunctionChanged();

  /// In multi-dataset context a button value editor was clicked
  void localParameterButtonClicked(const QString &parName);
  void globalsChanged();
  void constraintsChanged();
  void tiesChanged();

public slots:

  // Handling of multiple datasets
  void setNumberOfDatasets(int n) override;
  void resetLocalParameters();
  void setCurrentDataset(int i) override;
  void removeDatasets(QList<int> indices);
  void addDatasets(int n);
  //void editLocalParameter(const QString &parName, const QStringList &wsNames,
  //                        const std::vector<size_t> &wsIndices) override;

protected:
  std::unique_ptr<FunctionMultiDomainPresenter> m_presenter;
public:
  // Intended for testing only
  FunctionTreeView *view() const;
  QString getFitFunctionString() const;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDWIDGETS_FUNCTIONBROWSER_H_*/
