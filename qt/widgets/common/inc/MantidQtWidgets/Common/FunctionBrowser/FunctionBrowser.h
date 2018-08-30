#ifndef MANTIDWIDGETS_FUNCTIONBROWSER_H_
#define MANTIDWIDGETS_FUNCTIONBROWSER_H_

#include "DllConfig.h"

#include "IFunctionBrowser.h"

#include <QHash>
#include <QWidget>

#include <boost/optional.hpp>

/* Forward declarations */
template <typename PropertyManager> class QtAbstractEditorFactory;
class QtAbstractEditorFactoryBase;
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
class FunctionBrowserSubscriber;
}
} // namespace MantidQt

namespace MantidQt {
namespace MantidWidgets {
namespace QENS {

class MANTIDQT_INDIRECT_DLL FunctionBrowser : public QWidget,
                                              public IFunctionBrowser {
  Q_OBJECT
public:
  /// To keep QtProperty and its QtBrowserItem in one place
  struct AProperty {
    QtProperty *prop;
    QtBrowserItem *item;
    QtProperty *parent;
  };

  FunctionBrowser(QWidget *parent = nullptr);

  void subscribe(FunctionBrowserSubscriber *subscriber) override;

  void setParameterValue(std::string const &name, double value) override;
  void setParameterError(std::string const &name, double value) override;
  void removeParameterError(std::string const &name) override;
  void setParameterTie(std::string const &name,
                       std::string const &tie) override;
  void removeParameterTie(std::string const &name) override;
  void setParameterUpperBound(std::string const &name, double bound) override;
  void setParameterLowerBound(std::string const &name, double bound) override;
  void setParameterBounds(std::string const &name, double lowerBound,
                          double upperBound) override;
  void removeParameterUpperBound(std::string const &name) override;
  void removeParameterLowerBound(std::string const &name) override;
  void removeParameterConstraints(std::string const &name) override;

  std::vector<std::size_t> getSelectedFunctionPosition() const override;
  void selectFunctionAt(std::vector<std::size_t> const &position) override;

  void addFunctionToSelectedFunction(std::string const &name) override;
  void addFunctionToSelectedFunctionAndSelect(std::string const &name) override;
  void removeSelectedFunction() override;
  void addParameterToSelectedFunction(std::string const &name,
                                      std::string const &description,
                                      double value) override;
  void addIndexToSelectedFunction(std::string const &index) override;
  void
  setIndicesOfFunctionsAt(std::vector<std::string> const &indices,
                          std::vector<std::size_t> const &position) override;

  void addIntAttributeToSelectedFunction(std::string const &name,
                                         int value) override;
  void addBoolAttributeToSelectedFunction(std::string const &name,
                                          bool value) override;
  void addDoubleAttributeToSelectedFunction(std::string const &name,
                                            double value) override;
  void addStringAttributeToSelectedFunction(std::string const &name,
                                            std::string const &value) override;
  void addFileAttributeToSelectedFunction(std::string const &name,
                                          std::string const &value) override;
  void addFormulaAttributeToSelectedFunction(std::string const &name,
                                             std::string const &value) override;
  void
  addWorkspaceAttributeToSelectedFunction(std::string const &name,
                                          std::string const &value) override;
  void addVectorAttributeToSelectedFunction(
      std::string const &name, std::vector<double> const &value) override;
  void setIntAttribute(std::string const &name, int value) override;
  void setBoolAttribute(std::string const &name, bool value) override;
  void setDoubleAttribute(std::string const &name, double value) override;
  void setStringAttribute(std::string const &name,
                          std::string const &value) override;
  void setFileAttribute(std::string const &name,
                        std::string const &value) override;
  void setFormulaAttribute(std::string const &name,
                           std::string const &value) override;
  void setWorkspaceAttribute(std::string const &name,
                             std::string const &value) override;
  void setVectorAttribute(std::string const &name,
                          std::vector<double> const &value) override;
  void copyToClipboard(std::string const &str) override;
  void displayCompositeMenu() override;
  void displayFunctionMenu() override;
  void displayParameterMenu(bool isTied, bool isConstrained) override;

protected:
  /// Get the root function property
  virtual QtProperty *getRootFunctionProperty() const;
  /// Check if property is a function group
  bool isFunction(QtProperty *prop) const;
  /// Check if a property is a tie
  bool isTie(QtProperty *prop) const;
  /// Check if a property is a constraint
  bool isConstraint(QtProperty *prop) const;
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
  bool isAttribute(QtProperty *prop) const;
  /// Check if property is a function paramater
  bool isParameter(QtProperty *prop) const;
  /// Check if a property is an index
  bool isIndex(QtProperty *prop) const;
  /// Get the function index for a property
  QString getIndex(QtProperty *prop) const;
  QtProperty *getIndexProperty(QtProperty *prop) const;
  QtProperty *findIndexProperty(QList<QtProperty *> const &properties) const;
  /// Get name of the parameter for a property
  QString getParameterName(QtProperty *prop) const;
  /// Get property of the overall function
  AProperty FunctionBrowser::getFunctionProperty() const;
  /// Gets the local index of a function inside the parent
  std::size_t getLocalFunctionIndex(QtProperty *parent, QtProperty *prop) const;
  /// Gets the local index of a function within the specified list
  std::size_t getLocalFunctionIndex(QList<QtProperty *> const &subProperties,
                                    QtProperty *prop) const;
  /// Gets the position of the function represented by the specified property
  std::vector<std::size_t> getFunctionPosition(QtProperty *prop) const;

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
  /// Remove constraints from parameter represented by specified property
  void removeConstraints(QtProperty *prop);
  /// Add both constraints to current parameter
  void addConstraints10();
  /// Add both constraints to current parameter
  void addConstraints50();
  /// Remove one of the constraints
  void removeConstraint();
  /// Sets the property which has been selected in the browser
  void setSelectedProperty(QtBrowserItem *selectedItem);

  void stringAttributeChanged(QtProperty *, QString const &);
  void intAttributeChanged(QtProperty *, int);
  void doubleAttributeChanged(QtProperty *, double);
  void boolAttributeChanged(QtProperty *, bool);
  void vectorDoubleAttributeChanged(QtProperty *);
  void vectorSizeAttributeChanged(QtProperty *);
  void tieChanged(QtProperty *);

  void connectEditorCloseToBrowser(QtAbstractEditorFactoryBase *editor);
  virtual AProperty addParameterProperty(QtProperty *prop, QtProperty *parent,
                                         QString const &name,
                                         QString const &description,
                                         double value);

private:
  void createBrowser();
  virtual std::unique_ptr<QtTreePropertyBrowser> createNewBrowser();
  void createManagers();
  void createEditorFactories();
  void connectManagerSignals();
  void createActions();

  virtual std::unique_ptr<QtAbstractEditorFactory<ParameterPropertyManager>>
  getParameterEditorFactory();

  /// Add a function property
  AProperty addFunctionProperty(QtProperty *parent, QString const &funName);
  /// Add a parameter property
  AProperty addParameterProperty(QtProperty *parent, QString const &paramName,
                                 QString const &paramDesc, double paramValue);
  /// Add an attribute property
  AProperty addAttributeProperty(QtProperty *parent, QtProperty *prop);
  /// Add property showing function's index in the composite function
  AProperty addIndexProperty(QtProperty *prop, QString const &index);
  void setIndicesOfFunctionsAt(QtProperty *parent,
                               std::vector<std::string> const &indices);
  void setIndicesOfFunctionsIn(QList<QtProperty *> const &properties,
                               std::vector<std::string> const &indices);
  void setIndexPropertyOf(QtProperty *prop, QString const &index);
  /// Add a tie property
  void addTieProperty(QtProperty *prop, QString tie);
  /// Get the parameter property of the parameter with the specified name
  QtProperty *getParameterProperty(std::string const &parameterName) const;
  /// Get tie property of the specified parameter property
  QtProperty *getTieProperty(QtProperty *prop) const;
  /// Get upper bound property of the specified parameter property
  QtProperty *getUpperBoundProperty(QtProperty *prop) const;
  /// Get lower bound property of the specified parameter property
  QtProperty *getLowerBoundProperty(QtProperty *prop) const;
  /// Get a property for a parameter which is a parent of a given
  /// property (tie or constraint).
  QtProperty *getParentParameterProperty(QtProperty *prop) const;

  std::string getNameOfProperty(QtProperty *prop) const;
  QtProperty *getFirstProperty() const;
  QtProperty *getContainingFunctionProperty(QtProperty *prop) const;
  QtProperty *
  getFunctionPropertyAt(std::vector<std::size_t> const &position) const;
  QtProperty *
  getFunctionPropertyAt(QtProperty *start,
                        std::vector<std::size_t> const &position) const;
  QtProperty *
  getFunctionPropertyAt(QList<QtProperty *> properties,
                        std::vector<std::size_t> const &position) const;
  QtProperty *getFunctionPropertyAt(QList<QtProperty *> const &properties,
                                    std::size_t position) const;
  QtProperty *getParameterPropertyIn(QtProperty *prop,
                                     QString const &parameter) const;

  /// Add a sub-property
  AProperty addProperty(QtProperty *parent, QtProperty *subproperty);
  /// Add a lower bound property
  void addLowerBoundProperty(QtProperty *parent, QString const &value);
  /// Add an upper bound property
  void addUpperBoundProperty(QtProperty *parent, QString const &value);
  /// Set lower bound property
  void setLowerBoundProperty(QtProperty *parent, QString const &value);
  /// Set upper bound property
  void setUpperBoundProperty(QtProperty *parent, QString const &value);
  /// Remove lower bound property
  void removeLowerBoundProperty(QtProperty *parent);
  /// Remove upper bound property
  void removeUpperBoundProperty(QtProperty *parent);
  /// Remove and delete property
  void removeProperty(QtProperty *prop);

  void displayPopupMenu(QtProperty *prop);
  void displayDefaultMenu();
  void addTieActionsToMenu(QMenu &menu);
  void addClipboardActionsToMenu(QMenu &menu);
  std::unique_ptr<QMenu> getConstraintMenu();
  boost::optional<QString> getTieFromDialog();
  virtual QString getFunctionFromUserDialog() const;

  /// Qt property browser which displays properties
  QtTreePropertyBrowser *m_browser;

  FunctionBrowserSubscriber *m_subscriber;

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

  /// Store all properties in a map for easy access
  QHash<QtProperty *, AProperty> m_properties;
  /// Store parameter name to parameter property
  QHash<QString, QtProperty *> m_parameterNameToProperty;
  /// Store attribute name to attribute property
  QHash<QString, QtProperty *> m_attributeNameToProperty;

  QtProperty *m_selectedProperty;
};

} // namespace QENS
} // namespace MantidWidgets
} // namespace MantidQt

#endif
