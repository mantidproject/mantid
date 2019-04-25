// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONBROWSER_H_
#define MANTIDWIDGETS_FUNCTIONBROWSER_H_

#include "DllOption.h"

#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/IFunctionBrowser.h"

#include <QMap>
#include <QWidget>

#include <boost/optional.hpp>

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

class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class QSignalMapper;
class QMenu;
class QAction;
class QTreeWidget;

namespace Mantid {
namespace API {
class CompositeFunction;
class Workspace;
class ParameterTie;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

class CreateAttributePropertyForFunctionBrowser;

/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display
 * and control fitting function parameters and settings.
 *
 * @date 18/04/2012
 */
class EXPORT_OPT_MANTIDQT_COMMON FunctionBrowser : public QWidget,
                                                   public IFunctionBrowser {
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
    QString paramName;     ///< Parameter name
    QtProperty *tieProp;   ///< Tie property
  };
  /// Constraint structure
  struct AConstraint {
    QtProperty *paramProp; ///< Parameter property
    QtProperty *lower;     ///< Constraint property
    QtProperty *upper;     ///< Constraint property
  };

  /// Constructor
  FunctionBrowser(QWidget *parent = nullptr, bool multi = false);
  /// Destructor
  virtual ~FunctionBrowser() override;
  /// Clear the contents
  void clear() override;
  /// Set the function in the browser
  void setFunction(const QString &funStr) override;
  /// Set the function in the browser
  void setFunction(Mantid::API::IFunction_sptr fun);
  /// Return FunctionFactory function string
  QString getFunctionString() override;
  /// Return the function
  Mantid::API::IFunction_sptr getFunction(QtProperty *prop = nullptr,
                                          bool attributesOnly = false);
  /// Check if a function is set
  bool hasFunction() const;

  /// Return a function with specified index
  Mantid::API::IFunction_sptr getFunctionByIndex(const QString &index);

  /// Return index of the current function, if one is selected
  boost::optional<QString> currentFunctionIndex() {
    return m_currentFunctionIndex;
  }

  /// Update the function parameter value
  void setParameter(const QString &funcIndex, const QString &paramName,
                    double value);
  /// Update the function parameter error
  void setParamError(const QString &funcIndex, const QString &paramName,
                     double error);
  /// Get a value of a parameter
  double getParameter(const QString &funcIndex, const QString &paramName) const;
  /// Update the function parameter value
  void setParameter(const QString &paramName, double value);
  /// Update the function parameter error
  void setParamError(const QString &paramName, double error);
  /// Get a value of a parameter
  double getParameter(const QString &paramName) const;
  /// Update parameter values in the browser to match those of a function.
  void updateParameters(const Mantid::API::IFunction &fun) override;

  /// Get a list of names of global parameters
  QStringList getGlobalParameters() const;
  void setGlobalParameters(QStringList &globals);
  /// Get a list of names of local parameters
  QStringList getLocalParameters() const;
  /// Get the number of datasets
  int getNumberOfDatasets() const override;
  /// Get value of a local parameter
  double getLocalParameterValue(const QString &parName, int i) const override;
  /// Set value of a local parameter
  void setLocalParameterValue(const QString &parName, int i,
                              double value) override;
  /// Set value and error of a local parameter
  void setLocalParameterValue(const QString &parName, int i, double value,
                              double error);
  /// Get error of a local parameter
  double getLocalParameterError(const QString &parName, int i) const;
  /// Check if a local parameter is fixed
  bool isLocalParameterFixed(const QString &parName, int i) const override;
  /// Fix/unfix local parameter
  void setLocalParameterFixed(const QString &parName, int i,
                              bool fixed) override;
  /// Get the tie for a local parameter.
  QString getLocalParameterTie(const QString &parName, int i) const override;
  /// Set a tie for a local parameter.
  void setLocalParameterTie(const QString &parName, int i,
                            QString tie) override;

  /// Return the multidomain function if number of datasets is greater than 1
  Mantid::API::IFunction_sptr getGlobalFunction() override;
  /// Update parameter values in the browser to match those of a function.
  void updateMultiDatasetParameters(const Mantid::API::IFunction &fun) override;

  /// Resize the browser's columns
  void setColumnSizes(int s0, int s1, int s2 = -1);

  /// Set error display on/off
  void setErrorsEnabled(bool enabled) override;
  /// Clear all errors
  void clearErrors() override;

signals:
  /// User selects a different function (or one of it's sub-properties)
  void currentFunctionChanged();

  /// Function parameter gets changed
  /// @param funcIndex :: Index of the changed function
  /// @param paramName :: Name of the changed parameter
  void parameterChanged(const QString &funcIndex,
                        const QString &paramName) override;

  /// In multi-dataset context a button value editor was clicked
  void localParameterButtonClicked(const QString &parName);

  void functionStructureChanged() override;
  void globalsChanged();

public slots:

