%ModuleCode
#include "MantidQtWidgets/Common/Message.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidPythonInterface/core/VersionCompat.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
%End

%Include ../common_sip/string.sip
%Include ../common_sip/vector.sip

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
  void showHelpPage(const QString &url = QString());
  void showWikiPage(const QString &page);
  void showAlgorithmHelp(const QString &name, const int version = -1);
  void showConceptHelp(const QString &name);
  void showFitFunctionHelp(const QString &name = QString());
  void showCustomInterfaceHelp(const QString &name);
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
  void refreshWorkspaces();

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
  static void openUserDirsDialog(QWidget *parent);
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
  virtual std::vector<Hint> createHints();
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
namespace MantidQt
{
namespace MantidWidgets
{
namespace Batch
{

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
  bool isChildOf(const RowLocation &other);
  bool isSiblingOf(const RowLocation &other);
  bool isChildOrSiblingOf(const RowLocation &other);
  bool isDescendantOf(const RowLocation &other);
  RowLocation parent();
  RowLocation relativeTo(const RowLocation &ancestor);
  RowLocation child(int n);

  bool operator==(const RowLocation& other);
  bool operator!=(const RowLocation& other);
  bool operator<(const RowLocation& other);
  bool operator<=(const RowLocation& other);
  bool operator>(const RowLocation& other);
  bool operator>=(const RowLocation& other);
};

}; // Batch
}; // MantidWidgets
}; // MantidQt

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
  virtual void notifyRowInserted(const MantidQt::MantidWidgets::Batch::RowLocation& itemIndex) = 0;
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

    void subscribe(JobTreeViewSubscriber& subscriber);
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
    void removeRowAt(const MantidQt::MantidWidgets::Batch::RowLocation &location);
    void removeRows(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> rowsToRemove);
    void removeAllRows();
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
  virtual void notifyRowInserted(const MantidQt::MantidWidgets::Batch::RowLocation &itemIndex);
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
  void removeRowsRequested(
      const std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &locationsOfRowsToRemove);
  void copyRowsRequested();
  void cutRowsRequested();

};
