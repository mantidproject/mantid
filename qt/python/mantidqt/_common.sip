%ModuleCode
#include "MantidQtWidgets/Common/Message.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidPythonInterface/core/VersionCompat.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
%End

%InitialisationCode
qRegisterMetaType<std::string>("StdString");
qRegisterMetaType<Mantid::API::Workspace_sptr>("Workspace");
qRegisterMetaType<Message>("Message");
%End

// ----------------------------------------------------------------------------
// Exceptions
// ----------------------------------------------------------------------------

%Exception std::invalid_argument(SIP_Exception) /PyName=ValueError/
{
%TypeHeaderCode
#include <stdexcept>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(PyExc_ValueError, detail);
    SIP_UNBLOCK_THREADS
%End
};

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
  void attachLoggingChannel(int logLevel = -1);

  void appendFatal(const QString &text);
  void appendError(const QString &text);
  void appendWarning(const QString &text);
  void appendNotice(const QString &text);
  void appendInformation(const QString &text);
  void appendDebug(const QString &text);
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

public:
  ScriptEditor(const QString & language,
               QWidget *parent /TransferThis/ = 0) throw(std::invalid_argument);

  QString fileName() const;
  SIP_PYTUPLE getSelection() const;
%MethodCode
  int lineFrom(-1), indexFrom(-1), lineTo(-1), indexTo(-1);
  sipCpp->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  sipRes = sipBuildResult(nullptr, "(iiii)", lineFrom, indexFrom,
                          lineTo, indexTo);
%End
  bool hasSelectedText() const;
  bool isModified() const;
  bool isReadOnly() const;
  QString selectedText() const;
  QString text() const;

  void enableAutoCompletion(AutoCompletionSource source);
  void disableAutoCompletion();
  void setCaretLineBackgroundColor (const QColor & col);
  void setCaretLineVisible(bool enable);
  void setIndentationsUseTabs(bool tabs);
  void setFileName(const QString &filename);
  void setMarginWidth(int margin, int width);
  void setModified(bool m);
  void setReadOnly(bool ro);
  void setSelection(int lineFrom, int indexFrom, int lineTo, int indexTo);
  void setTabWidth(int width);
  void setText(const QString &text);
  void updateCompletionAPI(const QStringList & completions);

public slots:
  void updateProgressMarker(int lineno, bool error);

signals:
  void fileNameChanged(const QString &fileName);
  void modificationChanged(bool m);

private:
  ScriptEditor(const ScriptEditor&);
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


class InterfaceManager {
%TypeHeaderCode
#include "MantidQtWidgets/Common/InterfaceManager.h"
using namespace MantidQt::API;
%End
public:
  InterfaceManager();
  AlgorithmDialog *createDialogFromName(
      const QString &algorithmName, const int version = -1,
      QWidget *parent = nullptr, bool forScript = false,
      const QHash<QString, QString> &presetValues = QHash<QString, QString>(),
      const QString &optionalMsg = QString(),
      const QStringList &enabled = QStringList(),
      const QStringList &disabled = QStringList());
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
};

// Implementation to override context menu
class WorkspaceTreeWidgetSimple : WorkspaceTreeWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
%End
public:
  WorkspaceTreeWidgetSimple(bool viewOnly=false, QWidget *parent /TransferThis/ = nullptr);

signals:
  void plotSpectrumClicked(const QStringList & workspaceNames);
  void overplotSpectrumClicked(const QStringList & workspaceNames);
  void plotSpectrumWithErrorsClicked(const QStringList & workspaceNames);
  void overplotSpectrumWithErrorsClicked(const QStringList & workspaceNames);
  void plotColorfillClicked(const QStringList &workspaceNames);
  void sampleLogsClicked(const QStringList &workspaceNames);
  void showInstrumentClicked(const QStringList &workspaceNames);
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
    auto f = sipCpp->getFittingFunction();
    sipRes = FROM_CSTRING(f->asString().c_str());
%End
    int sizeOfFunctionsGroup() const;
    void loadFunction(QString funStr);
    double startX() const;
    void setStartX(double start);
    double endX() const;
    void setEndX(double end);
    void setXRange(double start, double end);
    int workspaceIndex() const;
    void setWorkspaceIndex(int i);
    QStringList getWorkspaceNames();
    void fit();
    void addAllowedSpectra(const QString &wsName, const QList<int> &wsIndices);
signals:
    void startXChanged(double);
    void endXChanged(double);
    void fittingDone(const QString &);
};

// ---------------------------------
// Function Browser
// ---------------------------------
class FunctionBrowser : public QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/FunctionBrowser.h"
%End
public:
    FunctionBrowser(QWidget *parent = nullptr, bool multi = false);
    FunctionTreeView *view() const;
    void setFunction(const QString &funStr);
    QString getFunctionString();
    void setParameter(const QString &funStr, double value);
    double getParameter(const QString &funStr);
    int getNumberOfDatasets() const;
    void setNumberOfDatasets(int n);
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
    QRect getVisualRectFunctionProperty(const QString &index) const;
    QRect getVisualRectParameterProperty(const QString &index) const;
    QTreeWidget *treeWidget() const;
    QWidget *getParamWidget(const QString &paramName) const;
    double getParameter(const QString &paramName) const;
};
