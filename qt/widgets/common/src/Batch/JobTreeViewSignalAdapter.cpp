#include "MantidQtWidgets/Common/Batch/JobTreeViewSignalAdapter.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
JobTreeViewSignalAdapter::JobTreeViewSignalAdapter(JobTreeView &view,
                                                   QObject *parent)
    : QObject(parent) {
  qRegisterMetaType<RowLocation>("MantidQt::MantidWidgets::Batch::RowLocation");
  qRegisterMetaType<std::vector<RowLocation>>(
      "std::vector<MantidQt::MantidWidgets::Batch::RowLocation>");
  view.subscribe(*this);
}

void JobTreeViewSignalAdapter::notifyCellTextChanged(
    RowLocation const &itemIndex, int column, std::string const &oldValue,
    std::string const &newValue) {
  emit cellTextChanged(itemIndex, column, oldValue, newValue);
}

void JobTreeViewSignalAdapter::notifyRowInserted(
    RowLocation const &newRowLocation) {
  emit rowInserted(newRowLocation);
}

void JobTreeViewSignalAdapter::notifyRemoveRowsRequested(
    std::vector<RowLocation> const &locationsOfRowsToRemove) {
  emit removeRowsRequested(locationsOfRowsToRemove);
}

void JobTreeViewSignalAdapter::notifyCopyRowsRequested() {
  emit copyRowsRequested();
}

void JobTreeViewSignalAdapter::notifyCutRowsRequested() {
  emit cutRowsRequested();
}

void JobTreeViewSignalAdapter::notifyPasteRowsRequested() {
  emit pasteRowsRequested();
}

void JobTreeViewSignalAdapter::notifyFilterReset() { emit filterReset(); }
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
