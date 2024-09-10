// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/IFunctionView.h"

#include <QMap>
#include <QRect>
#include <QStringList>
#include <QWidget>

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
class ParameterPropertyManager;
class DoubleDialogEditorFactory;

class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class QSignalMapper;
class QMenu;
class QAction;
class QTreeWidget;
class QTreeWidgetItem;

namespace Mantid {
namespace API {
class Workspace;
class ParameterTie;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

class CreateAttributePropertyForFunctionTreeView;
class SelectFunctionDialog;
struct GlobalTie;

/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display
 * and control fitting function parameters and settings.
 *
 * @date 18/04/2012
 */
class EXPORT_OPT_MANTIDQT_COMMON FunctionTreeView : public IFunctionView {
  Q_OBJECT
public:
  /// To keep QtProperty and its QtBrowserItem in one place
  struct AProperty {
    QtProperty *prop;
    QtBrowserItem *item;
    QtProperty *parent;
  };
  /// Tie structure
  struct ATie {
    QtProperty *paramProp; ///< Parameter property
    std::string paramName; ///< Parameter name
    QtProperty *tieProp;   ///< Tie property
  };
  /// Constraint structure
  struct AConstraint {
    QtProperty *paramProp; ///< Parameter property
    QtProperty *lower;     ///< Constraint property
    QtProperty *upper;     ///< Constraint property
  };

  /// Constructor
  FunctionTreeView(QWidget *parent, bool multi, std::vector<std::string> categories = std::vector<std::string>());
  /// Destructor
  ~FunctionTreeView() override;
  /// Clear the contents
  void clear() override;
  /// Set the function in the browser
  void setFunction(Mantid::API::IFunction_sptr fun) override;
  /// Check if a function is set
  bool hasFunction() const override;
  /// Update the function parameter value
  void setParameter(std::string const &parameterName, double value) override;
  /// Update the function parameter error
  void setParameterError(std::string const &parameterName, double error) override;
  /// Get a value of a parameter
  double getParameter(std::string const &parameterName) const override;
  /// Get a value of a attribute
  IFunction::Attribute getAttribute(std::string const &attrName) const override;
  /// Set error display on/off
  void setErrorsEnabled(bool enabled) override;
  /// Clear all errors
  void clearErrors() override;
  /// Index of currently selected function.
  std::optional<std::string> currentFunctionIndex() const override;
  /// Set a tie
  void setParameterTie(std::string const &parameterName, std::string const &tie) override;
  /// Set a constraint
  void setParameterConstraint(std::string const &parameterName, std::string const &constraint) override;
  /// Set new global parameters.
  void setGlobalParameters(const std::vector<std::string> &) override;
  /// Get a list of global parameters
  std::vector<std::string> getGlobalParameters() const;

  IFunction_sptr getSelectedFunction() override;
  void showFunctionHelp(std::string const &functionName) const override;

  /// Return the function
  Mantid::API::IFunction_sptr getFunction(QtProperty *prop = nullptr, bool attributesOnly = false);
  /// Resize the browser's columns
  void setColumnSizes(int s0, int s1, int s2 = -1);
  /// Set the last column to stretch.
  void setStretchLastColumn(bool stretch);

  // Hide global boxes
  void hideGlobals();
  // Show global boxes
  void showGlobals();

