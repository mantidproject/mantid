#ifndef FITPROPERTYBROWSER_H_
#define FITPROPERTYBROWSER_H_

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AlgorithmObserver.h"

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

class PropertyHandler;
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
  /// Get handler to the root composite function
  PropertyHandler* getHandler()const;

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
  PropertyHandler* currentHandler()const;
  /// Set new current function
  void setCurrentFunction(PropertyHandler* h)const;
  /// Get the current function
  const Mantid::API::IFunction* theFunction()const;
  /// Update the function parameters
  void updateParameters();

  /// Create a new function
  PropertyHandler* addFunction(const std::string& fnName);
  /// Get Composite Function
  Mantid::API::CompositeFunction* compositeFunction()const{return m_compositeFunction;}
  /// Get the default function type
  std::string defaultFunctionType()const;
  /// Set the default function type
  void setDefaultFunctionType(const std::string& fnType);
  /// Get the default peak type
  std::string defaultPeakType()const;
  /// Set the default peak type
  void setDefaultPeakType(const std::string& fnType);
  /// Get the default background type
  std::string defaultBackgroundType()const;
  /// Set the default background type
  void setDefaultBackgroundType(const std::string& fnType);

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
  /// Get the cost function
  std::string costFunction()const;

  /// Get the start X
  double startX()const;
  /// Set the start X
  void setStartX(double);
  /// Get the end X
  double endX()const;
  /// Set the end X
  void setEndX(double);

  /// Set LogValue for PlotPeakByLogValue
  void setLogValue(const QString& lv = "");
  /// Get LogValue
  std::string getLogValue()const;
  /// Remove LogValue from the browser
  void removeLogValue();

  /// Return a list of registered functions
  const QStringList& registeredFunctions()const{return m_registeredFunctions;}
  /// Return a list of registered peaks
  const QStringList& registeredPeaks()const{return m_registeredPeaks;}
  /// Return a list of registered backgrounds
  const QStringList& registeredBackgrounds()const{return m_registeredBackgrounds;}
  /// Return a list of registered other functions
  const QStringList& registeredOthers()const{return m_registeredOther;}

  /// Tells if undo can be done
  bool isUndoEnabled()const;
  /// Returns true if the function is ready for a fit
  bool isFitEnabled()const;

  /// Display a tip
  void setTip(const QString& txt);

  /// Creates the "Ties" property value for the Fit algorithm
  QString getTieString()const;

  /// Creates the "Constraints" property value for the Fit algorithm
  QString getConstraintsString()const;

  void init();

  // send parameterChanged signal
  void sendParameterChanged(const Mantid::API::IFunction* f){emit parameterChanged(f);}

  /// Creates and adds the autobackground
  void addAutoBackground();
  bool isAutoBack()const{return m_autoBackground!=NULL;}
  void setAutoBackgroundName(const QString& aName);
  void refitAutoBackground();
  QString getAutoBackgroundString()const{return m_autoBgName + " " + m_autoBgAttributes;}

  /// Number of decimal places in double properties
  int getDecimals()const{return m_decimals;}
  void setDecimals(int d);

public slots:
  void fit();
  void sequentialFit();
  void undoFit();
  void clear();
  void clearBrowser();
  void setPeakToolOn(bool on);
  void findPeaks();

signals:
  void currentChanged()const;
  void functionRemoved();
  void algorithmFinished(const QString&);
  void workspaceIndexChanged(int i);
  void workspaceNameChanged(const QString&);
  void functionChanged();
  void startXChanged(double);
  void endXChanged(double);
  void parameterChanged(const Mantid::API::IFunction*);
  void functionCleared();
  void plotGuess();
  void plotCurrentGuess();
  void removeGuess();
  void removeCurrentGuess();

private slots:

  void enumChanged(QtProperty* prop);
  void boolChanged(QtProperty* prop);
  void intChanged(QtProperty* prop);
  void doubleChanged(QtProperty* prop);
  void stringChanged(QtProperty* prop);
  void filenameChanged(QtProperty* prop);
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
  void removeGuessCurrent();
  void removeGuessAll();
  void plotOrRemoveGuessAll();
  void saveFunction();
  void loadFunction();
  void copy();///< Copy the function string to the clipboard
  void paste();///< Paste a function string from the clipboard
  void updateDecimals();
  void reset();///< reset the function part, renew function, all handlers are new

  void popupMenu(const QPoint &);
  /* Context menu slots */
  void addFunction();
  void deleteFunction();
