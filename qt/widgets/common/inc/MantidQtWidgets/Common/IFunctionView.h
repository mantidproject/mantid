// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/IFunction.h"

#include <QString>
#include <QWidget>
#include <optional>
#include <type_traits>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

/**
 * The interface to a function view.
 */
class EXPORT_OPT_MANTIDQT_COMMON IFunctionView : public QWidget {
  Q_OBJECT
public:
  IFunctionView(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~IFunctionView() = default;
  virtual void clear() = 0;
  virtual void setFunction(IFunction_sptr fun) = 0;
  virtual bool hasFunction() const = 0;
  virtual IFunction_sptr getSelectedFunction() = 0;
  virtual void setParameter(std::string const &parameterName, double value) = 0;
  virtual void setParameterError(std::string const &parameterName, double error) = 0;
  virtual double getParameter(std::string const &parameterName) const = 0;
  virtual IFunction::Attribute getAttribute(std::string const &attrName) const = 0;
  virtual void setErrorsEnabled(bool enabled) = 0;
  virtual void clearErrors() = 0;
  virtual std::optional<std::string> currentFunctionIndex() const = 0;
  virtual void setParameterTie(std::string const &parameterName, std::string const &tie) = 0;
  virtual void setParameterConstraint(std::string const &parameterName, std::string const &constraint) = 0;
  virtual void setGlobalParameters(const std::vector<std::string> &) = 0;
  virtual void showFunctionHelp(std::string const &) const = 0;
  // Set the value of an attribute based on the template type
  template <typename T> void setAttributeValue(std::string const &attributeName, T &value) {
    if constexpr (std::is_same_v<T, double>) {
      setDoubleAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, int>) {
      setIntAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      setStringAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, bool>) {
      setBooleanAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, std::vector<double>>) {
      setVectorAttribute(attributeName, value);
    }
  }

protected:
  virtual void setDoubleAttribute(std::string const &parameterName, double value) = 0;
  virtual void setIntAttribute(std::string const &parameterName, int value) = 0;
  virtual void setStringAttribute(std::string const &parameterName, std::string &value) = 0;
  virtual void setBooleanAttribute(std::string const &parameterName, bool value) = 0;
  virtual void setVectorAttribute(std::string const &parameterName, std::vector<double> &val) = 0;

signals:
  /// User replaces the whole function (eg, by pasting it from clipboard)
  void functionReplaced(std::string const &funStr);
  /// User adds a function
  void functionAdded(std::string const &funStr);
  /// User removes a function
  void functionRemoved(std::string const &functionIndex);
  /// User removes a function
  void functionRemovedString(std::string const &funStr);
  /// User selects a different (sub)function (or one of it's sub-properties)
  void currentFunctionChanged();
  /// Function parameter gets changed
  void parameterChanged(std::string const &parameterName);
  /// Function attribute gets changed
  void attributePropertyChanged(std::string const &attrName);
  /// In multi-dataset context a button value editor was clicked
  void localParameterButtonClicked(std::string const &parameterName);
  /// User sets a tie
  void parameterTieChanged(std::string const &parameterName, std::string const &tie);
  /// User sets a constraint
  void parameterConstraintAdded(std::string const &functionIndex, std::string const &constraint);
  /// User removes a constraint
  void parameterConstraintRemoved(std::string const &parameterName);
  /// User requested copy function to clipboard
  void copyToClipboardRequest();
  /// User requested function help
  void functionHelpRequest();
  /// User changed the list of global parameters.
  void globalsChanged(const std::vector<std::string> &);
};

} // namespace MantidWidgets
} // namespace MantidQt
