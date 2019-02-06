%ModuleCode
#include "MantidQtWidgets/Common/Message.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidPythonInterface/core/VersionCompat.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets::Batch;
%End

%Include ../../_common.sip


%InitialisationCode
qRegisterMetaType<std::string>("StdString");
%End


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

class RowPredicate {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/FilteredTreeModel.h"
%End
protected:
  RowPredicate();
  virtual bool rowMeetsCriteria(const RowLocation & row) const = 0;
};

class JobTreeViewSubscriber {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
%End
public:
  virtual void notifyCellTextChanged(const RowLocation& itemIndex,
                                     int column,
                                     const std::string& oldValue,
                                     const std::string& newValue) = 0;
  virtual void notifyRowInserted(const RowLocation& itemIndex) = 0;
  virtual void notifyRemoveRowsRequested(
    const std::vector<RowLocation>& locationsOfRowsToRemove) = 0;
  virtual void notifyCopyRowsRequested() = 0;
  virtual void notifyCutRowsRequested() = 0;
  virtual void notifyFilterReset() = 0;
  virtual void notifyPasteRowsRequested() = 0;
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
    RowLocation insertChildRowOf(const RowLocation
     &parent, int beforeRow,
                          const std::vector<Cell> &rowText);
    RowLocation insertChildRowOf(const RowLocation
    &parent, int beforeRow);
    RowLocation appendChildRowOf(const RowLocation
    &parent);
    RowLocation appendChildRowOf(const RowLocation
    &parentLocation,
                          const std::vector<Cell> &rowText);
    void removeRowAt(const RowLocation &location);
    void removeRows(std::vector<RowLocation> rowsToRemove);
    void removeAllRows();
    void replaceRows(std::vector<RowLocation> replacementPoints,
                     std::vector<std::vector<Row>> replacementLocations);
    void appendSubtreesAt(const RowLocation& parent,
                          std::vector<std::vector<Row>> subtrees);
    void appendSubtreeAt(const RowLocation& parent,
                         std::vector<Row> subtree);
    void replaceSubtreeAt(const RowLocation &rootToRemove,
                          std::vector<Row> subtree);
    void insertSubtreeAt(const RowLocation& parent, int index,
                         std::vector<Row> subtree);

    void filterRowsBy(RowPredicate* predicate /Transfer/);
    void resetFilter();
    bool hasFilter() const;

    void setHintsForColumn(int column, MantidQt::MantidWidgets::HintStrategy* hintStrategy /Transfer/);

    bool isOnlyChildOfRoot(const RowLocation& location) const;
    std::vector<Cell> cellsAt(const RowLocation &location) const;
    void setCellsAt(const RowLocation &location,
                    const std::vector<Cell> &cells);
    Cell cellAt(RowLocation location, int column);
    void setCellAt(RowLocation location, int column, const Cell &cellText);
    Cell deadCell() const;

    std::vector<RowLocation> selectedRowLocations() const;
    boost::optional<std::vector<std::vector<Row>>> selectedSubtrees() const;
    boost::optional<std::vector<RowLocation>> selectedSubtreeRoots() const;
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
  Row(RowLocation location, std::vector<Cell> cells);
  const std::vector<Cell>& cells() const;
  const RowLocation& location() const;
};

class JobTreeViewSignalAdapter : public QObject, public JobTreeViewSubscriber {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/JobTreeViewSignalAdapter.h"
%End
public:
  JobTreeViewSignalAdapter(JobTreeView &view, QObject *parent = nullptr);
  virtual void notifyCellTextChanged(const RowLocation &itemIndex,
                                     int column,
                                     const std::string &oldValue,
                                     const std::string &newValue);
  virtual void notifyRowInserted(const RowLocation &itemIndex);
  virtual void notifyRemoveRowsRequested(
      const std::vector<RowLocation> &locationsOfRowsToRemove);
  virtual void notifyCopyRowsRequested();
  virtual void notifyCutRowsRequested();
  virtual void notifyPasteRowsRequested();
  virtual void notifyFilterReset();
signals:
  void cellTextChanged(const RowLocation &itemIndex,
                       int column,
                       std::string oldValue,
                       std::string newValue);
  void rowInserted(const RowLocation &itemIndex);
  void filterReset();
  void removeRowsRequested(
      const std::vector<RowLocation> &locationsOfRowsToRemove);
  void copyRowsRequested();
  void cutRowsRequested();

};