private:

  /// Create CompositeFunction
  void createCompositeFunction(const QString& str = "");
  /// Get and store available workspace names
  void populateWorkspaceNames();
  /// Get the registered function names
  void populateFunctionNames();
  /// Check if the workspace can be used in the fit
  bool isWorkspaceValid(Mantid::API::Workspace_sptr)const;
  /// Check if the input workspace is a group
  bool isWorkspaceAGroup()const;
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
  /// Create a double property and set some settings
  QtProperty* addDoubleProperty(const QString& name)const;
  /// Create a string property and set some settings
  QtProperty* addStringProperty(const QString& name)const;
  void setStringPropertyValue(QtProperty* prop,const QString& value)const;
  QString getStringPropertyValue(QtProperty* prop)const;
  /// Check that the properties match the function
  void checkFunction();
  /// Sets the workspace to a function
  void setWorkspace(Mantid::API::IFunction* f)const;

  void setCurrentFunction(const Mantid::API::IFunction* f)const;

  /// Does a parameter have a tie
  void hasConstraints(QtProperty* parProp,bool& hasTie,bool& hasBounds)const;
  /// Returns the tie property for a parameter property, or NULL
  QtProperty* getTieProperty(QtProperty* parProp)const;

  /// Make sure m_groupMember belongs to the group
  void validateGroupMember();

  /// Button for doing fit
  QPushButton* m_btnFit;
  /// Button for undoing fit
  QPushButton* m_btnUnFit;
  /// Button for plotting the overall function
  QPushButton* m_btnPlotGuess;
  /// Button for the sequential fit
  QPushButton* m_btnSeqFit;
  /// Button for FindPeaks algorithm
  QPushButton* m_btnFindPeaks;
  /// To display a tip text
  QLabel* m_tip;

  QtTreePropertyBrowser* m_browser;
  /// Property managers:
  QtGroupPropertyManager  *m_groupManager;
  QtDoublePropertyManager *m_doubleManager;
  QtEnumPropertyManager *m_enumManager;
  QtIntPropertyManager *m_intManager;
  QtBoolPropertyManager *m_boolManager;
  QtStringPropertyManager *m_stringManager;
  QtStringPropertyManager *m_filenameManager;
  QtStringPropertyManager *m_formulaManager;
  /// String property managers for special case attributes such as Filename or Formula
  /// <attribute_name,string_manager>
  QMap<QString,QtStringPropertyManager*> m_stringManagers;

  // The main application window
  ApplicationWindow* m_appWindow;

  mutable PropertyHandler* m_currentHandler;

  /// Group for functions
  QtBrowserItem* m_functionsGroup;
  /// Group for input/output settings
  QtBrowserItem* m_settingsGroup;

  QtProperty *m_workspace;
  QtProperty *m_workspaceIndex;
  QtProperty *m_startX;
  QtProperty *m_endX;
  QtProperty *m_output;
  QtProperty *m_minimizer;
  QtProperty *m_costFunction;
  QtProperty *m_logValue;

  /// A list of registered functions
  mutable QStringList m_registeredFunctions;
  /// A list of registered peaks
  mutable QStringList m_registeredPeaks;
  /// A list of registered backgrounds
  mutable QStringList m_registeredBackgrounds;
  /// A list of registered functions that are neither peaks nor backgrounds
  mutable QStringList m_registeredOther;
  /// A list of available workspaces
  mutable QStringList m_workspaceNames;
  /// A list of available minimizers
  mutable QStringList m_minimizers;
  /// A list of available cost functions
  mutable QStringList m_costFunctions;

  /// A copy of the edited function
  Mantid::API::CompositeFunction* m_compositeFunction;
  /// To keep a copy of the initial parameters in case for undo fit
  std::vector<double> m_initialParameters;

  /// Default function name
  std::string m_defaultFunction;
  /// Default peak name
  std::string m_defaultPeak;
  /// Default background name
  std::string m_defaultBackground;

  /// The current function index
  int m_index_;

  /// if true the output name will be guessed every time workspace name is changeed
  bool m_guessOutputName;

  /// If false the change-slots (such as enumChanged(), doubleChanged()) are disabled
  bool m_changeSlotsEnabled;

  /// Shows if the PeakPickerTool is on
  bool m_peakToolOn;

  /// If true background function will be included automatically
  bool m_auto_back;

  /// Name of the autobackground function
  QString m_autoBgName;
  /// List of attributes of the autobackground function as name=value pairs separated by spaces
  QString m_autoBgAttributes;

  /// The autobackground handler
  PropertyHandler* m_autoBackground;

  /// if isWorkspaceAGroup() is true m_groupMember keeps name of the MatrixWorkspace
  /// fitted with theFunction() 
  std::string m_groupMember;

  /// Log names
  QStringList m_logs;

  /// Number of decimal places in double properties
  int m_decimals;

  friend class PropertyHandler;
  friend class CreateAttributeProperty;
  friend class SetAttribute;
  friend class SetAttributeProperty;
  friend class SequentialFitDialog;

};




#endif /*FITPROPERTYBROWSER_H_*/