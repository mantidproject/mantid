#ifndef MANTIDWIDGETS_FUNCTIONBROWSER_H_
#define MANTIDWIDGETS_FUNCTIONBROWSER_H_

#include "WidgetDllOption.h"

#include "MantidAPI/IFunction.h"

#include <QWidget>
#include <QMap>

    /* Forward declarations */

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtProperty;
class QtBrowserItem;

class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class QSignalMapper;
class QMenu;
class QAction;
class QTreeWidget;

namespace Mantid
{
  namespace API
  {
    class CompositeFunction;
    class Workspace;
    class ParameterTie;
  }
}

namespace MantidQt
{
namespace MantidWidgets
{

namespace 
{
  class CreateAttributeProperty;
}

/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display 
 * and control fitting function parameters and settings.
 * 
 * @date 18/04/2012
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS FunctionBrowser: public QWidget
{
  Q_OBJECT
  /// To keep QtProperty and its QtBrowserItem in one place
  struct AProperty
  {
    QtProperty *prop;
    QtBrowserItem *item;
    QtProperty *parent;
  };

public:
  /// Constructor
  FunctionBrowser(QWidget *parent = NULL);
  /// Destructor
  ~FunctionBrowser();
  /// Clear the contents
  void clear();
  /// Set the function in the browser
  void setFunction(QString funStr);
  /// Return FunctionFactory function string
  QString getFunctionString(QtProperty* prop = NULL) const;

protected:
  /// Create the Qt property browser
  void createBrowser();
  /// Create and connect actions
  void createActions();
  /// Add a sub-property
  AProperty addProperty(QtProperty* parent, QtProperty* subproperty);
  /// Remove and delete property
  void removeProperty(QtProperty *prop);
  /// Add a function
  void addFunction(QtProperty* prop, QString funStr);
  /// Add a function property
  AProperty addFunctionProperty(QtProperty* parent, QString funName);
  /// Add a parameter property
  AProperty addParameterProperty(QtProperty* parent, QString paramName, double paramValue);
  /// Add a attribute property
  AProperty addAttributeProperty(QtProperty* parent, QString attName, const Mantid::API::IFunction::Attribute& att);
  /// Add attribute and parameter properties to a function property
  void addAttributeAndParameterProperties(QtProperty* prop, Mantid::API::IFunction_sptr fun);
  /// Add property showing function's index in the composite function
  AProperty addIndexProperty(QtProperty* prop);
  /// Update function index properties 
  void updateFunctionIndices(QtProperty* prop = NULL, QString index = "");
  /// Check if property is a function group
  bool isFunction(QtProperty* prop) const;
  /// Check if property is a function attribute
  bool isAttribute(QtProperty* prop) const;
  /// Check if property is a function attribute
  bool isStringAttribute(QtProperty* prop) const;
  /// Check if property is a function attribute
  bool isDoubleAttribute(QtProperty* prop) const;
  /// Check if property is a function attribute
  bool isIntAttribute(QtProperty* prop) const;
  /// Get attribute as a string
  QString getAttribute(QtProperty* prop) const;
  /// Check if property is a function paramater
  bool isParameter(QtProperty* prop) const;
  /// Get attribute as a string
  QString getParameter(QtProperty* prop) const;
  /// Check if a property is an index
  bool isIndex(QtProperty* prop) const;

protected slots:
  /// Show the context menu
  void popupMenu(const QPoint &);
  /// Add a function
  void addFunction();
  /// Remove a function
  void removeFunction();

protected:
  /// Manager for function group properties
  QtGroupPropertyManager *m_functionManager;
  /// Manager for function parameter properties
  QtDoublePropertyManager *m_parameterManager;
  /// Manager for function string attribute properties
  QtStringPropertyManager *m_attributeStringManager;
  /// Manager for function double attribute properties
  QtDoublePropertyManager *m_attributeDoubleManager;
  /// Manager for function int attribute properties
  QtIntPropertyManager *m_attributeIntManager;
  /// Manager for function index properties
  QtStringPropertyManager *m_indexManager;
  /// Qt property browser which displays properties
  QtTreePropertyBrowser* m_browser;
  /// Store all properties in a map for easy access
  QMap<QtProperty*,AProperty> m_properties;

  //   Actions

  /// Add a function
  QAction *m_actionAddFunction;
  /// Remove a function
  QAction *m_actionRemoveFunction;

  friend class CreateAttributeProperty;
};


} // MantidWidgets

} // MantidQt

#endif /*MANTIDWIDGETS_FUNCTIONBROWSER_H_*/
