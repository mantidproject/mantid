// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// This file defines the Python bindings for the
// _common library.
%Module(name=_commonqt5)

%ModuleCode
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/Message.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include "frameobject.h"

#if defined(__GNUC__) && !defined(__clang__)
  // We need default visibility for this module so that
  // python can see the init function.
  #pragma GCC visibility push(default)
#endif
MSVC_DIAG_OFF(4290)
GNU_DIAG_OFF("pedantic")
GNU_DIAG_OFF("suggest-override")
GNU_DIAG_OFF("missing-field-initializers")
GNU_DIAG_OFF("maybe-uninitialized")

// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
%End

// Remove namespace qualifiers in generated code. We control
// the package layout separately in Python
%HideNamespace(name=MantidQt)
%HideNamespace(name=MantidQt::MantidWidgets)
%HideNamespace(name=MantidQt::MantidWidgets::Batch)

// PyQt5 widgets
%Import QtGui/QtGuimod.sip
%Import QtWidgets/QtWidgetsmod.sip

%Include ../sip/exceptions.sip
%Include ../sip/string.sip
%Include ../sip/vector.sip

%InitialisationCode
qRegisterMetaType<std::string>("StdString");
qRegisterMetaType<Mantid::API::Workspace_sptr>("Workspace");
qRegisterMetaType<Message>("Message");
qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>");
qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");
qRegisterMetaType<QVector<int>>("QVector<int>");
%End


// ----------------------------------------------------------------------------
// Classes
// ----------------------------------------------------------------------------

class Configurable {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Configurable.h"
%End

public:
  void readSettings(const QSettings &storage);
  void writeSettings(QSettings &storage);

private:
  // Not constructible or copyable
  Configurable();
  Configurable(const Configurable&);
};

class AlgorithmInputHistoryImpl : Configurable /PyName=AlgorithmInputHistory/ {
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
%End

public:
static SIP_PYOBJECT Instance() const;
%MethodCode
  auto &cppInstance = MantidQt::API::AlgorithmInputHistory::Instance();
  return sipConvertFromType(&cppInstance, sipType_AlgorithmInputHistoryImpl, nullptr);
%End

private:
  // lifetime management is with C++ so don't generate any of that in Python
  AlgorithmInputHistoryImpl();
  ~AlgorithmInputHistoryImpl();
  AlgorithmInputHistoryImpl(const AlgorithmInputHistoryImpl &);
};

class MessageDisplay : QWidget, Configurable {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MessageDisplay.h"
%End

public:
  MessageDisplay(QWidget *parent = 0);
  MessageDisplay(const QFont &font, QWidget *parent = 0);
  void attachLoggingChannel(int logLevel = -1);
  QPlainTextEdit *getTextEdit();
  void filterMessages();
  QMenu *generateContextMenu();
  void filePathModified(const QString &oldPath, const QString &newPath);

  bool showFrameworkOutput();
  void setShowFrameworkOutput(const bool &show);
  bool showAllScriptOutput();
  void setShowAllScriptOutput(const bool &show);
  bool showActiveScriptOutput();
  void setShowActiveScriptOutput(const bool &show);
  QString activeScript();
  void setActiveScript(const QString &scriptPath);

  void appendFatal(const QString &text);
  void appendError(const QString &text);
  void appendWarning(const QString &text);
  void appendNotice(const QString &text);
  void appendInformation(const QString &text);
  void appendDebug(const QString &text);
  void appendPython(const QString &text, const int &priority, const QString &fileName);
  void setSource(const QString &source);
};

class ScriptEditor : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/ScriptEditor.h"
%End

public:
  enum AutoCompletionSource {
      AcsNone,
      AcsAll,
      AcsDocument,
      AcsAPIs
  };

  enum WhitespaceVisibility {
     //! Whitespace is invisible.
     WsInvisible = SCWS_INVISIBLE,

     //! Whitespace is always visible.
     WsVisible = SCWS_VISIBLEALWAYS,

     //! Whitespace is visible after the whitespace used for indentation.
     WsVisibleAfterIndent = SCWS_VISIBLEAFTERINDENT
  };

public:
  ScriptEditor(const QString &lexer, const QFont &font = QFont(),
               QWidget *parent /TransferThis/ = 0) throw(std::invalid_argument);

  QString fileName() const;
  SIP_PYTUPLE getSelection() const;
%MethodCode
  int lineFrom(-1), indexFrom(-1), lineTo(-1), indexTo(-1);
  sipCpp->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  sipRes = sipBuildResult(nullptr, "(iiii)", lineFrom, indexFrom,
                          lineTo, indexTo);
