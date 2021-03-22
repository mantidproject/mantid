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

public:
  BasicFitOptionsBrowser(QWidget *parent = nullptr);
  ~BasicFitOptionsBrowser();

  void setFittingMode(FittingMode fittingMode);
  FittingMode getFittingMode() const;

  void setProperty(const QString &name, const QString &value);
  QString getProperty(const QString &name) const;

signals:
  void changedToSequentialFitting();
  void changedToSimultaneousFitting();

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

  void emitFittingModeChanged();

  void addProperty(const QString &name, QtProperty *prop,
                   QString (BasicFitOptionsBrowser::*getter)(QtProperty *)
                       const,
                   void (BasicFitOptionsBrowser::*setter)(QtProperty *,
                                                          const QString &));

  void setIntProperty(QtProperty *prop, QString const &value);
  QString getIntProperty(QtProperty *prop) const;

  void setStringEnumProperty(QtProperty *prop, QString const &value);
  QString getStringEnumProperty(QtProperty *prop) const;

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

  using SetterType = void (BasicFitOptionsBrowser::*)(QtProperty *,
                                                      const QString &);
  using GetterType = QString (BasicFitOptionsBrowser::*)(QtProperty *) const;

  /// Maps algorithm property name to the QtProperty
  QMap<QString, QtProperty *> m_propertyNameMap;
  /// Store for the properties setter methods
  QMap<QtProperty *, SetterType> m_setters;
  /// Store for the properties getter methods
  QMap<QtProperty *, GetterType> m_getters;
};

} // namespace MantidWidgets
} // namespace MantidQt
