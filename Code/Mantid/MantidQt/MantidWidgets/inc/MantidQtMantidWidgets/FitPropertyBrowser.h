#ifndef FITPROPERTYBROWSER_H_
#define FITPROPERTYBROWSER_H_

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "WidgetDllOption.h"

#include <QDockWidget>
#include <QHash>
#include <QList>
#include <QMap>

#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"


    /* Forward declarations */

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class ParameterPropertyManager;

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

namespace MantidQt
{
namespace MantidWidgets
{

class PropertyHandler;
/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display 
 * and control fitting function parameters and settings.
 * 
 * @date 13/11/2009
 */

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS FitPropertyBrowser: public QDockWidget, public Mantid::API::AlgorithmObserver,
                          public MantidQt::API::WorkspaceObserver
{
  Q_OBJECT
public:
  /// Constructor
  FitPropertyBrowser(QWidget *parent = NULL, QObject* mantidui = NULL);
  /// Destructor
  ~FitPropertyBrowser();
  /// Get handler to the root composite function
  PropertyHandler* getHandler()const;
  /// Initialise layout
  virtual void init();

  /// Centre of the current peak
  double centre()const;
  /// Set centre of the current peak
  void setCentre(double value);
  /// Height of the current peak
  double height()const;
  /// Set height of the current peak
  void setHeight(double value);
  /// Width of the current peak
  double fwhm()const;
  /// Set width of the current peak
  void setFwhm(double value);
  /// Get count
  int count()const;
  /// Is the current function a peak?
  bool isPeak()const;
  /// Get the current function
  PropertyHandler* currentHandler()const;
  /// Set new current function
  void setCurrentFunction(PropertyHandler* h)const;
  /// Get the current function
  boost::shared_ptr<const Mantid::API::IFunction> theFunction()const;
  /// Update the function parameters
  void updateParameters();
  /// Get function parameter values
  QList<double> getParameterValues() const;
  /// Get function parameter names
  QStringList getParameterNames() const;

  /// Create a new function
  PropertyHandler* addFunction(const std::string& fnName);

  /// Get Composite Function
  boost::shared_ptr<Mantid::API::CompositeFunction> compositeFunction()const{return m_compositeFunction;}

  /// Return the fitting function
  Mantid::API::IFunction_sptr getFittingFunction() const;

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

  /// Get the workspace
  boost::shared_ptr<Mantid::API::Workspace> getWorkspace() const;
  /// Get the input workspace name
  std::string workspaceName()const;
  /// Set the input workspace name
  virtual void setWorkspaceName(const QString& wsName);
  /// Get workspace index
  int workspaceIndex()const;
  /// Set workspace index
  void setWorkspaceIndex(int i);
  /// Get the output name
  std::string outputName()const;
  /// Set the output name
  void setOutputName(const std::string&);
  /// Get the minimizer
  std::string minimizer(bool withProperties = false)const;
  /// Get the ignore invalid data option
  bool ignoreInvalidData() const;
  /// Set the ignore invalid data option
  void setIgnoreInvalidData(bool on);
  /// Get the cost function
  std::string costFunction()const;
  /// Get the "ConvolveMembers" option
  bool convolveMembers()const;

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

  /// return groupMember
  //const std::string groupMember() const {return m_groupMember;};
  /// alter text of Plot Guess 
  void setTextPlotGuess(const QString text);

  /// Creates the "Ties" property value for the Fit algorithm
  QString getTieString()const;

  /// Creates the "Constraints" property value for the Fit algorithm
  QString getConstraintsString()const;

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

  /// Returns true if the difference plot should be drawn
  bool plotDiff()const;
  /// Returns true if a composite's member functions should be plotted also
  bool plotCompositeMembers() const;

  /// Returns true if the fit should be done against binned (bunched) data.  	
  bool rawData()const;

  void setADSObserveEnabled(bool enabled);

  void postDeleteHandle(const std::string& wsName);
  void addHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);

  /// Called when the Fit is finished
  virtual void finishHandle(const Mantid::API::IAlgorithm* alg);

  /// Returns the list of workspaces that are currently been worked on by the fit property browser.
  QStringList getWorkspaceNames();
  /// Create a MatrixWorkspace from a TableWorkspace
  Mantid::API::Workspace_sptr createMatrixFromTableWorkspace()const;


public slots:
  virtual void fit();
  virtual void sequentialFit();
  void undoFit();
  void clear();
  void clearBrowser();
  void setPeakToolOn(bool on);
  void findPeaks();
  void executeFitMenu(const QString&);
  void executeDisplayMenu(const QString&);
  void executeSetupMenu(const QString&);
  void executeSetupManageMenu(const QString&);

signals:
  void currentChanged()const;
  void functionRemoved();
  void algorithmFinished(const QString&);
  void workspaceIndexChanged(int i);
  void workspaceNameChanged(const QString&);

