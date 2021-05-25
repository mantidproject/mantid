// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FittingMode.h"

#include <QMap>
#include <QWidget>

class QtProperty;
class QtTreePropertyBrowser;
class QtIntPropertyManager;
class QtEnumPropertyManager;

namespace MantidQt {
namespace MantidWidgets {

/**
 * The BasicFitOptionsBrowser class implements a QtPropertyBrowser to display
 * fitting properties found in the Fit algorithm. It only shows the most
 * essential fit properties such as the FittingMode, Max Iterations, Minimizer,
 * Evaluation Type and Cost Function. The other fit properties are not
 * displayed in order to avoid the browser becoming cluttered with rarely used
 * options.
 */
class EXPORT_OPT_MANTIDQT_COMMON BasicFitOptionsBrowser : public QWidget {
  Q_OBJECT

  using PropertySetter = void (BasicFitOptionsBrowser::*)(QtProperty *, std::string const &);
  using PropertyGetter = std::string (BasicFitOptionsBrowser::*)(QtProperty *) const;

public:
  BasicFitOptionsBrowser(QWidget *parent = nullptr);
  ~BasicFitOptionsBrowser();

  void setFittingMode(FittingMode fittingMode);
  FittingMode getFittingMode() const;

  void setProperty(std::string const &name, std::string const &value);
  std::string getProperty(std::string const &name) const;

signals:
  void fittingModeChanged(FittingMode fittingMode);

private slots:
  void enumChanged(QtProperty *prop);

private:
  void createBrowser();
  void createProperties();
  void createFittingModeProperty();
  void createMaxIterationsProperty();
  void createMinimizerProperty();
  void createCostFunctionProperty();
  void createEvaluationTypeProperty();

  void addProperty(std::string const &name, QtProperty *prop, PropertyGetter getter, PropertySetter setter);

  void setIntProperty(QtProperty *prop, std::string const &value);
  std::string getIntProperty(QtProperty *prop) const;

  void setStringEnumProperty(QtProperty *prop, std::string const &value);
  std::string getStringEnumProperty(QtProperty *prop) const;

  QtProperty *getQtPropertyFor(std::string const &name) const;

  /// Property managers
  QtIntPropertyManager *m_intManager;
  QtEnumPropertyManager *m_enumManager;

  /// Properties
  QtProperty *m_fittingMode;
  QtProperty *m_maxIterations;
  QtProperty *m_minimizer;
  QtProperty *m_costFunction;
  QtProperty *m_evaluationType;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;

  /// Maps algorithm property name to the QtProperty
  QMap<std::string, QtProperty *> m_propertyNameMap;
  /// Store for the property setter methods
  QMap<QtProperty *, PropertySetter> m_setters;
  /// Store for the property getter methods
  QMap<QtProperty *, PropertyGetter> m_getters;
};

} // namespace MantidWidgets
} // namespace MantidQt
