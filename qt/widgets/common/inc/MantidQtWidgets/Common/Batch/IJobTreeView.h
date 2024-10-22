// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include "MantidQtWidgets/Common/Batch/Row.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/HintStrategy.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON JobTreeViewSubscriber {
public:
  virtual void notifyCellTextChanged(RowLocation const &itemIndex, int column, std::string const &oldValue,
                                     std::string const &newValue) = 0;
  virtual void notifySelectionChanged() = 0;
  virtual void notifyRowInserted(RowLocation const &newRowLocation) = 0;
  virtual void notifyAppendAndEditAtChildRowRequested() = 0;
  virtual void notifyAppendAndEditAtRowBelowRequested() = 0;
  virtual void notifyEditAtRowAboveRequested() = 0;
  virtual void notifyRemoveRowsRequested(std::vector<RowLocation> const &locationsOfRowsToRemove) = 0;
  virtual void notifyCutRowsRequested() = 0;
  virtual void notifyCopyRowsRequested() = 0;
  virtual void notifyPasteRowsRequested() = 0;
  virtual void notifyFilterReset() = 0;
  virtual ~JobTreeViewSubscriber() = default;
};

class EXPORT_OPT_MANTIDQT_COMMON IJobTreeView {
public:
  virtual void filterRowsBy(std::unique_ptr<RowPredicate> predicate) = 0;
  virtual void filterRowsBy(RowPredicate *predicate) = 0;
  virtual void resetFilter() = 0;
  virtual bool hasFilter() const = 0;

  virtual void setHintsForColumn(int column, std::unique_ptr<HintStrategy> hintStrategy) = 0;
  virtual void setHintsForColumn(int column, HintStrategy *hintStrategy) = 0;

  virtual void subscribe(JobTreeViewSubscriber *subscriber) = 0;

  virtual RowLocation insertChildRowOf(RowLocation const &parent, int beforeRow, std::vector<Cell> const &rowText) = 0;
  virtual RowLocation insertChildRowOf(RowLocation const &parent, int beforeRow) = 0;
  virtual RowLocation appendChildRowOf(RowLocation const &parent) = 0;
  virtual RowLocation appendChildRowOf(RowLocation const &parentLocation, std::vector<Cell> const &rowText) = 0;
  virtual void appendAndEditAtChildRow() = 0;
  virtual void appendAndEditAtRowBelow() = 0;
  virtual void editAtRowAbove() = 0;

  virtual void removeRowAt(RowLocation const &location) = 0;
  virtual void removeRows(std::vector<RowLocation> rowsToRemove) = 0;
  virtual void removeAllRows() = 0;

  virtual bool isOnlyChildOfRoot(RowLocation const &location) const = 0;

  virtual void replaceRows(std::vector<RowLocation> replacementPoints, std::vector<Subtree> replacements) = 0;

  virtual void appendSubtreesAt(RowLocation const &parent, std::vector<Subtree> subtrees) = 0;
  virtual void appendSubtreeAt(RowLocation const &parent, Subtree const &subtree) = 0;

  virtual void replaceSubtreeAt(RowLocation const &rootToRemove, Subtree const &toInsert) = 0;
  virtual void insertSubtreeAt(RowLocation const &parent, int index, Subtree const &subtree) = 0;

  virtual std::vector<Cell> cellsAt(RowLocation const &location) const = 0;
  virtual void setCellsAt(RowLocation const &location, std::vector<Cell> const &rowText) = 0;

  virtual Cell cellAt(RowLocation location, int column) const = 0;
  virtual void setCellAt(RowLocation location, int column, Cell const &cellText) = 0;

  virtual void clearSelection() = 0;
  virtual void expandAll() = 0;
  virtual void collapseAll() = 0;

  virtual std::vector<RowLocation> selectedRowLocations() const = 0;
  virtual boost::optional<std::vector<Subtree>> selectedSubtrees() const = 0;
  virtual boost::optional<std::vector<RowLocation>> selectedSubtreeRoots() const = 0;
  virtual int currentColumn() const = 0;
  virtual Cell deadCell() const = 0;
  virtual ~IJobTreeView() = default;
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
