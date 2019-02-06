%ModuleCode
// Allows suppression of namespaces within the module
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
using namespace MantidQt::MantidWidgets::Batch;
%End

%Include ../../_common.sip

%InitialisationCode
qRegisterMetaType<std::string>("StdString");
qRegisterMetaType<RowLocation>("RowLocation");
qRegisterMetaType<std::vector<RowLocation>>(
      "std::vector<RowLocation>");
%End

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
    JobTreeView(const QStringList &columnHeadings, const MantidQt::MantidWidgets::Batch::Cell& defaultCellStyle,
     QWidget *parent = nullptr);

    void subscribe(MantidQt::MantidWidgets::Batch::JobTreeViewSubscriber& subscriber);
    MantidQt::MantidWidgets::Batch::RowLocation insertChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
     &parent, int beforeRow,
                          const std::vector<MantidQt::MantidWidgets::Batch::Cell> &rowText);
    MantidQt::MantidWidgets::Batch::RowLocation insertChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
    &parent, int beforeRow);
    MantidQt::MantidWidgets::Batch::RowLocation appendChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
    &parent);
    MantidQt::MantidWidgets::Batch::RowLocation appendChildRowOf(const MantidQt::MantidWidgets::Batch::RowLocation
    &parentLocation,
                          const std::vector<MantidQt::MantidWidgets::Batch::Cell> &rowText);
    void removeRowAt(const MantidQt::MantidWidgets::Batch::RowLocation &location);
    void removeRows(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> rowsToRemove);
    void removeAllRows();
    void replaceRows(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> replacementPoints,
                     std::vector<std::vector<MantidQt::MantidWidgets::Batch::Row>> replacementLocations);
    void appendSubtreesAt(const MantidQt::MantidWidgets::Batch::RowLocation& parent,
                          std::vector<std::vector<MantidQt::MantidWidgets::Batch::Row>> subtrees);
    void appendSubtreeAt(const MantidQt::MantidWidgets::Batch::RowLocation& parent,
                         std::vector<MantidQt::MantidWidgets::Batch::Row> subtree);
    void replaceSubtreeAt(const MantidQt::MantidWidgets::Batch::RowLocation &rootToRemove,
                          std::vector<MantidQt::MantidWidgets::Batch::Row> subtree);
    void insertSubtreeAt(const MantidQt::MantidWidgets::Batch::RowLocation& parent, int index,
                         std::vector<MantidQt::MantidWidgets::Batch::Row> subtree);

    void filterRowsBy(MantidQt::MantidWidgets::Batch::RowPredicate* predicate /Transfer/);
    void resetFilter();
    bool hasFilter() const;

    void setHintsForColumn(int column, ::HintStrategy* hintStrategy /Transfer/);

    bool isOnlyChildOfRoot(const MantidQt::MantidWidgets::Batch::RowLocation& location) const;
    std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsAt(const MantidQt::MantidWidgets::Batch::RowLocation &location) const;
    void setCellsAt(const MantidQt::MantidWidgets::Batch::RowLocation &location,
                    const std::vector<MantidQt::MantidWidgets::Batch::Cell> &cells);
    MantidQt::MantidWidgets::Batch::Cell cellAt(MantidQt::MantidWidgets::Batch::RowLocation location, int column);
    void setCellAt(MantidQt::MantidWidgets::Batch::RowLocation location, int column, const MantidQt::MantidWidgets::Batch::Cell &cellText);
    MantidQt::MantidWidgets::Batch::Cell deadCell() const;

    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> selectedRowLocations() const;
    boost::optional<std::vector<std::vector<MantidQt::MantidWidgets::Batch::Row>>> selectedSubtrees() const;
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
  Row(MantidQt::MantidWidgets::Batch::RowLocation location, std::vector<MantidQt::MantidWidgets::Batch::Cell> cells);
  const std::vector<MantidQt::MantidWidgets::Batch::Cell>& cells() const;
  const MantidQt::MantidWidgets::Batch::RowLocation& location() const;
};

class JobTreeViewSignalAdapter : public QObject, public MantidQt::MantidWidgets::Batch::JobTreeViewSubscriber {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Batch/JobTreeViewSignalAdapter.h"
%End
public:
  JobTreeViewSignalAdapter(MantidQt::MantidWidgets::Batch::JobTreeView &view, QObject *parent = nullptr);
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
  void cellTextChangedTmp(int column, const std::string &oldValue, const std::string &newValue);

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

};
};
};
