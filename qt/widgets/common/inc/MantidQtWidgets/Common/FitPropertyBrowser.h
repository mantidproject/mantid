// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FITPROPERTYBROWSER_H_
#define FITPROPERTYBROWSER_H_

#include "DllOption.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/Workspace_fwd.h"

#include <QDockWidget>
#include <QHash>
#include <QList>
#include <QMap>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IWorkspaceFitControl.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"

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

namespace MantidQt {
namespace MantidWidgets {

class PropertyHandler;
/**
 * Class FitPropertyBrowser implements QtPropertyBrowser to display
 * and control fitting function parameters and settings.
 *
 * @date 13/11/2009
 */

class EXPORT_OPT_MANTIDQT_COMMON FitPropertyBrowser
    : public QDockWidget,
      public Mantid::API::AlgorithmObserver,
      public MantidQt::API::WorkspaceObserver,
      public IWorkspaceFitControl {
  Q_OBJECT
public:
  /// Constructor
  FitPropertyBrowser(QWidget *parent = nullptr, QObject *mantidui = nullptr);
  /// Destructor
  ~FitPropertyBrowser() override;
  /// Get handler to the root composite function
  PropertyHandler *getHandler() const;
  /// Initialise layout
  virtual void init();

  /// Centre of the current peak
  double centre() const;
  /// Set centre of the current peak
  void setCentre(double value);
  /// Height of the current peak
  double height() const;
  /// Set height of the current peak
  void setHeight(double value);
  /// Width of the current peak
  double fwhm() const;
  /// Set width of the current peak
  void setFwhm(double value);
  /// Get count
  int count() const;
  /// Is the current function a peak?
  bool isPeak() const;
  /// Get the current function
  PropertyHandler *currentHandler() const;
  /// Set new current function
  void setCurrentFunction(PropertyHandler *h) const;
  /// Get the current function
  boost::shared_ptr<const Mantid::API::IFunction> theFunction() const;
  /// Update the function parameters
  void updateParameters();
  /// Update the function attributes
  void updateAttributes();
  /// Get function parameter values
  QList<double> getParameterValues() const;
  /// Get function parameter names
  QStringList getParameterNames() const;

  /// Load function
  void loadFunction(const QString &funcString);
  /// Create a new function
  PropertyHandler *addFunction(const std::string &fnName);

  /// Removes the function held by the property handler
  virtual void removeFunction(PropertyHandler *handler);

  /// Get Composite Function
  boost::shared_ptr<Mantid::API::CompositeFunction> compositeFunction() const {
    return m_compositeFunction;
  }

  /// Return the fitting function
  Mantid::API::IFunction_sptr getFittingFunction() const;
  /// Return a function at a specific index in the composite function
  Mantid::API::IFunction_sptr
  getFunctionAtIndex(std::size_t const &index) const;

  /// Get the default function type
  std::string defaultFunctionType() const;
  /// Set the default function type
  void setDefaultFunctionType(const std::string &fnType);
  /// Get the default peak type
  std::string defaultPeakType() const;
  /// Set the default peak type
  void setDefaultPeakType(const std::string &fnType);
  /// Get the default background type
  std::string defaultBackgroundType() const;
  /// Set the default background type
  void setDefaultBackgroundType(const std::string &fnType);

  /// Get the workspace
  boost::shared_ptr<Mantid::API::Workspace> getWorkspace() const;
  /// Get the input workspace name
  std::string workspaceName() const;
  /// Set the input workspace name
  virtual void setWorkspaceName(const QString &wsName) override;
  /// Get workspace index
  int workspaceIndex() const;
  /// Set workspace index
  void setWorkspaceIndex(int i) override;
  /// Get the output name
  virtual std::string outputName() const;
  /// Set the output name
  void setOutputName(const std::string & /*name*/);
  /// Get the minimizer
  std::string minimizer(bool withProperties = false) const;
  /// Get the ignore invalid data option
  bool ignoreInvalidData() const;
  /// Set the ignore invalid data option
  void setIgnoreInvalidData(bool on);
  /// Get the cost function
  std::string costFunction() const;
  /// Get the "ConvolveMembers" option
  bool convolveMembers() const;
  /// Get "HistogramFit" option
  bool isHistogramFit() const;
  /// Set if the data must be normalised before fitting
  void normaliseData(bool on) { m_shouldBeNormalised = on; }
  /// Get the max number of iterations
  int maxIterations() const;
  /// Get the peak radius for peak functions
  int getPeakRadius() const;

  /// Get the start X
  double startX() const;
  /// Set the start X
  void setStartX(double start) override;
  /// Get the end X
  double endX() const;
  /// Set the end X
  void setEndX(double end) override;
  /// Set both start and end X
  void setXRange(double start, double end);
  /// Set LogValue for PlotPeakByLogValue
  void setLogValue(const QString &lv = "");
  /// Get LogValue
  std::string getLogValue() const;
  /// Remove LogValue from the browser
  void removeLogValue();

  /// Return a list of registered functions
  const QStringList &registeredFunctions() const {
    return m_registeredFunctions;
  }
  /// Return a list of registered peaks
  const QStringList &registeredPeaks() const { return m_registeredPeaks; }
  /// Return a list of registered backgrounds
  const QStringList &registeredBackgrounds() const {
    return m_registeredBackgrounds;
  }
  /// Return a list of registered other functions
  const QStringList &registeredOthers() const { return m_registeredOther; }

  /// Tells if undo can be done
  bool isUndoEnabled() const;
  /// Returns true if the function is ready for a fit
  bool isFitEnabled() const;

  /// Enable/disable the Fit buttons;
  virtual void setFitEnabled(bool enable);

  /// Display a tip
  void setTip(const QString &txt);

  /// alter text of Plot Guess
  void setTextPlotGuess(const QString text);

  /// Creates the "Ties" property value for the Fit algorithm
  QString getTieString() const;

  /// Creates the "Constraints" property value for the Fit algorithm
  QString getConstraintsString() const;

  // send parameterChanged signal
  void sendParameterChanged(const Mantid::API::IFunction *f) {
    emit parameterChanged(f);
  }

  // send parameterChanged signal
  void sendParameterChanged(const QString &prefix) {
    emit changedParameterOf(prefix);
  }

  /// Creates and adds the autobackground
  void addAutoBackground();
  bool isAutoBack() const { return m_autoBackground != nullptr; }
  void setAutoBackgroundName(const QString &aName);
  void refitAutoBackground();
  QString getAutoBackgroundString() const {
    return m_autoBgName + " " + m_autoBgAttributes;
  }

  /// Number of decimal places in double properties
  int getDecimals() const { return m_decimals; }
  void setDecimals(int d);

  /// Returns true if the difference plot should be drawn
  bool plotDiff() const;
  /// Returns true if a composite's member functions should be plotted also
  bool plotCompositeMembers() const;

  /// Returns true if the fit should be done against binned (bunched) data.
  bool rawData() const override;

  void setADSObserveEnabled(bool enabled);

  void postDeleteHandle(const std::string &wsName) override;
  void addHandle(const std::string &wsName,
                 const boost::shared_ptr<Mantid::API::Workspace> ws) override;

  /// Called when the Fit is finished
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;

  /// Returns the list of workspaces that are currently been worked on by the
  /// fit property browser.
  QStringList getWorkspaceNames();
  /// Create a MatrixWorkspace from a TableWorkspace
  Mantid::API::Workspace_sptr createMatrixFromTableWorkspace() const;

  /// Allow or disallow sequential fits (depending on whether other conditions
  /// are met)
  void allowSequentialFits(bool allow) override;

  /// Return the Fit menu. This gives Python access to events emitted by this
  /// menu.
  QMenu *getFitMenu() const { return m_fitMenu; }

  // Methods intended for testing only

  int sizeOfFunctionsGroup() const;

  // Methods intended for interfacing with the workbench fitting tools

  void addAllowedSpectra(const QString &wsName, const QList<int> &wsIndices);
  QString addFunction(const QString &fnName);
  PropertyHandler *getPeakHandler(const QString &prefix);
  void setPeakCentreOf(const QString &prefix, double value);
  double getPeakCentreOf(const QString &prefix);
  void setPeakHeightOf(const QString &prefix, double value);
  double getPeakHeightOf(const QString &prefix);
  void setPeakFwhmOf(const QString &prefix, double value);
  double getPeakFwhmOf(const QString &prefix);
  QStringList getPeakPrefixes() const;

public slots:
  virtual void fit();
  virtual void sequentialFit();
  void undoFit();
  virtual void clear();
  void clearBrowser();
  void setPeakToolOn(bool on);
  void findPeaks();
  virtual void executeFitMenu(const QString & /*item*/);
  void executeDisplayMenu(const QString & /*item*/);
  void executeSetupMenu(const QString & /*item*/);
  void executeSetupManageMenu(const QString & /*item*/);

signals:
  void currentChanged() const;
  void functionRemoved();
  void algorithmFinished(const QString & /*_t1*/);
  void workspaceIndexChanged(int index);
  void updatePlotSpectrum(int index);
  void workspaceNameChanged(const QString & /*_t1*/);

  void wsChangePPAssign(const QString & /*_t1*/);
  void functionChanged();

  void startXChanged(double /*_t1*/);
  void endXChanged(double /*_t1*/);
  void xRangeChanged(double /*_t1*/, double /*_t2*/);
  void parameterChanged(const Mantid::API::IFunction * /*_t1*/);
  void changedParameterOf(const QString &prefix);
  void functionCleared();
  void plotGuess();
  void plotCurrentGuess();
  void removeGuess();
  void removeCurrentGuess();
  void changeWindowTitle(const QString & /*_t1*/);
  void removePlotSignal(MantidQt::MantidWidgets::PropertyHandler * /*_t1*/);
  void removeFitCurves();

  void executeFit(QString /*_t1*/, QHash<QString, QString> /*_t2*/,
                  Mantid::API::AlgorithmObserver * /*_t3*/);
  void multifitFinished();

  /// signal which can optionally be caught for customization after a fit has
  /// been done
  void fittingDone(const QString & /*_t1*/);
  void functionFactoryUpdateReceived();
  void errorsEnabled(bool enabled);
  void fitUndone();
  void functionLoaded(const QString & /*_t1*/);
  void fitResultsChanged(const QString &status);

protected slots:
  /// Get the registered function names
  virtual void populateFunctionNames();
  /// Called when a bool property is changed
  virtual void boolChanged(QtProperty *prop);

  virtual void enumChanged(QtProperty *prop);

  virtual void intChanged(QtProperty *prop);

  virtual void doubleChanged(QtProperty *prop);

private slots:
  /// Called when one of the parameter values gets changed
  void parameterChanged(QtProperty *prop);
  void stringChanged(QtProperty *prop);
  void filenameChanged(QtProperty *prop);
  void columnChanged(QtProperty *prop);
  void currentItemChanged(QtBrowserItem * /*current*/);
  void vectorDoubleChanged(QtProperty *prop);
  void vectorSizeChanged(QtProperty *prop);
  void addTie();
  void addTieToFunction();
  void addFixTie();
  void deleteTie();
  void addLowerBound10();
  void addLowerBound50();
  void addLowerBound();
  void addConstraint(int f, bool lo, bool up);
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
  void copy();  ///< Copy the function string to the clipboard
  void paste(); ///< Paste a function string from the clipboard
  void
  reset(); ///< reset the function part, renew function, all handlers are new
  void
  functionHelp(); ///< Open a web page with description of the current function
  void
  browserHelp(); ///< Open a web page with description of FitPropertyBrowser

  void popupMenu(const QPoint & /*unused*/);
  /* Context menu slots */
  void addFunction();
  void deleteFunction();
  void setupMultifit();

  /// Process and create some output if it is a MultiBG fit
  void processMultiBGResults();

  void executeCustomSetupLoad(const QString &name);
  void executeCustomSetupRemove(const QString &name);

  /// Update structure tooltips for all functions
  void updateStructureTooltips();
  /// Display the status string returned from Fit
  void showFitResultStatus(const QString &status);
  /// Clear the Fit status display
  void clearFitResultStatus();

protected:
  void modifyFitMenu(QAction *fitAction, bool enabled);
  virtual void populateFitMenuButton(QSignalMapper *fitMapper, QMenu *fitMenu);
  bool getShouldBeNormalised() { return m_shouldBeNormalised; };
  /// actions to do before the browser made visible
  void showEvent(QShowEvent *e) override;
  /// actions to do before the browser is hidden
  void hideEvent(QHideEvent *e) override;
  /// Get and store available workspace names
  void populateWorkspaceNames();
  /// Create editors and assign them to the managers
  void createEditors(QWidget *w);
  ///
  void initLayout(QWidget *w);
  ///
  void initBasicLayout(QWidget *w);
  ///
  void updateDecimals();
  /// Sets the workspace to a function
  void setWorkspace(boost::shared_ptr<Mantid::API::IFunction> f) const;
  /// Display properties relevant to the selected workspace
  void setWorkspaceProperties();
  /// Adds the workspace index property to the browser.
  virtual void addWorkspaceIndexToBrowser();
  /// Set the parameters to the fit outcome
  void getFitResults();
  /// Create a double property and set some settings
  QtProperty *
  addDoubleProperty(const QString &name,
                    QtDoublePropertyManager *manager = nullptr) const;
  /// Called when the minimizer changes. Creates minimizes's properties.
  void minimizerChanged();
  /// Do the fitting
  void doFit(int maxIterations);
  /// Create CompositeFunction from string
  void createCompositeFunction(const QString &str = "");
  /// Catches unexpected not found exceptions
  Mantid::API::IFunction_sptr tryCreateFitFunction(const QString &str);
  /// Create CompositeFunction from pointer
  void createCompositeFunction(const Mantid::API::IFunction_sptr func);

  /// Property managers:
  QtGroupPropertyManager *m_groupManager;
  QtDoublePropertyManager *m_doubleManager;
  QtEnumPropertyManager *m_enumManager;
  QtIntPropertyManager *m_intManager;
  QtBoolPropertyManager *m_boolManager;
  QtStringPropertyManager *m_stringManager;
  QtStringPropertyManager *m_filenameManager;
  QtStringPropertyManager *m_formulaManager;
  QtEnumPropertyManager *m_columnManager;
  QtGroupPropertyManager *m_vectorManager;
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
  QtProperty *m_peakRadius;
  QtProperty *m_logValue;
  QtProperty *m_plotDiff;
  QtProperty *m_plotCompositeMembers;
  QtProperty *m_convolveMembers;
  QtProperty *m_rawData;
  QtProperty *m_xColumn;
  QtProperty *m_yColumn;
  QtProperty *m_errColumn;
  QtProperty *m_showParamErrors;
  QtProperty *m_evaluationType;
  QList<QtProperty *> m_minimizerProperties;

  /// A copy of the edited function
  boost::shared_ptr<Mantid::API::CompositeFunction> m_compositeFunction;

  QtTreePropertyBrowser *m_browser;

  QAction *m_fitActionUndoFit;
  QAction *m_fitActionSeqFit;
  QAction *m_fitActionFit;
  QAction *m_fitActionEvaluate;

  /// Group for functions
  QtBrowserItem *m_functionsGroup;
  /// Group for input/output settings
  QtBrowserItem *m_settingsGroup;
  /// Group for custom options available on muon analysis widget
  QtBrowserItem *m_customSettingsGroup;

  /// If false the change-slots (such as enumChanged(), doubleChanged()) are
  /// disabled
  bool m_changeSlotsEnabled;
  /// if true the output name will be guessed every time workspace name is
  /// changeed
  bool m_guessOutputName;
  /// Check if the input workspace is a group
  bool isWorkspaceAGroup() const;

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
  /// A list of possible function evaluation types
  mutable QStringList m_evaluationTypes;

  /// To keep a copy of the initial parameters in case for undo fit
  std::vector<double> m_initialParameters;

private:
  ///
  QPushButton *createFitMenuButton(QWidget *w);
  /// save function
  void saveFunction(const QString &fnName);
  /// Check if the workspace can be used in the fit
  virtual bool isWorkspaceValid(Mantid::API::Workspace_sptr /*ws*/) const;
  /// Find QtBrowserItem for a property prop among the chidren of
  QtBrowserItem *findItem(QtBrowserItem *parent, QtProperty *prop) const;

  /// disable undo when the function changes
  void disableUndo();
  /// Create a string property and set some settings
  QtProperty *addStringProperty(const QString &name) const;
  void setStringPropertyValue(QtProperty *prop, const QString &value) const;
  QString getStringPropertyValue(QtProperty *prop) const;
  /// Check that the properties match the function
  void checkFunction();
  /// Return the nearest allowed workspace index.
  int getAllowedIndex(int currentIndex) const;

  void setCurrentFunction(Mantid::API::IFunction_const_sptr f) const;

  /// Sets the new workspace to the current one
  virtual void workspaceChange(const QString &wsName);

  /// Does a parameter have a tie
  void hasConstraints(QtProperty *parProp, bool &hasTie, bool &hasBounds) const;
  /// Returns the tie property for a parameter property, or NULL
  QtProperty *getTieProperty(QtProperty *parProp) const;

  /// Callback for FunctionFactory update notifications
  void handleFactoryUpdate(
      Mantid::API::FunctionFactoryUpdateNotification_ptr /*notice*/);
  /// Observes algorithm factory update notifications
  Poco::NObserver<FitPropertyBrowser,
                  Mantid::API::FunctionFactoryUpdateNotification>
      m_updateObserver;

  /// Make sure m_groupMember belongs to the group
  // void validateGroupMember();

  /// Fit and Display menu
  QSignalMapper *m_fitMapper;
  QMenu *m_fitMenu;
  QAction *m_displayActionPlotGuess;
  QAction *m_displayActionQuality;
  QAction *m_displayActionClearAll;
  QString m_windowBaseString;

  /// Setup menu
  QAction *m_setupActionCustomSetup;
  QAction *m_setupActionRemove;
  void updateSetupMenus();

  /// To display a tip text
  QLabel *m_tip;
  /// To display fit status
  QLabel *m_status;

  // The widget for choosing the fit function.
  QDialog *m_fitSelector;
  // The tree widget containing the fit functions.
  QTreeWidget *m_fitTree;

  /// String property managers for special case attributes such as Filename or
  /// Formula
  /// <attribute_name,string_manager>
  QMap<QString, QtStringPropertyManager *> m_stringManagers;

  mutable PropertyHandler *m_currentHandler;

  /// A list of available data types
  mutable QStringList m_dataTypes;

  /// Default function name
  std::string m_defaultFunction;
  /// Default peak name
  std::string m_defaultPeak;
  /// Default background name
  std::string m_defaultBackground;

  /// Shows if the PeakPickerTool is on
  bool m_peakToolOn;

  /// If true background function will be included automatically
  bool m_auto_back;

  /// Name of the autobackground function
  QString m_autoBgName;
  /// List of attributes of the autobackground function as name=value pairs
  /// separated by spaces
  QString m_autoBgAttributes;

  /// The autobackground handler
  PropertyHandler *m_autoBackground;

  /// Log names
  QStringList m_logs;

  /// Number of decimal places in double properties
  int m_decimals;

  /// holds effectively a MantidUI for connecting
  QObject *m_mantidui;

  /// store current workspace name
  std::string m_storedWorkspaceName;

  /// Should the data be normalised before fitting?
  bool m_shouldBeNormalised;

  /// If non-empty it contains references to the spectra
  /// allowed to be fitted in this browser:
  ///   keys are workspace names,
  ///   values are lists of workspace indices
  QMap<QString, QList<int>> m_allowedSpectra;
  /// Store workspace index to revert to in case validation fails
  int m_oldWorkspaceIndex;

  friend class PropertyHandler;
  friend class CreateAttributeProperty;
  friend class SetAttribute;
  friend class SetAttributeProperty;
  friend class SequentialFitDialog;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*FITPROPERTYBROWSER_H_*/