%End
  int getZoom() const;
  bool hasSelectedText() const;
  bool isModified() const;
  bool isReadOnly() const;
  QString selectedText() const;
  QString text() const;

  void clearKeyBinding(const QString &keyCombination) throw(std::invalid_argument);
  void enableAutoCompletion(AutoCompletionSource source);
  void disableAutoCompletion();
  void getCursorPosition(int *line, int *index) const;
  void selectAll(bool select = true);
  void setCaretLineBackgroundColor (const QColor & col);
  void setCaretLineVisible(bool enable);
  void setCursorPosition(int line, int index);
  void setIndentationsUseTabs(bool tabs);
  void setFileName(const QString &filename);
  void setMarginWidth(int margin, int width);
  void setModified(bool m);
  void setReadOnly(bool ro);
  void setSelection(int lineFrom, int indexFrom, int lineTo, int indexTo);
  void setTabWidth(int width);
  void setText(const QString &text);
  void setEolVisibility(bool state);
  void setWhitespaceVisibility(WhitespaceVisibility mode);
  void updateCompletionAPI(const QStringList & completions);

  int findFirst(const QString &expr, bool re, bool cs, bool wo, bool wrap, bool forward);
  int findNext();
  void replace(const QString &replaceStr);
  void replaceAll(const QString &search, const QString &replace,
                              bool regex, bool caseSensitive, bool matchWords,
                              bool wrap, bool backward);
public slots:
  void updateProgressMarker(int lineno, bool error);
  void zoomTo(int level);

signals:
  void fileNameChanged(const QString &fileName);
  void modificationChanged(bool m);
  void textZoomedIn();
  void textZoomedOut();
  void progressMade(const int);
  void editorFocusIn(const QString &filename);

private:
  ScriptEditor(const ScriptEditor&);
};

class AlgorithmProgressWidget : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"
%End

public:
  AlgorithmProgressWidget(QWidget *parent /TransferThis/ = 0);
  void blockUpdates(bool block = true);
};

class CodeExecution {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Python/CodeExecution.h"
using MantidQt::Widgets::Common::Python::CodeExecution;
%End

public:
  CodeExecution(ScriptEditor*);
  SIP_PYOBJECT execute(const QString &, const QString &, int, SIP_PYOBJECT, int);
private:
  CodeExecution(const CodeExecution&);
};

class AlgorithmDialog: QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidAPI/AlgorithmObserver.h"
#include <boost/python/extract.hpp>
using namespace MantidQt::API;
%End
public:
  void setShowKeepOpen(const bool showOption);
  bool isShowKeepOpen() const;
  void initializeLayout();
  bool isInitialized() const;
  void setAlgorithm(SIP_PYOBJECT);
%MethodCode
  sipCpp->setAlgorithm(boost::python::extract<Mantid::API::IAlgorithm_sptr>(a0));
%End
  void setPresetValues(const QHash<QString, QString> &presetValues);
  void isForScript(bool forScript);
  void executeOnAccept(bool on);
  void setOptionalMessage(const QString &message);
  void addEnabledAndDisableLists(const QStringList &enabled,
                                 const QStringList &disabled);
  void addAlgorithmObserver(SIP_PYOBJECT);
%MethodCode
  sipCpp->addAlgorithmObserver(boost::python::extract<Mantid::API::AlgorithmObserver*>(a0));
%End
private:
  AlgorithmDialog();
  AlgorithmDialog(const AlgorithmDialog&);
};


class GenericDialog: AlgorithmDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/GenericDialog.h"
using namespace MantidQt::API;
%End
public:
  GenericDialog(QWidget *parent = nullptr);
};

class UserSubWindow : QMainWindow {
%TypeHeaderCode
#include "MantidQtWidgets/Common/UserSubWindow.h"
using namespace MantidQt::API;
%End
private:
  UserSubWindow(QWidget *parent = nullptr);
};

class BaseEncoder {
%TypeHeaderCode
#include "MantidQtWidgets/Common/BaseEncoder.h"
%End
public:
  virtual QMap<QString, QVariant> encode(const QWidget *window,
                                         const std::string &directory) = 0;
  virtual QList<QString> tags() = 0;
private:
  BaseEncoder();
  virtual ~BaseEncoder();
};

class BaseDecoder {
%TypeHeaderCode
#include "MantidQtWidgets/Common/BaseDecoder.h"
%End
public:
  virtual QWidget * decode(const QMap<QString, QVariant> &map,
                           const std::string &directory) = 0;
  virtual QList<QString> tags() = 0;
private:
  BaseDecoder();
  virtual ~BaseDecoder();
};

class UserSubWindowFactoryImpl /PyName=UserSubWindowFactory/ {
%TypeHeaderCode
#include "MantidQtWidgets/Common/UserSubWindowFactory.h"
%End
public:
    static SIP_PYOBJECT Instance() const;
    %MethodCode
        auto &cppInstance = MantidQt::API::UserSubWindowFactory::Instance();
        return sipConvertFromType(&cppInstance, sipType_UserSubWindowFactoryImpl, nullptr);
    %End
    UserSubWindow *createUnwrapped(const std::string &name) const;
    QSet<QString> categories(const QString &interfaceName) const;
    QStringList keys() const;
    BaseEncoder *findEncoder(QWidget *window);
    BaseDecoder *findDecoder(const std::string &decodeString);


private:
  UserSubWindowFactoryImpl();
  UserSubWindowFactoryImpl(const UserSubWindowFactoryImpl &);
  ~UserSubWindowFactoryImpl();
};