  void wsChangePPAssign(const QString&);
  void functionChanged();

  void startXChanged(double);
  void endXChanged(double);
  void xRangeChanged(double, double);
  void parameterChanged(const Mantid::API::IFunction*);
  void functionCleared();
  void plotGuess();
  void plotCurrentGuess();
  void removeGuess();
  void removeCurrentGuess();
  void changeWindowTitle(const QString&);
  void removePlotSignal(MantidQt::MantidWidgets::PropertyHandler*);
  void removeFitCurves();

  void executeFit(QString,QHash<QString,QString>,Mantid::API::AlgorithmObserver*);
  void multifitFinished();

  /// signal which can optionally be caught for customization after a fit has 
  /// been done
  void fittingDone(QString);
  void functionFactoryUpdateReceived();

protected slots:
  /// Get the registered function names
  virtual void populateFunctionNames();

private slots:

  void enumChanged(QtProperty* prop);
  void boolChanged(QtProperty* prop);
  void intChanged(QtProperty* prop);
  virtual void doubleChanged(QtProperty* prop);
  /// Called when one of the parameter values gets changed
  void parameterChanged(QtProperty* prop);
  void stringChanged(QtProperty* prop);
  void filenameChanged(QtProperty* prop);
  void columnChanged(QtProperty* prop);
  void currentItemChanged(QtBrowserItem*);
  void vectorDoubleChanged(QtProperty* prop);
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
  void clearAllPlots();
  void saveFunction();
  void loadFunction();
  void loadFunctionFromString();
  void acceptFit();
  void closeFit();
  void copy();///< Copy the function string to the clipboard
  void paste();///< Paste a function string from the clipboard
  void reset();///< reset the function part, renew function, all handlers are new
  void functionHelp(); ///< Open a web page with description of the current function
  void browserHelp();  ///< Open a web page with description of FitPropertyBrowser

  void popupMenu(const QPoint &);
  /* Context menu slots */
  void addFunction();
  void deleteFunction();
  void setupMultifit();

  /// Process and create some output if it is a MultiBG fit
  void processMultiBGResults();

  void executeCustomSetupLoad(const QString& name);
  void executeCustomSetupRemove(const QString& name);

  /// Update structure tooltips for all functions
  void updateStructureTooltips();

protected:
  /// actions to do before the browser made visible
  virtual void showEvent(QShowEvent* e);
  /// actions to do before the browser is hidden
  virtual void hideEvent(QHideEvent* e);  
  /// Get and store available workspace names
  void populateWorkspaceNames();
  /// Create editors and assign them to the managers
  void createEditors(QWidget *w);
  ///
  void initLayout(QWidget *w);
  ///
  void updateDecimals();
  /// Sets the workspace to a function
  void setWorkspace(boost::shared_ptr<Mantid::API::IFunction> f)const;
  /// Display properties relevant to the selected workspace
  void setWorkspaceProperties();

  /// Create a double property and set some settings
  QtProperty* addDoubleProperty(const QString& name, QtDoublePropertyManager *manager = NULL)const;
  /// Called when the minimizer changes. Creates minimizes's properties.
  void minimizerChanged();
  /// Do the fitting
  void doFit(int maxIterations);

  /// Property managers:
  QtGroupPropertyManager  *m_groupManager;
  QtDoublePropertyManager *m_doubleManager;
  QtEnumPropertyManager *m_enumManager;
  QtIntPropertyManager *m_intManager;
  QtBoolPropertyManager *m_boolManager;
  QtStringPropertyManager *m_stringManager;
  QtStringPropertyManager *m_filenameManager;
  QtStringPropertyManager *m_formulaManager;
  QtEnumPropertyManager *m_columnManager;
  QtGroupPropertyManager  *m_vectorManager;
  QtIntPropertyManager *m_vectorSizeManager;
  QtDoublePropertyManager *m_vectorDoubleManager;
  ParameterPropertyManager *m_parameterManager;

  QtProperty *m_workspace;
  QtProperty *m_workspaceIndex;
  QtProperty *m_startX;
  QtProperty *m_endX;
  QtProperty *m_output;
  QtProperty *m_minimizer;
  QtProperty *m_ignoreInvalidData;
  QtProperty *m_costFunction;
  QtProperty *m_maxIterations;
  QtProperty *m_logValue;
  QtProperty *m_plotDiff;
  QtProperty *m_plotCompositeMembers;
  QtProperty *m_convolveMembers;
  QtProperty *m_rawData;
  QtProperty *m_xColumn;
  QtProperty *m_yColumn;
  QtProperty *m_errColumn;
  QtProperty *m_showParamErrors;
  QList<QtProperty*> m_minimizerProperties;

  /// A copy of the edited function
  boost::shared_ptr<Mantid::API::CompositeFunction> m_compositeFunction;

  QtTreePropertyBrowser* m_browser;

