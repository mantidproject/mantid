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
#include <boost/optional.hpp>
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
  virtual ~IFunctionView() {}
  virtual void clear() = 0;
  virtual void setFunction(IFunction_sptr fun) = 0;
  virtual bool hasFunction() const = 0;
  virtual IFunction_sptr getSelectedFunction() = 0;
  virtual void setParameter(const QString &paramName, double value) = 0;
  virtual void setParameterError(const QString &paramName, double error) = 0;
  virtual double getParameter(const QString &paramName) const = 0;
  virtual IFunction::Attribute getAttribute(const QString &attrName) const = 0;
  virtual void setErrorsEnabled(bool enabled) = 0;
  virtual void clearErrors() = 0;
  virtual boost::optional<QString> currentFunctionIndex() const = 0;
  virtual void setParameterTie(const QString &paramName, const QString &tie) = 0;
  virtual void setParameterConstraint(const QString &paramName, const QString &constraint) = 0;
  virtual void setGlobalParameters(const QStringList &) = 0;
  virtual void showFunctionHelp(const QString &) const = 0;
  // Set the value of an attribute based on the template type
  template <typename T> void setAttributeValue(const QString &attributeName, T &value) {
    if constexpr (std::is_same_v<T, double>) {
      setDoubleAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, int>) {
      setIntAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, QString>) {
      setStringAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, bool>) {
      setBooleanAttribute(attributeName, value);
    } else if constexpr (std::is_same_v<T, std::vector<double>>) {
      setVectorAttribute(attributeName, value);
    }
  }

protected:
  virtual void setDoubleAttribute(const QString &paramName, double value) = 0;
  virtual void setIntAttribute(const QString &paramName, int value) = 0;
  virtual void setStringAttribute(const QString &paramName, std::string &value) = 0;
  virtual void setBooleanAttribute(const QString &paramName, bool value) = 0;
  virtual void setVectorAttribute(const QString &paramName, std::vector<double> &val) = 0;

signals:
  /// User replaces the whole function (eg, by pasting it from clipboard)
  void functionReplaced(const QString &funStr);
  /// User adds a function
  void functionAdded(const QString &funStr);
  /// User removes a function
  void functionRemoved(const QString &functionIndex);
  /// User removes a function
  void functionRemovedString(const QString &funStr);
  /// User selects a different (sub)function (or one of it's sub-properties)
  void currentFunctionChanged();
  /// Function parameter gets changed
  void parameterChanged(const QString &paramName);
  /// Function attribute gets changed
  void attributePropertyChanged(const QString &attrName);
  /// In multi-dataset context a button value editor was clicked
  void localParameterButtonClicked(const QString &parName);
  /// User sets a tie
  void parameterTieChanged(const QString &parName, const QString &tie);
  /// User sets a constraint
  void parameterConstraintAdded(const QString &functionIndex, const QString &constraint);
  /// User removes a constraint
  void parameterConstraintRemoved(const QString &paramName);
  /// User requested copy function to clipboard
  void copyToClipboardRequest();
  /// User requested function help
  void functionHelpRequest();
  /// User changed the list of global parameters.
  void globalsChanged(const QStringList &);
};

} // namespace MantidWidgets
} // namespace MantidQt