class InterfaceManager {
%TypeHeaderCode
#include "MantidQtWidgets/Common/InterfaceManager.h"
using namespace MantidQt::API;
%End
public:
  InterfaceManager() throw(std::exception);
  AlgorithmDialog *createDialogFromName(
      const QString &algorithmName, const int version = -1,
      QWidget *parent = nullptr, bool forScript = false,
      const QHash<QString, QString> &presetValues = QHash<QString, QString>(),
      const QString &optionalMsg = QString(),
      const QStringList &enabled = QStringList(),
      const QStringList &disabled = QStringList()) throw(std::exception);

  UserSubWindow *createSubWindow(
      const QString &interface_name,
      QWidget *parent = nullptr) throw(std::exception);
  QStringList getUserSubWindowKeys() const;

  void showHelpPage(const QString &url=QString());
  void showAlgorithmHelp(const QString &name, const int version=-1);
  void showConceptHelp(const QString &name);
  void showFitFunctionHelp(const QString &name=QString());
  void showCustomInterfaceHelp(const QString &name, const QString &area=QString(), const QString &section=QString());
  void showWebPage(const QString &url);
  void closeHelpWindow();
};

// ---------------------------------
// WorkspaceWidget and dependencies
// ---------------------------------

class MantidDisplayBase /Abstract/ {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MantidDisplayBase.h"
%End
private:
  MantidDisplayBase();
};

class MantidTreeModel : MantidDisplayBase {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MantidTreeModel.h"
%End
public:
  MantidTreeModel();
private:
  MantidTreeModel(const MantidTreeModel &);
};

class WorkspaceTreeWidget : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h"
%End
public:
  WorkspaceTreeWidget(MantidDisplayBase *mdb /Transfer/, bool viewOnly=false,
                      QWidget *parent /TransferThis/ = nullptr);
  std::vector<std::string> getSelectedWorkspaceNames() const;
};

// Implementation to override context menu
class WorkspaceTreeWidgetSimple : WorkspaceTreeWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
%End
public:
  WorkspaceTreeWidgetSimple(bool viewOnly=false, QWidget *parent /TransferThis/ = nullptr);
  void refreshWorkspaces();
  void enableDeletePrompt(bool enable);
  void setOverplotDisabled(bool disabled);

signals:
  void plotSpectrumClicked(const QStringList & workspaceNames);
  void overplotSpectrumClicked(const QStringList & workspaceNames);
  void plotSpectrumWithErrorsClicked(const QStringList & workspaceNames);
  void overplotSpectrumWithErrorsClicked(const QStringList & workspaceNames);
  void plotColorfillClicked(const QStringList &workspaceNames);
  void sampleLogsClicked(const QStringList &workspaceNames);
  void sliceViewerClicked(const QStringList &workspaceNames);
  void showInstrumentClicked(const QStringList &workspaceNames);
  void showAlgorithmHistoryClicked(const QStringList &workspaceNames);
  void plotMDHistoClicked(const QStringList &workspaceNames);
  void overplotMDHistoClicked(const QStringList &workspaceNames);
  void plotMDHistoWithErrorsClicked(const QStringList &workspaceNames);
  void overplotMDHistoWithErrorsClicked(const QStringList &workspaceNames);
  void sampleMaterialClicked(const QStringList &workspaceNames);
  void sampleShapeClicked(const QStringList &workspaceNames);
};

class AlgorithmHistoryWindow : public QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmHistoryWindow.h"
%End
public:
	AlgorithmHistoryWindow(QWidget *parent /TransferThis/, const QString &workspaceName) throw(std::invalid_argument);
};

// ---------------------------------
// Manage User Directories
// ---------------------------------

class ManageUserDirectories : QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
%End
public:
  ManageUserDirectories(QWidget *parent /TransferThis/ = nullptr);
  static void openManageUserDirectories();
};

// ---------------------------------
// Script Repository
// ---------------------------------

class ScriptRepositoryView : QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/ScriptRepositoryView.h"
%End
public:
    ScriptRepositoryView(QWidget *parent = nullptr);
};

// ---------------------------------
// Instrument Selector
// ---------------------------------

class InstrumentSelector : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/InstrumentSelector.h"
%End
public:
  InstrumentSelector(QWidget *parent /TransferThis/ = nullptr);
  void setTechniques(const QStringList &techniques);
  void setCurrentIndex(int);
  void setCurrentText(QString);
  QString currentText();
  int count();
};