  QAction* m_fitActionUndoFit;
  QAction* m_fitActionSeqFit;
  QAction* m_fitActionFit;
  QAction* m_fitActionEvaluate;

  /// Group for functions
  QtBrowserItem* m_functionsGroup;
  /// Group for input/output settings
  QtBrowserItem* m_settingsGroup;
  /// Group for custom options available on muon analysis widget
  QtBrowserItem* m_customSettingsGroup;

  /// If false the change-slots (such as enumChanged(), doubleChanged()) are disabled
  bool m_changeSlotsEnabled;
  /// if true the output name will be guessed every time workspace name is changeed
  bool m_guessOutputName;
  /// Check if the input workspace is a group
  bool isWorkspaceAGroup()const;

  /// A list of registered functions
  mutable QStringList m_registeredFunctions;
  /// A list of registered peaks
  mutable QStringList m_registeredPeaks;
  /// A list of registered backgrounds
  mutable QStringList m_registeredBackgrounds;
  /// A list of registered functions that are neither peaks nor backgrounds
  mutable QStringList m_registeredOther;
  /// A list of available minimizers
  mutable QStringList m_minimizers;  
  /// A list of available workspaces
  mutable QStringList m_workspaceNames;
  /// A list of available cost functions
  mutable QStringList m_costFunctions;

  /// To keep a copy of the initial parameters in case for undo fit
  std::vector<double> m_initialParameters;

private:
  /// load and save function
  void loadFunction(const QString& funcString);
  void saveFunction(const QString& fnName);
  /// Create CompositeFunction
  void createCompositeFunction(const QString& str = "");
  /// Check if the workspace can be used in the fit
  virtual bool isWorkspaceValid(Mantid::API::Workspace_sptr)const;
  /// Find QtBrowserItem for a property prop among the chidren of 
  QtBrowserItem* findItem(QtBrowserItem* parent,QtProperty* prop)const;
  /// Set the parameters to the fit outcome
  void getFitResults();
  /// disable undo when the function changes
  void disableUndo();
  /// Enable/disable the Fit button;
  virtual void setFitEnabled(bool yes);
  /// Create a string property and set some settings
  QtProperty* addStringProperty(const QString& name)const;
  void setStringPropertyValue(QtProperty* prop,const QString& value)const;
  QString getStringPropertyValue(QtProperty* prop)const;
  /// Check that the properties match the function
  void checkFunction();

  void setCurrentFunction(Mantid::API::IFunction_const_sptr f)const;

  /// Sets the new workspace to the current one
  virtual void workspaceChange(const QString& wsName);

  /// Does a parameter have a tie
  void hasConstraints(QtProperty* parProp,bool& hasTie,bool& hasBounds)const;
  /// Returns the tie property for a parameter property, or NULL
  QtProperty* getTieProperty(QtProperty* parProp)const;

  /// Callback for FunctionFactory update notifications
  void handleFactoryUpdate(Mantid::API::FunctionFactoryUpdateNotification_ptr);
  /// Observes algorithm factory update notifications
  Poco::NObserver<FitPropertyBrowser,
                  Mantid::API::FunctionFactoryUpdateNotification> m_updateObserver;


  /// Make sure m_groupMember belongs to the group
  //void validateGroupMember();

  /// Fit and Display menu
  QSignalMapper* m_fitMapper;
  QMenu* m_fitMenu;
  QAction* m_displayActionPlotGuess;
  QAction* m_displayActionQuality;
  QAction* m_displayActionClearAll;
  QString m_windowBaseString;

  /// Setup menu
  QAction* m_setupActionCustomSetup;
  QAction* m_setupActionRemove; 
  void updateSetupMenus();

  /// To display a tip text
  QLabel* m_tip;

  // The widget for choosing the fit function.
  QDialog* m_fitSelector;
  // The tree widget containing the fit functions.
  QTreeWidget* m_fitTree;
  
  /// String property managers for special case attributes such as Filename or Formula
  /// <attribute_name,string_manager>
  QMap<QString,QtStringPropertyManager*> m_stringManagers;

  mutable PropertyHandler* m_currentHandler;

  /// A list of available data types
  mutable QStringList m_dataTypes;

  /// Default function name
  std::string m_defaultFunction;
  /// Default peak name
  std::string m_defaultPeak;
  /// Default background name
  std::string m_defaultBackground;

  /// The current function index
  int m_index_;

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
  //std::string m_groupMember;

  /// Log names
  QStringList m_logs;

  /// Number of decimal places in double properties
  int m_decimals;

  /// holds effectively a MantidUI for connecting
  QObject* m_mantidui;

  /// store current workspace name
  std::string m_storedWorkspaceName;

  friend class PropertyHandler;
  friend class CreateAttributeProperty;
  friend class SetAttribute;
  friend class SetAttributeProperty;
  friend class SequentialFitDialog;

};


} // MantidQt
} // API

#endif /*FITPROPERTYBROWSER_H_*/
