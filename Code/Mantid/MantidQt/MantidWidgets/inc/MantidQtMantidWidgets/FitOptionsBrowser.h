#ifndef MANTIDWIDGETS_FITOPTIONSBROWSER_H_
#define MANTIDWIDGETS_FITOPTIONSBROWSER_H_

#include "WidgetDllOption.h"

#include <QWidget>
#include <QMap>

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

namespace Mantid
{
namespace Kernel
{
  class Property;
}
namespace API
{
  class IAlgorithm;
}
}

namespace MantidQt
{
namespace MantidWidgets
{

/**
 * Class FitOptionsBrowser implements QtPropertyBrowser to display 
 * and set properties of Fit algorithm (excluding Function and Workspace)
 * 
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS FitOptionsBrowser: public QWidget
{
  Q_OBJECT
public:
  /// Constructor
  FitOptionsBrowser(QWidget *parent = NULL);
  QString getProperty(const QString& name) const;
  void setProperty(const QString& name, const QString& value);
  void copyPropertiesToAlgorithm(Mantid::API::IAlgorithm& fit) const;
  void saveSettings(QSettings& settings) const;
  void loadSettings(const QSettings& settings);

private slots:

  void enumChanged(QtProperty*);

private:

  void createBrowser();
  void createProperties();
  void updateMinimizer();
  QtProperty* createPropertyProperty(Mantid::Kernel::Property* prop);
  QtProperty* addDoubleProperty(const QString& name);

  //  Setters and getters
  QString getMinimizer() const;
  void setMinimizer(const QString&);
  QString getCostFunction() const;
  void setCostFunction(const QString&);
  QString getMaxIterations() const;
  void setMaxIterations(const QString&);
  QString getOutput() const;
  void setOutput(const QString&);
  QString getIgnoreInvalidData() const;
  void setIgnoreInvalidData(const QString&);

  void addProperty(const QString& name, 
    QString (FitOptionsBrowser::*getter)()const, 
    void (FitOptionsBrowser::*setter)(const QString&));

  /// Qt property browser which displays properties
  QtTreePropertyBrowser* m_browser;

  /// Manager for double properties
  QtDoublePropertyManager* m_doubleManager;
  /// Manager for int properties
  QtIntPropertyManager* m_intManager;
  /// Manager for bool properties
  QtBoolPropertyManager* m_boolManager;
  /// Manager for string properties
  QtStringPropertyManager* m_stringManager;
  /// Manager for the string list properties
  QtEnumPropertyManager* m_enumManager;
  /// Manager for groups of properties
  QtGroupPropertyManager* m_groupManager;

  /// Minimizer group property
  QtProperty* m_minimizerGroup;
  /// Minimizer property
  QtProperty* m_minimizer;
  /// CostFunction property
  QtProperty* m_costFunction;
  /// MaxIterations property
  QtProperty* m_maxIterations;
  /// Output property
  QtProperty* m_output;
  /// IgnoreInvalidData property
  QtProperty* m_ignoreInvalidData;

  /// Precision of doubles in m_doubleManager
  int m_decimals;

  /// Store for the properties setter methods
  QMap<QString,void (FitOptionsBrowser::*)(const QString&)> m_setters;
  /// Store for the properties getter methods
  QMap<QString,QString (FitOptionsBrowser::*)()const> m_getters;
};

} // MantidWidgets

} // MantidQt

#endif /*MANTIDWIDGETS_FITOPTIONSBROWSER_H_*/