// ---------------------------------
// Property Handler
// ---------------------------------
class PropertyHandler
    : public QObject {
%TypeHeaderCode
#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidPythonInterface/core/ExtractSharedPtr.h"
#include <boost/python/extract.hpp>
#include <boost/python/to_python_value.hpp>
%End
public:
    PropertyHandler(SIP_PYOBJECT, SIP_PYOBJECT, FitPropertyBrowser *)[(const Mantid::API::IFunction_sptr &,
 Mantid::API::CompositeFunction_sptr,  FitPropertyBrowser *)];
    %MethodCode
    auto function = boost::python::extract<const Mantid::API::IFunction_sptr &>(a0)();
    auto compositeFunction = boost::python::extract<Mantid::API::CompositeFunction_sptr>(a1)();
    sipCpp = new sipPropertyHandler(function, compositeFunction,  a2);
    %End
    SIP_PYOBJECT ifun();
    %MethodCode
    sipRes = boost::python::to_python_value<const Mantid::API::IFunction_sptr &>()(sipCpp->ifun());
    %End
    bool hasTies();
    void setHasPlot(bool);
    QString functionPrefix() const;
    PropertyHandler * findHandler(SIP_PYOBJECT);
    %MethodCode
    auto function = boost::python::extract<const Mantid::API::IFunction_sptr &>(a0)();
    sipRes = sipCpp->findHandler(function);
    %End
    QString functionName() const;
};

// ---------------------------------
// Fit Property Browser
// ---------------------------------
class FitPropertyBrowser : public QDockWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
%End
public:
    FitPropertyBrowser(QWidget *parent = nullptr, QObject *mantidui = nullptr);
    void init();
    SIP_PYOBJECT getFittingFunction() const;
%MethodCode
    try {
      auto f = sipCpp->getFittingFunction();
      sipRes = FROM_CSTRING(f->asString().c_str());
    } catch (std::out_of_range&) {
      sipRes = FROM_CSTRING("");
    }
%End
    int sizeOfFunctionsGroup() const;
    bool isFitEnabled() const;
    std::string getFunctionString() const;
    void loadFunction(QString funStr);
    void saveFunction(const QString &fnName);
    void executeCustomSetupRemove(const QString &name);
    bool ignoreInvalidData();
    double startX() const;
    void setStartX(double start);
    double endX() const;
    void setEndX(double end);
    void setXRange(double start, double end);
    QVector<double> getXRange() throw(std::invalid_argument);
    std::string getExcludeRange() const;
    QString getXColumnName() const;
    QString getYColumnName() const;
    QString getErrColumnName() const;
    int workspaceIndex() const;
    void setWorkspaceIndex(int i);
    QStringList getWorkspaceNames();
    void fit();
    void toggleSettingsBrowserVisible();
    void removePropertiesFromSettingsBrowser(const QStringList &propsToRemove);
    void toggleWsListVisible();
    void addAllowedSpectra(const QString &wsName, const QList<int> &wsIndices);
    void addAllowedTableWorkspace(const QString &wsName);
    void setTextPlotGuess(const QString);
    bool plotDiff() const;
    std::string outputName() const;
    void addFitResultWorkspacesToTableWidget();
    void setWorkspaceName(const QString &wsName);
    void postDeleteHandle(const std::string &wsName);
    void removeWorkspaceAndSpectra(const std::string &wsName);
    std::string getFitAlgorithmParameters() const;
    std::string getFitAlgorithmOutputStatus() const;
    PropertyHandler * currentHandler() const;

    SIP_PYOBJECT defaultPeakType();
%MethodCode
    auto f = sipCpp->defaultPeakType();
    sipRes = FROM_CSTRING(f.c_str());
%End
    void setDefaultPeakType(SIP_PYOBJECT);
%MethodCode
    sipCpp->setDefaultPeakType(TO_CSTRING(a0));
%End
SIP_PYOBJECT defaultBackgroundType();
%MethodCode
    auto f = sipCpp->defaultBackgroundType();
    sipRes = FROM_CSTRING(f.c_str());
%End
    QStringList registeredPeaks() const;
    QStringList registeredBackgrounds() const;
    QStringList registeredOthers() const;
    QStringList getPeakPrefixes() const;
    QString addFunction(QString);
    PropertyHandler *getPeakHandler(const QString &prefix);
    double getPeakCentreOf(QString);
    void setPeakCentreOf(QString, double);
    double getPeakHeightOf(QString);
    void setPeakHeightOf(QString, double);
    double getPeakFwhmOf(QString);
    void setPeakFwhmOf(QString, double) throw (std::invalid_argument);
    std::string getWidthParameterNameOf(QString);
    std::string getCentreParameterNameOf(QString);
    bool isParameterExplicitlySetOf(QString, std::string);
    void normaliseData(bool on);
    QMenu *getFitMenu() const;
    QListWidget* getWorkspaceList() const;

    SIP_PYOBJECT workspaceName() const;
%MethodCode
    auto f = sipCpp->workspaceName();
    sipRes = FROM_CSTRING(f.c_str());
%End

    SIP_PYOBJECT outputName() const;
%MethodCode
    auto f = sipCpp->outputName();
    sipRes = FROM_CSTRING(f.c_str());
