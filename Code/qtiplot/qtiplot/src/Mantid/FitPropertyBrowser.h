#ifndef FITPROPERTYBROWSER_H_
#define FITPROPERTYBROWSER_H_

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AlgorithmObserver.h"

#include "FitParameterTie.h"

#include <boost/shared_ptr.hpp>
#include <QDockWidget>
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
  /// Destructor
  ~FitPropertyBrowser();
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
  /// Get index
  int index()const;
  /// Set index
  void setIndex(int i)const;
  /// Is the current function a peak?
  bool isPeak()const;
  /// Get the i-th function
  Mantid::API::IFunction* function(int i)const;
  /// Get the current function
  Mantid::API::IFunction* function()const{return function(index());}
  /// Get the i-th function if it is a peak
  Mantid::API::IPeakFunction* peakFunction(int i)const;
  /// Get the current function if it is a peak
  Mantid::API::IPeakFunction* peakFunction()const{return peakFunction(index());}
  /// Select a function
  void selectFunction(int i)const;
  /// Update the function parameters
  void updateParameters();

  /// Create a new function
  void addFunction(const std::string& fnName);
  /// Replace function
  void replaceFunction(int i,const std::string& fnName);
  /// Remove function
  void removeFunction(int i);
  /// Get Composite Function
  boost::shared_ptr<Mantid::API::CompositeFunction> compositeFunction()const{return m_compositeFunction;}
  /// Get the current function type
  std::string functionType(int i)const;
  std::string functionType()const{return functionType(index());}
  /// Get the current function name
  QString functionName(int i)const;
  QString functionName()const{return functionName(index());}
  /// Get the default function name
  std::string defaultFunctionType()const;
  /// Set the default function name
  void setDefaultFunctionType(const std::string& fnType);

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
  /// Get the minimizer
  std::string minimizer()const;

  /// Get the start X
  double startX()const;
  /// Set the start X
  void setStartX(double);
  /// Get the end X
  double endX()const;
  /// Set the end X
  void setEndX(double);

  /// Return a list of registered functions
  const QStringList& registeredFunctions()const{return m_registeredFunctions;}
  /// Return a list of registered peaks
  const QStringList& registeredPeaks()const{return m_registeredPeaks;}
  /// Return a list of registered backgrounds
  const QStringList& registeredBackgrounds()const{return m_registeredBackgrounds;}

  /// Tells if undo can be done
  bool isUndoEnabled()const;
  /// Returns true if the function is ready for a fit
  bool isFitEnabled()const;

  /// Adds a tie
  void addTie(const QString& tstr);
  /// Check ties' validity. Removes invalid ties.
  void removeTiesWithFunction(int);

  /// Display a tip
  void setTip(const QString& txt);

  /// Creates the "Ties" property value for the Fit algorithm
  QString getTieString()const;

  /// Creates the "Constraints" property value for the Fit algorithm
  QString getConstraintsString()const;

  void init();
  void reinit();

public slots:
  void fit();
  void undoFit();
  void clear();
  void setPeakToolOn(bool on){m_peakToolOn = on;}

signals:
  void indexChanged(int i)const;
  void functionRemoved(int i);
  void algorithmFinished(const QString&);
  void workspaceIndexChanged(int i);
  void workspaceNameChanged(const QString&);
  void functionChanged(int);
  void startXChanged(double);
  void endXChanged(double);
  void parameterChanged(int);
  void functionCleared();
  void plotGuess(int i);
  //void removeGuesses();

private slots:
  void enumChanged(QtProperty* prop);
  void boolChanged(QtProperty* prop);
  void intChanged(QtProperty* prop);
  void doubleChanged(QtProperty* prop);
  void stringChanged(QtProperty* prop);
  void workspace_added(const QString &, Mantid::API::Workspace_sptr);
  void workspace_removed(const QString &);
  void currentItemChanged(QtBrowserItem*);
  void addTie();
  void addTieToFunction();
  void addFixTie();
  void deleteTie();
  void addLowerBound10();
  void addLowerBound50();
  void addLowerBound();
  void addLowerBound(int f);
  void addUpperBound10();
  void addUpperBound50();
  void addUpperBound();
  void addUpperBound(int f);
  void addBothBounds10();
  void addBothBounds50();
  void addBothBounds();
  void removeLowerBound();
  void removeUpperBound();
  void removeBounds();
  void plotGuessCurrent();

  void popupMenu(const QPoint &);
  /* Context menu slots */
  void addFunction();
  void deleteFunction();
