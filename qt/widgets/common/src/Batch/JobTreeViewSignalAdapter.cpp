#include "MantidQtWidgets/Common/Batch/JobTreeViewSignalAdapter.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
JobTreeViewSignalAdapter::JobTreeViewSignalAdapter(JobTreeView &view,
                                                   QObject *parent)
    : QObject(parent) {
  qRegisterMetaType<RowLocation>("MantidQt::MantidWidgets::Batch::RowLocation");
  qRegisterMetaType<std::vector<RowLocation>>("std::vector<MantidQt::MantidWidgets::Batch::RowLocation>");
  view.subscribe(*this);
}

void JobTreeViewSignalAdapter::notifyCellChanged(RowLocation const& itemIndex,
                                                 int column,
                                                 std::string const& newValue) {
  emit cellChanged(itemIndex, column, newValue);
}

void JobTreeViewSignalAdapter::notifyRowInserted(RowLocation const& itemIndex) {
  emit rowInserted(itemIndex);
}

void JobTreeViewSignalAdapter::notifyRemoveRowsRequested(
    std::vector<RowLocation> const &locationsOfRowsToRemove) {
  emit removeRowsRequested(locationsOfRowsToRemove);
}
}
}
}