%End

    void setOutputName(SIP_PYOBJECT);
%MethodCode
    sipCpp->setOutputName(TO_CSTRING(a0));
%End

    void setPeakToolOn(bool on);

signals:
    void functionChanged();
    void functionRemoved();
    void startXChanged(double);
    void endXChanged(double);
    void changedParameterOf(const QString &prefix);
    void algorithmFinished(const QString &);
    void removeFitCurves();
    void removeGuess();
    void removeCurrentGuess();
    void plotGuess();
    void plotCurrentGuess();
    void sequentialFitDone() const;
    void workspaceClicked(const QString &);
    void removePlotSignal(PropertyHandler *);
    void fitResultsChanged(const QString &);
};

// ---------------------------------
// Hint Strategy
// ---------------------------------
class Hint {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Hint.h"
%End
public:
  Hint(std::string word, std::string description);
  const std::string& word() const;
  const std::string& description() const;
};

class HintStrategy
{
%TypeHeaderCode
#include "MantidQtWidgets/Common/HintStrategy.h"
%End
public:
  HintStrategy();
  virtual std::vector<Hint> createHints() = 0;
  virtual ~HintStrategy();
};

class AlgorithmHintStrategy : public HintStrategy
{
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
%End
public:
  AlgorithmHintStrategy(const std::string& algorithmName,
                        std::vector<std::string> blacklist);
  virtual std::vector<Hint> createHints();
};

// ---------------------------------
// JobTreeView
// ---------------------------------

// RowLocation needs to be wrapped in a SIP namespace class
// otherwise it cannot be recognised as a type when passed into a signal
// e.g. in JobTreeViewSignalAdapter's cellTextChanged signal
// The namespace is hidden so that it is not visible to the outside world.
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class RowLocation
{
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
%End
public:
  RowLocation();
  RowLocation(std::vector<int> path);
  const std::vector<int> &path();
  int rowRelativeToParent();
  bool isRoot();
  int depth();
  bool isChildOf(const MantidQt::MantidWidgets::Batch::RowLocation &other);
  bool isSiblingOf(const MantidQt::MantidWidgets::Batch::RowLocation &other);
  bool isChildOrSiblingOf(const MantidQt::MantidWidgets::Batch::RowLocation &other);
  bool isDescendantOf(const MantidQt::MantidWidgets::Batch::RowLocation &other);
  MantidQt::MantidWidgets::Batch::RowLocation parent();
  MantidQt::MantidWidgets::Batch::RowLocation relativeTo(const MantidQt::MantidWidgets::Batch::RowLocation &ancestor);
  MantidQt::MantidWidgets::Batch::RowLocation child(int n);

  bool operator==(const MantidQt::MantidWidgets::Batch::RowLocation& other);
  bool operator!=(const MantidQt::MantidWidgets::Batch::RowLocation& other);
  bool operator<(const MantidQt::MantidWidgets::Batch::RowLocation& other);
  bool operator<=(const MantidQt::MantidWidgets::Batch::RowLocation& other);
  bool operator>(const MantidQt::MantidWidgets::Batch::RowLocation& other);
  bool operator>=(const MantidQt::MantidWidgets::Batch::RowLocation& other);
};

};
};
};

class RowPredicate {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/FilteredTreeModel.h"
%End
protected:
  RowPredicate();
  virtual bool rowMeetsCriteria(const MantidQt::MantidWidgets::Batch::RowLocation & row) const = 0;
};

class JobTreeViewSubscriber {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
%End
public:
  virtual void notifyCellTextChanged(const MantidQt::MantidWidgets::Batch::RowLocation& itemIndex,
                                     int column,
                                     const std::string& oldValue,
                                     const std::string& newValue) = 0;
  virtual void notifySelectionChanged() = 0;
  virtual void notifyRowInserted(const MantidQt::MantidWidgets::Batch::RowLocation& itemIndex) = 0;
  virtual void notifyAppendAndEditAtChildRowRequested() = 0;
  virtual void notifyAppendAndEditAtRowBelowRequested() = 0;
  virtual void notifyEditAtRowAboveRequested() = 0;
  virtual void notifyRemoveRowsRequested(
    const std::vector<MantidQt::MantidWidgets::Batch::RowLocation>& locationsOfRowsToRemove) = 0;
  virtual void notifyCopyRowsRequested() = 0;
  virtual void notifyCutRowsRequested() = 0;
  virtual void notifyPasteRowsRequested() = 0;
  virtual void notifyFilterReset() = 0;
};

class JobTreeView : public QTreeView
{

%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
%End
public:
    JobTreeView(const QStringList &columnHeadings, const Cell& defaultCellStyle,
     QWidget *parent = nullptr);

