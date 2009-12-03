#ifndef FITPROPERTYBROWSER_H_
#define FITPROPERTYBROWSER_H_

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AlgorithmObserver.h"
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

class FitPropertyBrowser: public QDockWidget, public Mantid::API::AlgorithmObserver
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
  /// Creates Composite Function
  void setComposite(bool on=true);
  /// Is the current function a peak?
  bool isPeak()const;

  /// Create a new function
  void addFunction(const std::string& fnName);
  /// Replace function
  void replaceFunction(const std::string& fnName);
  /// Remove function
  void removeFunction();
  /// Set the current function
  void setFunction(Mantid::API::IFunction* fun);
  /// Get Composite Function
  boost::shared_ptr<Mantid::API::CompositeFunction> compositeFunction()const{return m_compositeFunction;}
  /// Get the current function name
  std::string functionName()const;
  /// Get the default function name
  std::string defaultFunctionName()const;

  /// Get the input workspace name
  std::string workspaceName()const;
  /// Set the input workspace name
  void setWorkspaceName(const QString& wsName);
  /// Get workspace index
  int workspaceIndex()const;
  /// Set workspace index
  void setWorkspaceIndex(int i);
  /// Get the output name
  std::string outputName()const;
  /// Set the output name
  void setOutputName(const std::string&);

  /// Get the start X
  double startX()const;
  /// Set the start X
  void setStartX(double);
  /// Get the end X
  double endX()const;
  /// Set the end X
  void setEndX(double);

  void init();
  void reinit();

signals:
  void indexChanged(int i);
  void functionRemoved(int i);
  void algorithmFinished(const QString&);
  void workspaceIndexChanged(int i);
  void workspaceNameChanged(const QString&);
  void functionChanged(const QString&);
  void startXChanged(double);
  void endXChanged(double);

private slots:
  void enumChanged(QtProperty* prop);
  void boolChanged(QtProperty* prop);
  void intChanged(QtProperty* prop);
  void doubleChanged(QtProperty* prop);
  void stringChanged(QtProperty* prop);
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
  void finishHandle(const Mantid::API::IAlgorithm* alg);

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

  /// Default width for added peaks
  double m_default_width;

  /// if true the output name will be guessed every time workspace name is changeed
  bool m_guessOutputName;

  ApplicationWindow* m_appWindow;

};


#endif /*FITPROPERTYBROWSER_H_*/