  // Sets the function prefix of a domain to be displayed within a
  // MultiDomainFunction
  void setMultiDomainFunctionPrefix(std::string const &functionPrefix);
  // Sets the global ties to be displayed within a MultiDomainFunction
  void setGlobalTies(std::vector<GlobalTie> const &globalTies);

protected:
  /// Create the Qt property browser
  void createBrowser();
  /// Create and connect actions
  void createActions();
  /// Add a sub-property
  AProperty addProperty(QtProperty *parent, QtProperty *subproperty);
  /// Remove and delete property
  void removeProperty(QtProperty *prop);
  /// Set a function
  void setFunction(QtProperty *prop, const Mantid::API::IFunction_sptr &fun);
  /// Add a function
  bool addFunction(QtProperty *prop, const Mantid::API::IFunction_sptr &fun);
  /// Add a function property
  AProperty addFunctionProperty(QtProperty *parent, std::string const &funName);
  /// Add a parameter property
  AProperty addParameterProperty(QtProperty *parent, std::string const &parameterName, std::string const &paramDesc,
                                 double paramValue);
  /// Add a attribute property
  AProperty addAttributeProperty(QtProperty *parent, std::string const &attName,
                                 const Mantid::API::IFunction::Attribute &att);
  /// Add attribute and parameter properties to a function property
  void addAttributeAndParameterProperties(QtProperty *prop, const Mantid::API::IFunction_sptr &fun,
                                          const Mantid::API::CompositeFunction_sptr &parentComposite = nullptr,
                                          const std::size_t &parentIndex = 0);
  /// Add tie to a parameter property. This will also work for ties across
  /// functions in a composite function.
  void addParameterTie(QtProperty *property, const Mantid::API::IFunction_sptr &function,
                       const std::string &parameterName, const std::size_t &parameterIndex,
                       const Mantid::API::CompositeFunction_sptr &parentComposite = nullptr,
                       const std::size_t &parentIndex = 0);
  /// Add tie to a parameter property stored within a composite function.
  bool addParameterTieInComposite(QtProperty *property, const std::string &parameterName,
                                  const Mantid::API::CompositeFunction_sptr &composite, const std::size_t &index);
  /// Adds a global tie for a parameter if one exists
  void addGlobalParameterTie(QtProperty *property, const std::string &parameterName,
                             const CompositeFunction_sptr &parentComposite = nullptr,
                             const std::size_t &parentIndex = 0);
  /// Adds an index property representing the function index of a specific
  /// domain within a MultiDomainFunction.
  void addMultiDomainIndexProperty(QtProperty *prop);
  /// Add property showing function's index in the composite function
  AProperty addIndexProperty(QtProperty *prop);
  /// Update function index properties
  void updateFunctionIndices(QtProperty *prop = nullptr, std::string const &index = "");
  /// Get property of the overall function
  AProperty getFunctionProperty() const;
  /// Check if property is a function group
  bool isFunction(QtProperty *prop) const;
  /// Check if property is a function attribute
  bool isAttribute(QtProperty *prop) const;
  /// Check if property is a string attribute
  bool isStringAttribute(QtProperty *prop) const;
  /// Check if property is a double attribute
  bool isDoubleAttribute(QtProperty *prop) const;
  /// Check if property is a int attribute
  bool isIntAttribute(QtProperty *prop) const;
  /// Check if property is a bool attribute
  bool isBoolAttribute(QtProperty *prop) const;
  /// Check if property is a vector attribute
  bool isVectorAttribute(QtProperty *prop) const;
  /// Check if property is a function paramater
  bool isParameter(QtProperty *prop) const;
  /// Get attribute as a string
  double getParameter(QtProperty *prop) const;
  /// Check if a property is an index
  bool isIndex(QtProperty *prop) const;
  /// Get the function index for a property
  std::string getIndex(QtProperty *prop) const;
  /// Get name of the parameter for a property
  std::string getParameterName(QtProperty *prop) const;
  /// Get name of the attribute for a property
  std::string getAttributeName(QtProperty *prop) const;
  /// Get function property for the index
  QtProperty *getFunctionProperty(std::string const &index) const;
  ///// Get a property for a parameter
  /// Get a property for a parameter
  QtProperty *getParameterProperty(std::string const &parameterName) const;
  /// Get a property for a parameter
  QtProperty *getAttributeProperty(std::string const &parameterName) const;
  /// Get a property for a parameter which is a parent of a given
  /// property (tie or constraint).
  QtProperty *getParentParameterProperty(QtProperty *prop) const;
  /// Get a tie property attached to a parameter property
  QtProperty *getTieProperty(QtProperty *prop) const;

  /// Add a tie property
  void addTieProperty(QtProperty *prop, std::string const &tie, bool globalTie = false);
  /// Check if a parameter property has a tie
  bool hasTie(QtProperty *prop) const;
  /// Check if a property is a tie
  bool isTie(QtProperty *prop) const;
  /// Get a tie for a paramater
  QString getTie(QtProperty *prop) const;

  /// Add a constraint property
  QList<AProperty> addConstraintProperties(QtProperty *prop, std::string const &constraint);
  /// Check if a property is a constraint
  bool isConstraint(QtProperty *prop) const;
  /// Check if a parameter property has a constraint
  bool hasConstraint(QtProperty *prop) const;
  /// Check if a parameter property has a lower bound
  bool hasLowerBound(QtProperty *prop) const;
  /// Check if a parameter property has a upper bound
  bool hasUpperBound(QtProperty *prop) const;
  /// Get a constraint string
  std::string getConstraint(std::string const &parameterName, const double &lowerBound = Mantid::EMPTY_DBL(),
                            const double &upperBound = Mantid::EMPTY_DBL()) const;
  /// Get a pair of function index (eg f0.f2.) and constraint expression given a
  /// parameter property
  std::pair<std::string, std::string> getFunctionAndConstraint(QtProperty *prop) const;

protected slots:
  /// Show the context menu
  void popupMenu(const QPoint &);
  /// Add a function
  void addFunctionBegin();
  void addFunctionEnd(int result);
  /// Remove a function
  void removeFunction();
  /// Fix a parameter
  void fixParameter();
  /// Unfix a parameter
  void removeTie();
  /// Add a tie to a parameter
  void addTie();
  /// Copy function from the clipboard
  void pasteFromClipboard();
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
  /// Update index of currently selected function
  void updateCurrentFunctionIndex();
  //   Property change slots
  /// Called when a function attribute property is changed
  void attributeChanged(QtProperty *);
  /// Called when a member of a vector attribute is changed
  void attributeVectorDoubleChanged(QtProperty *);
  /// Called when the size of a vector attribute is changed
  void attributeVectorSizeChanged(QtProperty *);
  /// Called when a function parameter property is changed
  void parameterPropertyChanged(QtProperty *);
  /// Called when button in local parameter editor was clicked
  void parameterButtonClicked(QtProperty *);
  /// Called when a tie property changes
  void tieChanged(QtProperty *);
  /// Called when a constraint property changes
  void constraintChanged(QtProperty *);
  /// Called when "Global" check-box was clicked
  void globalChanged(QtProperty *, const QString &, bool);
  /// Set value of an attribute (as a property) to a function
  void setAttributeToFunction(Mantid::API::IFunction &fun, QtProperty *prop);

protected:
  void removeConstraintsQuiet(QtProperty *paramProp);