    void subscribe(JobTreeViewSubscriber *subscriber);
    MantidQt::MantidWidgets::Batch::RowLocation insertChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
     &parent, int beforeRow,
                          const std::vector<Cell> &rowText);
    MantidQt::MantidWidgets::Batch::RowLocation insertChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
    &parent, int beforeRow);
    MantidQt::MantidWidgets::Batch::RowLocation appendChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
    &parent);
    MantidQt::MantidWidgets::Batch::RowLocation appendChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
    &parentLocation,
                          const std::vector<Cell> &rowText);
    void appendAndEditAtChildRow();
    void appendAndEditAtRowBelow();
    void editAtRowAbove();
    void removeRowAt(const MantidQt::MantidWidgets::Batch::RowLocation &location);
    void removeRows(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> rowsToRemove);
    void removeAllRows() throw(std::exception);
    void replaceRows(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> replacementPoints,
                     std::vector<std::vector<Row>> replacementLocations);
    void appendSubtreesAt(const MantidQt::MantidWidgets::Batch::RowLocation& parent,
                          std::vector<std::vector<Row>> subtrees);
    void appendSubtreeAt(const MantidQt::MantidWidgets::Batch::RowLocation& parent,
                         std::vector<Row> subtree);
    void replaceSubtreeAt(const MantidQt::MantidWidgets::Batch::RowLocation &rootToRemove,
                          std::vector<Row> subtree);
    void insertSubtreeAt(const MantidQt::MantidWidgets::Batch::RowLocation& parent, int index,
                         std::vector<Row> subtree);

    void filterRowsBy(RowPredicate* predicate /Transfer/);
    void resetFilter();
    bool hasFilter() const;

    void setHintsForColumn(int column, HintStrategy* hintStrategy /Transfer/);

    bool isOnlyChildOfRoot(const MantidQt::MantidWidgets::Batch::RowLocation& location) const;
    std::vector<Cell> cellsAt(const MantidQt::MantidWidgets::Batch::RowLocation &location) const;
    void setCellsAt(const MantidQt::MantidWidgets::Batch::RowLocation &location,
                    const std::vector<Cell> &cells);
    Cell cellAt(MantidQt::MantidWidgets::Batch::RowLocation location, int column);
    void setCellAt(MantidQt::MantidWidgets::Batch::RowLocation location, int column, const Cell &cellText);
    Cell deadCell() const;

    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> selectedRowLocations() const;
    boost::optional<std::vector<std::vector<Row>>> selectedSubtrees() const;
    boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>> selectedSubtreeRoots() const;
};

class Cell {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/Cell.h"
%End
public:
  Cell(const std::string &contentText);

  Cell(const std::string &contentText, const std::string& backgroundColor, int borderThickness,
       const std::string &color, int borderOpacity, bool isEditable);

  const std::string &contentText() const;
  void setContentText(const std::string& contentText);

  const std::string &toolTip() const;
  void setToolTip(const std::string &toolTip);


  const std::string & borderColor() const;
  void setBorderColor(const std::string& borderColor);

  const std::string & backgroundColor() const;
  void setBackgroundColor(const std::string& backgroundColor);

  int borderOpacity() const;
  void setBorderOpacity(int alpha);

  int borderThickness() const;
  void setBorderThickness(int borderThickness);

  bool isEditable() const;
  void disableEditing();
  void enableEditing();
  void setEditable(bool isEditable);

  const std::string &iconFilePath() const;
  void setIconFilePath(const std::string& iconFilePath);

};

class Row {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/Row.h"
%End
public:
  Row(MantidQt::MantidWidgets::Batch::RowLocation location, std::vector<Cell> cells);
  const std::vector<Cell>& cells() const;
  const MantidQt::MantidWidgets::Batch::RowLocation& location() const;
};

class JobTreeViewSignalAdapter : public QObject, public JobTreeViewSubscriber {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/JobTreeViewSignalAdapter.h"
%End
public:
  JobTreeViewSignalAdapter(JobTreeView &view, QObject *parent = nullptr);
  virtual void notifyCellTextChanged(const MantidQt::MantidWidgets::Batch::RowLocation &itemIndex,
                                     int column,
                                     const std::string &oldValue,
                                     const std::string &newValue);
  virtual void notifySelectionChanged();
  virtual void notifyRowInserted(const MantidQt::MantidWidgets::Batch::RowLocation &itemIndex);
  virtual void notifyAppendAndEditAtChildRowRequested();
  virtual void notifyAppendAndEditAtRowBelowRequested();
  virtual void notifyEditAtRowAboveRequested();
  virtual void notifyRemoveRowsRequested(
      const std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &locationsOfRowsToRemove);
  virtual void notifyCopyRowsRequested();
  virtual void notifyCutRowsRequested();
  virtual void notifyPasteRowsRequested();
  virtual void notifyFilterReset();
signals:
  void cellTextChanged(const MantidQt::MantidWidgets::Batch::RowLocation &itemIndex,
                       int column,
                       std::string oldValue,
                       std::string newValue);
  void rowInserted(const MantidQt::MantidWidgets::Batch::RowLocation &itemIndex);
  void filterReset();
  void appendAndEditAtChildRowRequested();
  void appendAndEditAtRowBelowRequested();
  void editAtRowAboveRequested();
  void removeRowsRequested(
      const std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &locationsOfRowsToRemove);
  void copyRowsRequested();
  void cutRowsRequested();
};


