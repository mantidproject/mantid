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

private slots:

  void enumChanged(QtProperty*);

private:

  void createBrowser();
  void createProperties();
  void updateMinimizer();
  QtProperty* createPropertyProperty(Mantid::Kernel::Property* prop);
  QtProperty* addDoubleProperty(const QString& name);

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

  /// Precision of doubles in m_doubleManager
  int m_decimals;
};

} // MantidWidgets

} // MantidQt

#endif /*MANTIDWIDGETS_FITOPTIONSBROWSER_H_*/