private:

  /// Create CompositeFunction
  void createCompositeFunction();
  /// Replace function
  void replaceFunction(int i,Mantid::API::IFunction* f);
  /// Get and store available workspace names
  void populateWorkspaceNames();
  /// Get the registered function names
  void populateFunctionNames();
  /// Check if the workspace can be used in the fit
  bool isWorkspaceValid(Mantid::API::Workspace_sptr)const;
  /// Called when the fit is finished
  void finishHandle(const Mantid::API::IAlgorithm* alg);
  /// Find QtBrowserItem for a property prop among the chidren of 
  QtBrowserItem* findItem(QtBrowserItem* parent,QtProperty* prop)const;
  /// Set the parameters to the fit outcome
  void getFitResults();
  /// disable undo when the function changes
  void disableUndo();
  /// Enable/disable the Fit button;
  void setFitEnabled(bool yes);
  /// Adds a tie
  void addTie(int i,QtProperty* parProp,const QString& tieExpr);
  /// Find the tie index for a property. 
  int indexOfTie(QtProperty* tieProp);
  /// Does a parameter have a tie
  bool hasTie(QtProperty* parProp)const;
  /// Returns the tie property for a parameter property, or NULL
  QtProperty* getTieProperty(QtProperty* parProp)const;
  /// Remove all properties associated with a function
  void removeFunProperties(QtProperty* fnProp,bool doubleOnly = false);
  /// Add properties associated with a function: type, attributes, parameters
  void addFunProperties(Mantid::API::IFunction* f,QtProperty* fnProp,bool doubleOnly = false);

  /// Button for doing fit
  QPushButton* m_btnFit;
  /// Button for undoing fit
  QPushButton* m_btnUnFit;
  /// To display a tip text
  QLabel* m_tip;

  QtTreePropertyBrowser* m_browser;
  /// Property managers:
  QtGroupPropertyManager  *m_groupManager;
  QtDoublePropertyManager *m_doubleManager;
  QtStringPropertyManager *m_stringManager;
  QtEnumPropertyManager *m_enumManager;
  QtIntPropertyManager *m_intManager;
  QtBoolPropertyManager *m_boolManager;
  /// Properties:

  mutable int m_index;

  /// The top level group
  //QtBrowserItem* m_fitGroup;
  /// Group for functions
  QtBrowserItem* m_functionsGroup;
  /// Group for input/output settings
  QtBrowserItem* m_settingsGroup;
  /// Browser items for functions
  QList<QtBrowserItem*> m_functionItems;

  QtProperty *m_workspace;
  QtProperty *m_workspaceIndex;
  QtProperty *m_startX;
  QtProperty *m_endX;
  QtProperty *m_output;
  QtProperty *m_minimizer;

  /// A list of registered functions
  mutable QStringList m_registeredFunctions;
  /// A list of registered peaks
  mutable QStringList m_registeredPeaks;
  /// A list of registered backgrounds
  mutable QStringList m_registeredBackgrounds;
  /// A list of available workspaces
  mutable QStringList m_workspaceNames;
  /// A list of available minimizers
  mutable QStringList m_minimizers;

  /// A copy of the edited function
  boost::shared_ptr<Mantid::API::CompositeFunction> m_compositeFunction;
  /// To keep a copy of the initial parameters in case for undo fit
  std::vector<double> m_initialParameters;

  /// Default function name
  std::string m_defaultFunction;

  /// Default width for added peaks
  double m_default_width;

  /// The current function index
  int m_index_;

  /// if true the output name will be guessed every time workspace name is changeed
  bool m_guessOutputName;

  /// If false the change-slots (such as enumChanged(), doubleChanged()) are disabled
  bool m_changeSlotsEnabled;

  /// Ties
  QList<FitParameterTie> m_ties;

  /// Constraints <parameter property, <lower bound property, upper bound property> >
  QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> > m_constraints;

  /// Shows if the PeakPickerTool is on
  bool m_peakToolOn;

  ApplicationWindow* m_appWindow;

};


#endif /*FITPROPERTYBROWSER_H_*/