// ---------------------------------
// Function Browser
// ---------------------------------
class FunctionBrowser : public QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include <boost/python/extract.hpp>
#include <boost/python/to_python_value.hpp>
using namespace MantidQt::API;
%End
public:
    FunctionBrowser(QWidget *parent = nullptr, bool multi = false);
    FunctionTreeView *view() const;
    void setStretchLastColumn(bool stretch);
    void setFunction(const std::string &funStr);
    std::string getFunctionString();
    std::string getFitFunctionString() const;
    void setParameter(const std::string &funStr, double value);
    double getParameter(const std::string &funStr);
    int getNumberOfDatasets() const;
    void setNumberOfDatasets(int n);
    void setCurrentDataset(int i) throw(std::exception);
    int getCurrentDataset() const;
    void setDatasets(const std::vector<std::string> &datasetNames);
    void addDatasets(const std::vector<std::string> &names);
    void removeDatasets(QList<int> indices);
    std::vector<std::string> getDatasetNames() const;
    void setErrorsEnabled(bool enabled);
    void hideGlobalCheckbox();
    void showGlobalCheckbox();

    std::vector<std::string> getGlobalParameters() const;
    void setGlobalParameters(const std::vector<std::string> &globals);
    void clear();
    void updateParameters(SIP_PYOBJECT) throw(std::exception);
    %MethodCode
      auto function = boost::python::extract<const Mantid::API::IFunction_sptr &>(a0)();
      sipCpp->updateParameters(*function);
    %End
    void updateMultiDatasetParameters(SIP_PYOBJECT) throw(std::exception);
    %MethodCode
      auto function = boost::python::extract<const Mantid::API::IFunction_sptr &>(a0)();
      sipCpp->updateMultiDatasetParameters(*function);
    %End
    SIP_PYOBJECT getFunction();
    %MethodCode
      sipRes = boost::python::to_python_value<const Mantid::API::IFunction_sptr &>()(sipCpp->getFunction());
    %End
    SIP_PYOBJECT getGlobalFunction();
    %MethodCode
      sipRes = boost::python::to_python_value<const Mantid::API::IFunction_sptr &>()(sipCpp->getGlobalFunction());
    %End
    SIP_PYOBJECT getFunctionByIndex(const std::string &index);
    %MethodCode
      sipRes = boost::python::to_python_value<const Mantid::API::IFunction_sptr &>()(sipCpp->getFunctionByIndex(*a0));
    %End
signals:
    void functionStructureChanged();
    void parameterChanged(const std::string &funcIndex, const std::string &parameterName);
    void attributeChanged(const std::string &attributeName);
};

// ---------------------------------
// FunctionTreeView
// ---------------------------------
class FunctionTreeView : public QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/FunctionTreeView.h"
%End
public:
    FunctionTreeView(QWidget *parent, bool multi);
    int getNumberOfQtProperties() const;
    int getNumberOfTieProperties() const;
    int getNumberOfConstraintProperties() const;
    QRect getVisualRectFunctionProperty(const std::string &index) const;
    QRect getVisualRectParameterProperty(const std::string &index) const;
    QTreeWidget *treeWidget() const;
    QWidget *getParamWidget(const std::string &parameterName) const;
    double getParameter(const std::string &parameterName) const;
};

// ---------------------------------
// FileFinderWidget
// ---------------------------------
class FileFinderWidget : QWidget
{
%TypeHeaderCode
#include "MantidQtWidgets/Common/FileFinderWidget.h"
%End
public:
    FileFinderWidget(QWidget *parent = nullptr) /KeywordArgs="All"/;

    QString getText();
    QString getFirstFilename() const;
    QStringList getFilenames() const;
    void isForRunFiles(bool /*mode*/);
    void isForDirectory(bool /*mode*/);
    void allowMultipleFiles(bool /*allow*/);
    bool isSearching() const;
    bool isValid();
    void liveButtonSetChecked(bool);
    bool liveButtonIsChecked();
    void setInstrumentOverride(const QString &instName);
    void setLabelText(const QString &text);
    void setLabelMinWidth(const int);
    void setText(const QString&);
    void setUserInput(const QVariant &);
    void setFileProblem(const QString &message);
    QVariant getUserInput() const;
    void setReadOnly(bool readOnly);
    void setFileExtensions(const QStringList &extensions);
    void findFiles(bool isModified);
    void isOptional(bool);
    void setLastDirectory(const QString &lastDir);
    void setUseNativeWidget(bool native);
    void setProxyModel(QAbstractProxyModel* /Transfer/);
    void readSettings(const QString &group);
    void saveSettings(const QString &group);

signals:
    void fileTextChanged(const QString & /*_t1*/);
    void fileEditingFinished();
    void fileFindingFinished();
    void liveButtonPressed(bool);
    void findingFiles();
    void filesFound();
};

