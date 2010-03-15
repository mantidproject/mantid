#ifndef FITPROPERTYBROWSER_H_
#define FITPROPERTYBROWSER_H_

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AlgorithmObserver.h"

#include "FitParameterTie.h"

//#include <boost/shared_ptr.hpp>
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
    class ParameterTie;
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
  /// Is the current function a peak?
  bool isPeak()const;
  /// Get the current function
  Mantid::API::IFunction* function()const{return m_currentFunction;}
  /// Set new current function
  void setCurrentFunction(Mantid::API::IFunction* f)const;
  /// Get the current function
  Mantid::API::IFunction* theFunction()const;
  /// Get the current function if it is a peak
  Mantid::API::IPeakFunction* peakFunction()const;
  /// Update the function parameters
  void updateParameters();
  /// Remove items from m_functionItems
  void removeFunctionItems(QtBrowserItem* fnItem);

  /// Create a new function
  void addFunction(const std::string& fnName, Mantid::API::CompositeFunction* cfun = NULL);
  /// Replace function
  void replaceFunction(Mantid::API::IFunction* f_old,const std::string& fnName);
  /// Remove function
  void removeFunction(Mantid::API::IFunction* f);
  /// Get Composite Function
  Mantid::API::CompositeFunction* compositeFunction()const{return m_compositeFunction;}
  /// Get the current function name
  QString functionName(Mantid::API::IFunction* f,Mantid::API::CompositeFunction* cf = NULL)const;
  QString functionName()const{return functionName(m_currentFunction);}
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
  //void addTie(const QString& tstr);
  /// Check ties' validity. Removes invalid ties.
  //void removeTiesWithFunction(int);

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
  void currentChanged()const;
  void functionRemoved(Mantid::API::IFunction*);
  void algorithmFinished(const QString&);
  void workspaceIndexChanged(int i);
  void workspaceNameChanged(const QString&);
  void functionChanged(int);
  void startXChanged(double);
  void endXChanged(double);
  void parameterChanged(Mantid::API::IFunction*);
  void functionCleared();
  void plotGuess(Mantid::API::IFunction*);

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
  void addConstraint(int f,bool lo,bool up);
  void addUpperBound10();
  void addUpperBound50();
  void addUpperBound();
  void addBothBounds10();
  void addBothBounds50();
  void addBothBounds();
  void removeBounds();
  void plotGuessCurrent();
  void plotGuessAll();

  void popupMenu(const QPoint &);
  /* Context menu slots */
  void addFunction();
  void deleteFunction();
private:

  /// Create CompositeFunction
  void createCompositeFunction();
  /// Replace function
  void replaceFunction(Mantid::API::IFunction* f_old,Mantid::API::IFunction* f_new);
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
  /// Remove all properties associated with a function
  void removeFunProperties(QtProperty* fnProp,bool doubleOnly = false);
  /// Add properties associated with a function: type, attributes, parameters
  void addFunProperties(Mantid::API::IFunction* f,bool doubleOnly = false);
  /// Create a double property and set some settings
  QtProperty* addDoubleProperty(const QString& name)const;
  /// Updates function names after removing/replacing functions
  void updateNames();
  /// Get the property for a parameter
  QtProperty* getParameterProperty(Mantid::API::IFunction* f,int i)const;
  /// Check that the properties match the function
  void checkFunction();

   /// Adds a tie
  bool addTie(const QString& tieExpr,Mantid::API::IFunction* f);
   /// Adds a tie
  //void addTie(int i,QtProperty* parProp,const QString& tieExpr);
  /// Find the tie index for a property. 
  //int indexOfTie(QtProperty* tieProp);
  /// Does a parameter have a tie
  bool hasTie(QtProperty* parProp)const;
  /// Returns the tie property for a parameter property, or NULL
  QtProperty* getTieProperty(QtProperty* parProp)const;
  /// Extracts lower and upper bounds form a string of the form 1<Sigma<3, or 1<Sigma or Sigma < 3
  void extractLowerAndUpper(const std::string& str,double& lo,double& up,bool& hasLo, bool& hasUp)const;

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

  mutable Mantid::API::IFunction* m_currentFunction;

  /// The top level group
  //QtBrowserItem* m_fitGroup;
  /// Group for functions
  QtBrowserItem* m_functionsGroup;
  /// Group for input/output settings
  QtBrowserItem* m_settingsGroup;
  /// Browser items for functions
  QMap<QtBrowserItem*,Mantid::API::IFunction*> m_functionItems;
  /// Map from properties to their browser items
  QMap<QtProperty*,QtBrowserItem*> m_paramItems;

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
  Mantid::API::CompositeFunction* m_compositeFunction;
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
  QMap<QtProperty*,Mantid::API::ParameterTie*> m_ties;

  /// Constraints <parameter property, <lower bound property, upper bound property> >
  QMap<QtProperty*,std::pair<QtProperty*,QtProperty*> > m_constraints;

  /// Shows if the PeakPickerTool is on
  bool m_peakToolOn;

  ApplicationWindow* m_appWindow;

};


#endif /*FITPROPERTYBROWSER_H_*/