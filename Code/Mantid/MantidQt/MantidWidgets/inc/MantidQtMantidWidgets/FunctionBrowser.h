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

class CreateAttributePropertyForFunctionBrowser;

/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display 
 * and control fitting function parameters and settings.
 * 
 * @date 18/04/2012
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS FunctionBrowser: public QWidget
{
  Q_OBJECT
public:
  /// To keep QtProperty and its QtBrowserItem in one place
  struct AProperty
  {
    QtProperty *prop;
    QtBrowserItem *item;
    QtProperty *parent;
  };
  /// Tie structure
  struct ATie
  {
    QtProperty *paramProp; ///< Parameter property
    QtProperty *tieProp;   ///< Tie property
  };
  /// Constraint structure
  struct AConstraint
  {
    QtProperty *paramProp; ///< Parameter property
    QtProperty *lower;   ///< Constraint property
    QtProperty *upper;   ///< Constraint property
  };

  /// Constructor
  FunctionBrowser(QWidget *parent = NULL);
  /// Destructor
  ~FunctionBrowser();
  /// Clear the contents
  void clear();
  /// Set the function in the browser
  void setFunction(const QString& funStr);
  /// Set the function in the browser
  void setFunction(Mantid::API::IFunction_sptr fun);
  /// Return FunctionFactory function string
  QString getFunctionString();
  /// Return the function
  Mantid::API::IFunction_sptr getFunction(QtProperty* prop = NULL, bool attributesOnly = false);

protected:
  /// Create the Qt property browser
  void createBrowser();
  /// Create and connect actions
  void createActions();
  /// Add a sub-property
  AProperty addProperty(QtProperty* parent, QtProperty* subproperty);
  /// Remove and delete property
  void removeProperty(QtProperty *prop);
  /// Set a function
  void setFunction(QtProperty* prop, Mantid::API::IFunction_sptr fun);
  /// Add a function
  void addFunction(QtProperty* prop, Mantid::API::IFunction_sptr fun);
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
  /// Get property of the overall function
  AProperty getFunctionProperty();
  /// Check if property is a function group
  bool isFunction(QtProperty* prop) const;
  /// Check if property is a function attribute
  bool isAttribute(QtProperty* prop) const;
  /// Check if property is a string attribute
  bool isStringAttribute(QtProperty* prop) const;
  /// Check if property is a double attribute
  bool isDoubleAttribute(QtProperty* prop) const;
  /// Check if property is a int attribute
  bool isIntAttribute(QtProperty* prop) const;
  /// Check if property is a bool attribute
  bool isBoolAttribute(QtProperty* prop) const;
  /// Check if property is a vector attribute
  bool isVectorAttribute(QtProperty* prop) const;
  /// Check if property is a function paramater
  bool isParameter(QtProperty* prop) const;
  /// Get attribute as a string
  double getParameter(QtProperty* prop) const;
  /// Check if a property is an index
  bool isIndex(QtProperty* prop) const;
  /// Get the function index for a property
  QString getIndex(QtProperty* prop) const;

  /// Add a tie property
  AProperty addTieProperty(QtProperty* prop, QString tie);
  /// Check if a parameter property has a tie
  bool hasTie(QtProperty* prop) const;
  /// Check if a property is a tie
  bool isTie(QtProperty* prop) const;
  /// Get a tie for a paramater
  std::string getTie(QtProperty* prop) const;

  /// Add a constraint property
  QList<AProperty> addConstraintProperties(QtProperty* prop, QString constraint);
  /// Check if a property is a constraint
  bool isConstraint(QtProperty* prop) const;
  /// Check if a parameter property has a constraint
  bool hasConstraint(QtProperty* prop) const;
  /// Check if a parameter property has a lower bound
  bool hasLowerBound(QtProperty* prop) const;
  /// Check if a parameter property has a upper bound
  bool hasUpperBound(QtProperty* prop) const;

protected slots:
  /// Show the context menu
  void popupMenu(const QPoint &);
  /// Add a function
  void addFunction();
  /// Remove a function
  void removeFunction();
  /// Fix a parameter
  void fixParameter();
  /// Unfix a parameter
  void removeTie();
  /// Add a tie to a parameter
  void addTie();
  /// Copy function from the clipboard
  void copyFromClipboard();
  /// Copy the function to the clipboard
  void copyToClipboard();
  /// Add both constraints to current parameter
  void addConstraints();
  /// Remove both constraints from current parameter
  void removeConstraints();
  /// Add both constraints to current parameter
  void addConstraints10();
  /// Add both constraints to current parameter
  void addConstraints50();
  /// Remove one of the constraints
  void removeConstraint();

  //   Property change slots

  /// Called when a function attribute property is changed
  void attributeChanged(QtProperty*);
  /// Called when a member of a vector attribute is changed
  void attributeVectorDoubleChanged(QtProperty*);

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
  /// Manager for function bool attribute properties
  QtBoolPropertyManager *m_attributeBoolManager;
  /// Manager for function index properties
  QtStringPropertyManager *m_indexManager;
  /// Manager for function tie properties
  QtStringPropertyManager *m_tieManager;
  /// Manager for parameter constraint properties
  QtStringPropertyManager *m_constraintManager;
  /// Manager for Formula attributes
  QtStringPropertyManager *m_formulaManager;
  /// Manager for vector attribute properties
  QtGroupPropertyManager *m_attributeVectorManager;
  /// Manager for vector attribute member properties
  QtDoublePropertyManager *m_attributeVectorDoubleManager;
  /// Manager for vector attribute size properties
  QtIntPropertyManager *m_attributeSizeManager;


  /// Qt property browser which displays properties
  QtTreePropertyBrowser* m_browser;
  /// Store all properties in a map for easy access
  QMap<QtProperty*,AProperty> m_properties;
  /// Store parameter ties. Keys are function properties.
  QMultiMap<QtProperty*,ATie> m_ties;
  /// Store parameter constraints. Keys are function properties.
  QMultiMap<QtProperty*,AConstraint> m_constraints;

  //   Actions

  /// Add a function
  QAction *m_actionAddFunction;
  /// Remove a function
  QAction *m_actionRemoveFunction;
  /// Fix a parameter
  QAction *m_actionFixParameter;
  /// Unfix a parameter
  QAction *m_actionRemoveTie;
  /// Add a custom tie to a parameter
  QAction *m_actionAddTie;
  /// Copy a function from the clipboard
  QAction *m_actionFromClipboard;
  /// Copy a function to the clipboard
  QAction *m_actionToClipboard;
  /// Add both constraints to current parameter with 10% spread
  QAction *m_actionConstraints10;
  /// Add both constraints to current parameter with 50% spread
  QAction *m_actionConstraints50;
  /// Add both constraints to current parameter
  QAction *m_actionConstraints;
  /// Remove both constraints from current parameter
  QAction *m_actionRemoveConstraints;
  /// Remove one constraints from current parameter
  QAction *m_actionRemoveConstraint;

  friend class CreateAttributePropertyForFunctionBrowser;
  friend class SetAttributeFromProperty;
};


} // MantidWidgets

} // MantidQt

#endif /*MANTIDWIDGETS_FUNCTIONBROWSER_H_*/
