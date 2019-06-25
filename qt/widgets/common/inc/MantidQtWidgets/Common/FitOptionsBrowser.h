// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FITOPTIONSBROWSER_H_
#define MANTIDWIDGETS_FITOPTIONSBROWSER_H_

#include "DllOption.h"

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
class EXPORT_OPT_MANTIDQT_COMMON FitOptionsBrowser : public QWidget {
  Q_OBJECT
public:
  /// Support for fitting algorithms:
  ///   Simultaneous: Fit
  ///   Sequential:   PlotPeakByLogValue
  ///   SimultaneousAndSequential: both Fit and PlotPeakByLogValue, toggled with
  ///       "Fitting" property.
  enum FittingType { Simultaneous = 0, Sequential, SimultaneousAndSequential };

  /// Constructor
  FitOptionsBrowser(QWidget *parent = nullptr,
                    FittingType fitType = Simultaneous);
  QString getProperty(const QString &name) const;
  void setProperty(const QString &name, const QString &value);
  void copyPropertiesToAlgorithm(Mantid::API::IAlgorithm &fit) const;
  void saveSettings(QSettings &settings) const;
  void loadSettings(const QSettings &settings);
  FittingType getCurrentFittingType() const;
  void setCurrentFittingType(FittingType fitType);
  void lockCurrentFittingType(FittingType fitType);
  void unlockCurrentFittingType();
  void setLogNames(const QStringList &logNames);
  void setParameterNamesForPlotting(const QStringList &parNames);
  QString getParameterToPlot() const;

signals:
  void changedToSequentialFitting();
  // emitted when m_doubleManager reports a change
  void doublePropertyChanged(const QString &propertyName);

protected:
  QtProperty *addDoubleProperty(const QString &propertyName);
  void displayProperty(const QString &propertyName, bool show = true);
  void displaySequentialFitProperties();

private slots:
  void enumChanged(QtProperty * /*prop*/);
  void doubleChanged(QtProperty *property);

private:
  void createBrowser();
  void initFittingTypeProp();
  void createProperties();
  void createCommonProperties();
  void createSimultaneousFitProperties();
  void createSequentialFitProperties();
  void updateMinimizer();
  void switchFitType();
  void displayNormalFitProperties();

  QtProperty *createPropertyProperty(Mantid::Kernel::Property *prop);

  void addProperty(const QString &name, QtProperty *prop,
                   QString (FitOptionsBrowser::*getter)(QtProperty *) const,
                   void (FitOptionsBrowser::*setter)(QtProperty *,
                                                     const QString &));

  void removeProperty(const QString &name);

  //  Setters and getters
  QString getMinimizer(QtProperty * /*unused*/) const;
  void setMinimizer(QtProperty * /*unused*/, const QString & /*value*/);

  QString getIntProperty(QtProperty * /*prop*/) const;
  void setIntProperty(QtProperty * /*prop*/, const QString & /*value*/);
  QString getDoubleProperty(QtProperty * /*prop*/) const;
  void setDoubleProperty(QtProperty * /*prop*/, const QString & /*value*/);
  QString getBoolProperty(QtProperty * /*prop*/) const;
  void setBoolProperty(QtProperty * /*prop*/, const QString & /*value*/);
  QString getStringEnumProperty(QtProperty * /*prop*/) const;
  void setStringEnumProperty(QtProperty * /*prop*/, const QString & /*value*/);
  QString getStringProperty(QtProperty * /*prop*/) const;
  void setStringProperty(QtProperty * /*prop*/, const QString & /*value*/);

  void setPropertyEnumValues(QtProperty *prop, const QStringList &values);

  /// Manager for bool properties
  QtBoolPropertyManager *m_boolManager;
  /// Manager for int properties
  QtIntPropertyManager *m_intManager;
  /// Manager for double properties
  QtDoublePropertyManager *m_doubleManager;
  /// Manager for string properties
  QtStringPropertyManager *m_stringManager;
  /// Manager for the string list properties
  QtEnumPropertyManager *m_enumManager;
  /// Manager for groups of properties
  QtGroupPropertyManager *m_groupManager;

  /// FitType property
  QtProperty *m_fittingTypeProp;
  /// Minimizer group property
  QtProperty *m_minimizerGroup;
  /// Minimizer property
  QtProperty *m_minimizer;
  /// CostFunction property
  QtProperty *m_costFunction;
  /// MaxIterations property
  QtProperty *m_maxIterations;
  /// EvaluationType property
  QtProperty *m_evaluationType;
  /// Peak radius property
  QtProperty *m_peakRadius;

  // Fit properties
  /// Output property
  QtProperty *m_output;
  /// IgnoreInvalidData property
  QtProperty *m_ignoreInvalidData;

  // PlotPeakByLogValue properties
  /// Store special properties of the sequential Fit
  QList<QtProperty *> m_sequentialProperties;
  /// FitType property
  QtProperty *m_fitType;
  /// OutputWorkspace property
  QtProperty *m_outputWorkspace;
  /// LogValue property
  QtProperty *m_logValue;
  /// Property for a name of a parameter to plot
  /// against LogValue
  QtProperty *m_plotParameter;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;

  /// Precision of doubles in m_doubleManager
  int m_decimals;

  using SetterType = void (FitOptionsBrowser::*)(QtProperty *, const QString &);
  using GetterType = QString (FitOptionsBrowser::*)(QtProperty *) const;
  /// Maps algorithm property name to the QtProperty
  QMap<QString, QtProperty *> m_propertyNameMap;
  /// Store for the properties setter methods
  QMap<QtProperty *, SetterType> m_setters;
  /// Store for the properties getter methods
  QMap<QtProperty *, GetterType> m_getters;

  /// The Fitting Type
  FittingType m_fittingType;
  /// Store special properties of the normal Fit
  QList<QtProperty *> m_simultaneousProperties;
};

} // namespace MantidWidgets

} // namespace MantidQt

#endif /*MANTIDWIDGETS_FITOPTIONSBROWSER_H_*/