  /// Editor used for editing doubles.
  DoubleDialogEditorFactory *m_doubleEditorFactory;
  /// Manager for function group properties
  QtGroupPropertyManager *m_functionManager;
  /// Manager for function parameter properties
  ParameterPropertyManager *m_parameterManager;
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
  QtDoublePropertyManager *m_constraintManager;
  /// Manager for file name attributes
  QtStringPropertyManager *m_filenameManager;
  /// Manager for Formula attributes
  QtStringPropertyManager *m_formulaManager;
  /// Manager for Workspace attributes
  QtStringPropertyManager *m_workspaceManager;
  /// Manager for vector attribute properties
  QtGroupPropertyManager *m_attributeVectorManager;
  /// Manager for vector attribute member properties
  QtDoublePropertyManager *m_attributeVectorDoubleManager;
  /// Manager for vector attribute size properties
  QtIntPropertyManager *m_attributeSizeManager;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;
  /// Store all properties in a map for easy access
  QMap<QtProperty *, AProperty> m_properties;
  /// Store parameter ties. Keys are function properties.
  QMultiMap<QtProperty *, ATie> m_ties;
  /// Store parameter constraints. Keys are function properties.
  QMultiMap<QtProperty *, AConstraint> m_constraints;

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
  /// Show function help menu
  QAction *m_actionFunctionHelp;
  /// Index of currently selected function. Gets updated in
  /// updateCurrentFunctionIndex()
  std::optional<std::string> m_currentFunctionIndex;
  /// Set true if the constructed function is intended to be used in a
  /// multi-dataset fit
  bool m_multiDataset;
  /// The function prefix of the domain with a MultiDomainFunction currently
  /// being displayed.
  std::string m_multiDomainFunctionPrefix;
  // A vector of global ties. E.g. f0.f0.A0=f1.f0.A0
  std::vector<GlobalTie> m_globalTies;
  std::vector<std::string> m_allowedCategories;
  SelectFunctionDialog *m_selectFunctionDialog;
  QtProperty *m_selectedFunctionProperty;
  bool m_emitParameterValueChange = true;
  bool m_emitAttributeValueChange = true;

  friend class CreateAttributePropertyForFunctionTreeView;
  friend class SetAttributeFromProperty;

private:
  /// Update a double attribute
  void setDoubleAttribute(std::string const &attrName, double value) override;
  void setIntAttribute(std::string const &attrName, int value) override;
  void setStringAttribute(std::string const &attrName, std::string &value) override;
  void setBooleanAttribute(std::string const &attrName, bool value) override;
  void setVectorAttribute(std::string const &attrName, std::vector<double> &val) override;

  /// Gets the full tie when using the m_multiDomainFunctionPrefix
  std::string getFullTie(std::string const &tie) const;
  /// Gets the full parameter name when using the m_multiDomainFunctionPrefix
  std::string getFullParameterName(const std::string &parameter, int compositeIndex = -1) const;

  // Intended for testing only
  QTreeWidgetItem *getPropertyWidgetItem(QtProperty *prop) const;
  QRect visualItemRect(QtProperty *prop) const;

public:
  // Intended for testing only
  int getNumberOfQtProperties() const { return m_properties.size(); }
  int getNumberOfTieProperties() const { return m_ties.size(); }
  int getNumberOfConstraintProperties() const { return m_constraints.size(); }
  QRect getVisualRectFunctionProperty(std::string const &index) const;
  QRect getVisualRectParameterProperty(std::string const &index) const;
  QTreeWidget *treeWidget() const;
  QtTreePropertyBrowser *treeBrowser();
  DoubleDialogEditorFactory *doubleEditorFactory();
  QWidget *getParamWidget(std::string const &parameterName) const;
};

} // namespace MantidWidgets
} // namespace MantidQt