// ---------------------------------
// ImageInformationWidget
// ---------------------------------
class ImageInfoWidget : public QTableWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/ImageInfoWidget.h"
#include "MantidPythonInterface/core/ExtractSharedPtr.h"
#include <boost/python/object.hpp>
#include <boost/python/handle.hpp>
%End
public:
    ImageInfoWidget(QWidget *parent = nullptr);
    void cursorAt(const double x, const double y, const double signal, const QMap<QString, QString>& extraValues) throw(std::exception);
    void setShowSignal(const bool showSignal);
    void setWorkspace(SIP_PYOBJECT) throw(std::exception);
    %MethodCode
      using boost::python::extract;
      using Mantid::PythonInterface::ExtractSharedPtr;
      sipCpp->setWorkspace(ExtractSharedPtr<Mantid::API::Workspace>(a0)());
    %End
};

class ImageInfoWidgetMini : public QLabel {
%TypeHeaderCode
#include "MantidQtWidgets/Common/ImageInfoWidgetMini.h"
#include "MantidPythonInterface/core/ExtractSharedPtr.h"
#include <boost/python/object.hpp>
#include <boost/python/handle.hpp>
%End
public:
    ImageInfoWidgetMini(QWidget *parent = nullptr);
    void cursorAt(const double x, const double y, const double signal, const QMap<QString, QString>& extraValues) throw(std::exception);
    void setWorkspace(SIP_PYOBJECT) throw(std::exception);
    %MethodCode
      using boost::python::extract;
      using Mantid::PythonInterface::ExtractSharedPtr;
      sipCpp->setWorkspace(ExtractSharedPtr<Mantid::API::Workspace>(a0)());
    %End
};


// ---------------------------------
// DataSelector
// ---------------------------------
class DataSelector : QWidget
{
%TypeHeaderCode
#include "MantidQtWidgets/Common/DataSelector.h"
%End
public:
    DataSelector(QWidget *parent = nullptr) /KeywordArgs="All"/;
};

// ---------------------------------
// WorkspaceSelector
// ---------------------------------
class WorkspaceSelector : QComboBox
{
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspaceSelector.h"
%End
public:
    WorkspaceSelector(QWidget *parent = nullptr, bool init = true) /KeywordArgs="All"/;
    void setWorkspaceTypes(const QStringList &types);
    void setOptional(bool optional);
    void setSorted(bool sorted);
    bool isConnected();
    void disconnectObservers();
    void connectObservers();
};

// ----------------------------------------------------------------------------
// FittingMode
// ----------------------------------------------------------------------------

enum class FittingMode {SEQUENTIAL, SIMULTANEOUS, SEQUENTIAL_AND_SIMULTANEOUS};

// ----------------------------------------------------------------------------
// FitScriptGeneratorView
// ----------------------------------------------------------------------------

class FitScriptGeneratorView : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
%End
public:
    FitScriptGeneratorView(
        QWidget *parent = nullptr,
        FittingMode fittingMode = FittingMode::SEQUENTIAL,
        const QMap<QString, QString> &fitOptions = QMap<QString, QString>()) /KeywordArgs="All"/;
};

// ----------------------------------------------------------------------------
// FitScriptGeneratorModel
// ----------------------------------------------------------------------------

class FitScriptGeneratorModel {
%TypeHeaderCode
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
%End
public:
    FitScriptGeneratorModel();

private:
    FitScriptGeneratorModel(const FitScriptGeneratorModel &model);
};

// ----------------------------------------------------------------------------
// FitScriptGeneratorPresenter
// ----------------------------------------------------------------------------

class FitScriptGeneratorPresenter {
%TypeHeaderCode
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
%End
public:
    FitScriptGeneratorPresenter(
	    FitScriptGeneratorView *view, FitScriptGeneratorModel *model,
        const QStringList &workspaceNames = QStringList(),
        double startX = 0.0, double endX = 0.0) /KeywordArgs="All"/;

    void openFitScriptGenerator();
};

// ---------------------------------
// Muon Period Info
// ---------------------------------

class MuonPeriodInfo : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MuonPeriodInfo.h"
#include "MantidPythonInterface/core/ExtractSharedPtr.h"
#include <boost/python/object.hpp>
#include <boost/python/handle.hpp>
%End
public:
    MuonPeriodInfo(QWidget *parent = nullptr) /KeywordArgs="All"/;
    void clear();
    void setWidgetTitleRuns(const std::string &title);
    bool isEmpty() const ;
    void addInfo(SIP_PYOBJECT) throw(std::exception);
    %MethodCode
      using boost::python::extract;
      using Mantid::PythonInterface::ExtractSharedPtr;
      sipCpp->addInfo(ExtractSharedPtr<Mantid::API::Workspace>(a0)());
    %End
};
