#ifndef FITPROPERTYBROWSER_H_
#define FITPROPERTYBROWSER_H_

#include "MantidAPI/Workspace.h"
#include <boost/shared_ptr.hpp>
#include <QDockWidget>
#include <QMap>

    /* Forward definitions */

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtProperty;

class ApplicationWindow;

namespace Mantid
{
  namespace API
  {
    class IFunction;
    class IPeakFunction;
    class CompositeFunction;
    class Workspace;
  }
}
/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display 
 * and control fitting function parameters and settings.
 * 
 * @autor Roman Tolchenov, Tessella Support Services plc
 * @date 13/11/2009
 */

class FitPropertyBrowser: public QDockWidget
{
  Q_OBJECT
public:
  /// Constructor
  FitPropertyBrowser(QWidget* parent);
  /// Centre of the current peak
  double centre()const;
  /// Set centre of the current peak
  void setCentre(double value);
  /// Height of the current peak
  double height()const;
  /// Set height of the current peak
  void setHeight(double value);
  /// Width of the current peak
  double width()const;
  /// Set width of the current peak
  void setWidth(double value);
  /// Get count
  int count()const;
  /// Set count
  void setCount();
  /// Get index
  int index()const;
  /// Set index
  void setIndex(int i);

  /// Create a new function
  void addFunction(const std::string& fnName);
  /// Replace function
  void replaceFunction(const std::string& fnName);
  /// Remove function
  void removeFunction();
  /// Set the current function
  void setFunction(Mantid::API::IFunction* fun);
  /// Get the current function name
  std::string functionName()const;
  /// Get the input workspace name
  std::string workspaceName()const;
  /// Get the output name
  std::string outputName()const;

  void init();

private slots:
  void functionChanged(QtProperty* prop);
  void boolChanged(QtProperty* prop);
  void intChanged(QtProperty* prop);
  void doubleChanged(QtProperty* prop);
  void fit();
  void workspace_added(const QString &, Mantid::API::Workspace_sptr);
  void workspace_removed(const QString &);
private:

  /// Is this function composite
  bool isComposite()const;
  /// Index of member function
  int functionIndex()const;
  /// Create CompositeFunction
  void createCompositeFunction();
  /// Remove CompositeFunction
  void removeCompositeFunction();
  /// Makes sure m_functionName property shows the right function name
  void displayFunctionName();
  /// Makes sure the parameters are displayed correctly
  void displayParameters();
  /// Makes sure the peak parameters (centre,height,width) are displayed correctly
  void displayPeak();
  /// Get and store available workspace names
  void populateWorkspaceNames();
  /// Get the registered function names
  void populateFunctionNames();
  /// Check if the workspace can be used in the fit
  bool isWorkspaceValid(Mantid::API::Workspace_sptr)const;

  QtTreePropertyBrowser* m_browser;
  /// Property managers:
  QtGroupPropertyManager  *m_groupManager;
  QtDoublePropertyManager *m_doubleManager;
  QtStringPropertyManager *m_stringManager;
  QtEnumPropertyManager *m_enumManager;
  QtIntPropertyManager *m_intManager;
  QtBoolPropertyManager *m_boolManager;
  /// Properties:
  QtProperty *m_functionGroup;
  QtProperty *m_functionName;
  QtProperty *m_peakGroup;
  QtProperty *m_height;
  QtProperty *m_centre;
  QtProperty *m_width;
  QtProperty *m_parametersGroup;
  QMap<std::string,QtProperty*> m_parameters;
  QtProperty *m_composite;
  QtProperty *m_compositeGroup;
  QtProperty *m_count;
  QtProperty *m_index;

  QtProperty *m_workspace;
  QtProperty *m_workspaceIndex;
  QtProperty *m_startX;
  QtProperty *m_endX;
  QtProperty *m_output;

  /// A list of registered functions
  mutable QStringList m_registeredFunctions;
  /// A list of available workspaces
  mutable QStringList m_workspaceNames;

  /// A copy of the edited function
  Mantid::API::IFunction* m_function;
  boost::shared_ptr<Mantid::API::CompositeFunction> m_compositeFunction;

  /// Default function name
  std::string m_defaultFunction;

  ApplicationWindow* m_appWindow;

};


#endif /*FITPROPERTYBROWSER_H_*/