// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FittingMode.h"

#include <QMap>
#include <QWidget>

/* Forward declarations */
class QtProperty;
class QtTreePropertyBrowser;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtGroupPropertyManager;
class QSettings;

namespace Mantid {
namespace Kernel {
class Property;
}
namespace API {
class IAlgorithm;
}
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

/**
 * Class FitOptionsBrowser implements QtPropertyBrowser to display
 * and set properties of Fit algorithm (excluding Function and Workspace)
 *
 */
class EXPORT_OPT_MANTIDQT_COMMON BasicFitOptionsBrowser : public QWidget {
  Q_OBJECT
public:
  BasicFitOptionsBrowser(QWidget *parent = nullptr,
                         FittingMode fitType = FittingMode::SIMULTANEOUS);
  ~BasicFitOptionsBrowser();
  void setProperty(const QString &name, const QString &value);
  FittingMode getCurrentFittingType() const;
  void setCurrentFittingType(FittingMode fitType);

signals:
  void changedToSequentialFitting();
  void changedToSimultaneousFitting();

private slots:
  void enumChanged(QtProperty * /*prop*/);

private:
  void createBrowser();
  void initFittingTypeProp();
  void createProperties();
  void createCommonProperties();
  void switchFitType();

  void addProperty(const QString &name, QtProperty *prop,
                   QString (BasicFitOptionsBrowser::*getter)(QtProperty *)
                       const,
                   void (BasicFitOptionsBrowser::*setter)(QtProperty *,
                                                          const QString &));

  QString getIntProperty(QtProperty * /*prop*/) const;
  void setIntProperty(QtProperty * /*prop*/, const QString & /*value*/);
  QString getStringEnumProperty(QtProperty * /*prop*/) const;
  void setStringEnumProperty(QtProperty * /*prop*/, const QString & /*value*/);

  /// Manager for int properties
  QtIntPropertyManager *m_intManager;
  /// Manager for the string list properties
  QtEnumPropertyManager *m_enumManager;

  /// FitType property
  QtProperty *m_fittingTypeProp;
  /// Minimizer property
  QtProperty *m_minimizer;
  /// CostFunction property
  QtProperty *m_costFunction;
  /// MaxIterations property
  QtProperty *m_maxIterations;
  /// EvaluationType property
  QtProperty *m_evaluationType;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;

  /// Precision of doubles in m_doubleManager
  int m_decimals;

  using SetterType = void (BasicFitOptionsBrowser::*)(QtProperty *,
                                                      const QString &);
  using GetterType = QString (BasicFitOptionsBrowser::*)(QtProperty *) const;
  /// Maps algorithm property name to the QtProperty
  QMap<QString, QtProperty *> m_propertyNameMap;
  /// Store for the properties setter methods
  QMap<QtProperty *, SetterType> m_setters;
  /// Store for the properties getter methods
  QMap<QtProperty *, GetterType> m_getters;

  /// The Fitting Type
  FittingMode m_fittingType;
  QList<QtProperty *> m_blacklist;
};

} // namespace MantidWidgets

} // namespace MantidQt
