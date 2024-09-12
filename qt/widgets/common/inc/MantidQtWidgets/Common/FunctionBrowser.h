// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IFunctionBrowser.h"

#include <QWidget>

#include <memory>
#include <optional>

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
class EXPORT_OPT_MANTIDQT_COMMON FunctionBrowser : public QWidget, public IFunctionBrowser {
  Q_OBJECT
public:
  /// Constructor
  FunctionBrowser(QWidget *parent = nullptr, bool multi = false,
                  const std::vector<std::string> &categories = std::vector<std::string>());
  /// Destructor
  virtual ~FunctionBrowser() override;
  /// Clear the contents
  void clear() override;
  /// Set the function in the browser
  void setFunction(std::string const &funStr) override;
  /// Set the function in the browser
  void setFunction(IFunction_sptr fun);
  /// Return FunctionFactory function string
  std::string getFunctionString() override;
  /// Return the function
  IFunction_sptr getFunction();
  /// Check if a function is set
  bool hasFunction() const;
  /// Return a function with specified index
  IFunction_sptr getFunctionByIndex(std::string const &index);
  /// Return index of the current function, if one is selected
  std::optional<std::string> currentFunctionIndex();
  /// Update the function parameter value
  void setParameter(std::string const &parameterName, double value);
  /// Update the function parameter error
  void setParameterError(std::string const &parameterName, double error);
  /// Get a value of a parameter
  double getParameter(std::string const &parameterName) const;
  /// Update parameter values in the browser to match those of a function.
  void updateParameters(const IFunction &fun) override;
  /// Get a list of names of global parameters
  std::vector<std::string> getGlobalParameters() const;
  void setGlobalParameters(std::vector<std::string> const &globals);
  /// Get a list of names of local parameters
  std::vector<std::string> getLocalParameters() const;
  /// Get the number of datasets
  int getNumberOfDatasets() const override;
  /// Get the names of datasets
  std::vector<std::string> getDatasetNames() const override;
  /// Get the names of the dataset domains
  std::vector<std::string> getDatasetDomainNames() const override;
  /// Get value of a local parameter
  double getLocalParameterValue(std::string const &parameterName, int i) const override;
  /// Set value of a local parameter
  void setLocalParameterValue(std::string const &parameterName, int i, double value) override;
  /// Set value and error of a local parameter
  void setLocalParameterValue(std::string const &parameterName, int i, double value, double error);
  /// Get error of a local parameter
  double getLocalParameterError(std::string const &parameterName, int i) const;
  /// Check if a local parameter is fixed
  bool isLocalParameterFixed(std::string const &parameterName, int i) const override;
  /// Fix/unfix local parameter
  void setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) override;
  /// Get the tie for a local parameter.
  std::string getLocalParameterTie(std::string const &parameterName, int i) const override;
  /// Set a tie for a local parameter.
  void setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) override;
  /// Return the multidomain function if number of datasets is greater than 1
  IFunction_sptr getGlobalFunction() override;
  /// Update parameter values in the browser to match those of a function.
  void updateMultiDatasetParameters(const IFunction &fun) override;
  /// Update parameter values in the browser to match those of a function.
  void updateMultiDatasetAttributes(const IFunction &fun);
  /// Update parameter values in the browser to match those in a table
  /// workspace.
  void updateMultiDatasetParameters(const ITableWorkspace &paramTable) override;
  /// Get the index of the current dataset.
  int getCurrentDataset() const override;
  /// Resize the browser's columns
  void setColumnSizes(int s0, int s1, int s2 = -1);
  /// Set the last column to stretch.
  void setStretchLastColumn(bool stretch);
  /// Set error display on/off
  void setErrorsEnabled(bool enabled) override;
  /// Clear all errors
  void clearErrors() override;
  /// Set a parameter that is responsible for the background level
  void setBackgroundA0(double value);
  // hide the global options
  void hideGlobalCheckbox();
  // show the global options
  void showGlobalCheckbox();

signals:
  void parameterChanged(std::string const &funcIndex, std::string const &paramName);
  void attributeChanged(std::string const &attributeName);
  void functionStructureChanged();
  /// User selects a different function (or one of it's sub-properties)
  void currentFunctionChanged();

  /// In multi-dataset context a button value editor was clicked
  void localParameterButtonClicked(std::string const &parName);
  void globalsChanged();

public slots:

  // Handling of multiple datasets
  void setNumberOfDatasets(int n) override;
  void setDatasets(const std::vector<std::string> &datasetNames) override;
  void setDatasets(const QList<FunctionModelDataset> &datasets) override;
  void resetLocalParameters();
  void setCurrentDataset(int i) override;
  void removeDatasets(const QList<int> &indices);
  void addDatasets(const std::vector<std::string> &names);

protected:
  std::unique_ptr<FunctionMultiDomainPresenter> m_presenter;

public:
  // Intended for testing only
  FunctionTreeView *view() const;
  std::string getFitFunctionString() const;
};

} // namespace MantidWidgets
} // namespace MantidQt