  // Handling of multiple datasets
  void setNumberOfDatasets(int n) override;
  void resetLocalParameters();
  void setCurrentDataset(int i) override;
  void removeDatasets(QList<int> indices);
  void addDatasets(int n);

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
  void setFunction(QtProperty *prop, Mantid::API::IFunction_sptr fun);
  /// Add a function
  void addFunction(QtProperty *prop, Mantid::API::IFunction_sptr fun);
  /// Add a function property
  AProperty addFunctionProperty(QtProperty *parent, QString funName);
  /// Add a parameter property
  AProperty addParameterProperty(QtProperty *parent, QString paramName,
                                 QString paramDesc, double paramValue);
  /// Add a attribute property
  AProperty addAttributeProperty(QtProperty *parent, QString attName,
                                 const Mantid::API::IFunction::Attribute &att);
  /// Add attribute and parameter properties to a function property
  void addAttributeAndParameterProperties(QtProperty *prop,
                                          Mantid::API::IFunction_sptr fun);
  /// Add property showing function's index in the composite function
  AProperty addIndexProperty(QtProperty *prop);
  /// Update function index properties
  void updateFunctionIndices(QtProperty *prop = nullptr, QString index = "");
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
  QString getIndex(QtProperty *prop) const;
  /// Get name of the parameter for a property
  QString getParameterName(QtProperty *prop);
  /// Get function property for the index
  QtProperty *getFunctionProperty(const QString &index) const;
  /// Split a qualified parameter name into function index and local parameter
  /// name.
  QStringList splitParameterName(const QString &paramName) const;
  /// Get a property for a parameter
  QtProperty *getParameterProperty(const QString &paramName) const;
  /// Get a property for a parameter
  QtProperty *getParameterProperty(const QString &funcIndex,
                                   const QString &paramName) const;
  /// Get a property for a parameter which is a parent of a given
  /// property (tie or constraint).
  QtProperty *getParentParameterProperty(QtProperty *prop) const;
  /// Get a tie property attached to a parameter property
  QtProperty *getTieProperty(QtProperty *prop) const;

  /// Add a tie property
  void addTieProperty(QtProperty *prop, QString tie);
  /// Check if a parameter property has a tie
  bool hasTie(QtProperty *prop) const;
  /// Check if a property is a tie
  bool isTie(QtProperty *prop) const;
  /// Get a tie for a paramater
  std::string getTie(QtProperty *prop) const;

  /// Add a constraint property
  QList<AProperty> addConstraintProperties(QtProperty *prop,
                                           QString constraint);
  /// Check if a property is a constraint
  bool isConstraint(QtProperty *prop) const;
  /// Check if a parameter property has a constraint
  bool hasConstraint(QtProperty *prop) const;
  /// Check if a parameter property has a lower bound
  bool hasLowerBound(QtProperty *prop) const;
  /// Check if a parameter property has a upper bound
  bool hasUpperBound(QtProperty *prop) const;
  /// Get a constraint string
  QString getConstraint(const QString &paramName, const QString &lowerBound,
                        const QString &upperBound) const;

  /// Initialize storage and values for local parameters
  void initLocalParameter(const QString &parName) const;
  /// Make sure that the parameter is initialized
  void checkLocalParameter(const QString &parName) const;
  /// Check that a property contains a local parameter
  bool isLocalParameterProperty(QtProperty *prop) const;
  /// Check that a property contains a global parameter
  bool isGlobalParameterProperty(QtProperty *prop) const;
  /// Make sure that properties are in sync with the cached ties
  void updateLocalTie(const QString &parName);
  /// Make sure that properties are in sync with the cached constraints
  void updateLocalConstraint(const QString &parName);

  /// Ask user for function type
  virtual QString getUserFunctionFromDialog();

protected slots:
  /// Show the context menu
  void popupMenu(const QPoint & /*unused*/);
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
  /// Update current function index depending on currently selected item
  void updateCurrentFunctionIndex();

  //   Property change slots

  /// Called when a function attribute property is changed
  void attributeChanged(QtProperty * /*prop*/);
  /// Called when a member of a vector attribute is changed
  void attributeVectorDoubleChanged(QtProperty * /*prop*/);
  /// Called when the size of a vector attribute is changed
  void attributeVectorSizeChanged(QtProperty * /*prop*/);
  /// Called when a function parameter property is changed
  void parameterChanged(QtProperty * /*prop*/);
  /// Called when button in local parameter editor was clicked
  void parameterButtonClicked(QtProperty * /*prop*/);
  /// Called when a tie property changes
  void tieChanged(QtProperty * /*prop*/);
  /// Called when a constraint property changes
  void constraintChanged(QtProperty * /*prop*/);
  /// Called when "Global" check-box was clicked
  void globalChanged(QtProperty * /*unused*/, const QString & /*unused*/,
                     bool /*unused*/);
  /// Set value of an attribute (as a property) to a function
  void setAttributeToFunction(Mantid::API::IFunction &fun, QtProperty *prop);

protected:
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
  QtStringPropertyManager *m_constraintManager;
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

  /// Index of currently selected function. Gets updated in
  /// updateCurrentFunctionIndex()
  boost::optional<QString> m_currentFunctionIndex;

  struct LocalParameterData {
    explicit LocalParameterData(double v = 0.0, double e = 0.0)
        : value(v), error(e), fixed(false) {}
    double value;
    double error;
    bool fixed;
    QString tie;
    QString lowerBound;
    QString upperBound;
  };

  /// Set true if the constructed function is intended to be used in a
  /// multi-dataset fit
  bool m_multiDataset;
  /// Number of datasets this function is used for
  int m_numberOfDatasets;
  /// Storage for local paramter values
  mutable QMap<QString, QVector<LocalParameterData>> m_localParameterValues;
  /// Index of a dataset for which the parameters are currently displayed
  int m_currentDataset;

  friend class CreateAttributePropertyForFunctionBrowser;
  friend class SetAttributeFromProperty;
};

} // namespace MantidWidgets

} // namespace MantidQt

#endif /*MANTIDWIDGETS_FUNCTIONBROWSER_H_*/
