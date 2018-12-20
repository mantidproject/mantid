#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

RowLocationAdapter::RowLocationAdapter(QStandardItemModel const &model)
    : m_model(model) {}

RowLocation
RowLocationAdapter::atIndex(QModelIndexForMainModel const &index) const {
  if (index.isValid()) {
    auto pathComponents = RowPath();
    auto currentIndex = index.untyped();
    while (currentIndex.isValid()) {
      pathComponents.insert(pathComponents.begin(), currentIndex.row());
      currentIndex = currentIndex.parent();
    }
    return RowLocation(pathComponents);
  } else {
    return RowLocation();
  }
}

QModelIndex RowLocationAdapter::walkFromRootToParentIndexOf(
    RowLocation const &location) const {
  auto parentIndex = QModelIndex();
  auto &path = location.path();
  for (auto it = path.cbegin(); it != path.cend() - 1; ++it)
    parentIndex = m_model.index(*it, 0, parentIndex);
  return parentIndex;
}

boost::optional<QModelIndexForMainModel>
RowLocationAdapter::indexIfExistsAt(RowLocation const &location,
                                    int column) const {
  if (location.isRoot()) {
    return fromMainModel(QModelIndex(), m_model);
  } else {
    auto const parentIndex = walkFromRootToParentIndexOf(location);
    auto const row = location.rowRelativeToParent();
    if (m_model.hasIndex(row, column, parentIndex))
      return fromMainModel(m_model.index(row, column, parentIndex), m_model);
    else
      return boost::none;
  }
}

QModelIndexForMainModel RowLocationAdapter::indexAt(RowLocation const &location,
                                                    int column) const {
  auto maybeIndex = indexIfExistsAt(location, column);
  if (maybeIndex.is_initialized())
    return maybeIndex.get();
  else {
    throw std::runtime_error("indexAt: Attempted to get model index for "
                             "row location which does not exist.");
  }
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
