#include "BatchPresenter.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
namespace MantidQt {
namespace CustomInterfaces {

BatchPresenterFactory::BatchPresenterFactory(
    std::vector<std::string> const &instruments)
    : m_instruments(instruments) {}

std::unique_ptr<BatchPresenter> BatchPresenterFactory::
operator()(IBatchView *view) const {
  return std::make_unique<BatchPresenter>(view, m_instruments);
}

BatchPresenter::BatchPresenter(IBatchView *view,
                               std::vector<std::string> const &instruments)
    : m_view(view), m_instruments(instruments) {
  m_view->subscribe(this);
}

void BatchPresenter::notifyDeleteRowRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (!selected.empty()) {
    if (!containsGroups(selected)) {
      m_view->jobs().removeRows(selected);
    } else {
      // TODO: m_view->mustNotSelectGroupError();
    }
  } else {
    // TODO: m_view->mustNotSelectRowError();
  }
}

void BatchPresenter::notifyDeleteGroupRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groups = groupIndexesFromSelection(selected);
    // groupIndexesFromSelection returns indexes ordered from low to high.
    // To avoid invalidating our row locations we need to delete in the reverse
    // order.
    for (auto it = groups.crbegin(); it < groups.crend(); ++it)
      m_view->jobs().removeRowAt(
          MantidQt::MantidWidgets::Batch::RowLocation({*it}));
  } else {
    // TODO: m_view->mustSelectGroupOrRowError();
  }
}

void BatchPresenter::notifyPauseRequested() {

}

void BatchPresenter::notifyProcessRequested() {

}

template <typename T>
void sortAndRemoveDuplicatesInplace(std::vector<T> &items) {
  std::sort(items.begin(), items.end());
  auto eraseBegin = std::unique(items.begin(), items.end());
  items.erase(eraseBegin, items.end());
}

std::vector<int> mapToContainingGroups(std::vector<
    MantidQt::MantidWidgets::Batch::RowLocation> const &mustNotContainRoot) {
  auto groups = std::vector<int>();
  std::transform(mustNotContainRoot.cbegin(), mustNotContainRoot.cend(),
                 std::back_inserter(groups),
                 [](MantidWidgets::Batch::RowLocation const &location)
                     -> int { return location.path()[0]; });
  return groups;
}

std::vector<int> BatchPresenter::groupIndexesFromSelection(
    std::vector<MantidWidgets::Batch::RowLocation> const &selected) const {
  auto groups = mapToContainingGroups(selected);
  sortAndRemoveDuplicatesInplace(groups);
  return groups;
}

void BatchPresenter::notifyInsertRowRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groups = groupIndexesFromSelection(selected);
    for (auto &&groupIndex : groups)
      m_view->jobs().appendChildRowOf(
          MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}));
  } else {
    // TODO: m_view->mustSelectGroupError();
  }
}

void BatchPresenter::notifyInsertGroupRequested() {
  auto selected = m_view->jobs().selectedRowLocations();
  if (selected.size() > 0) {
    auto groups = groupIndexesFromSelection(selected);
    m_view->jobs().insertChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation(), groups[0] + 1);
  } else {
    m_view->jobs().appendChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation());
  }
}

void BatchPresenter::notifyExpandAllRequested() { m_view->jobs().expandAll(); }

void BatchPresenter::notifyCollapseAllRequested() {
  m_view->jobs().collapseAll();
}

void BatchPresenter::notifyCellTextChanged(
    MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {}

bool BatchPresenter::isGroupLocation(
    MantidQt::MantidWidgets::Batch::RowLocation const &location) const {
  return location.depth() == 1;
}

bool BatchPresenter::containsGroups(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations)
    const {
  return std::count_if(
             locations.cbegin(), locations.cend(),
             [this](MantidQt::MantidWidgets::Batch::RowLocation const &location)
                 -> bool { return isGroupLocation(location); }) > 0;
}

void BatchPresenter::notifyRowInserted(
    MantidQt::MantidWidgets::Batch::RowLocation const &newRowLocation) {
  if (newRowLocation.depth() > DEPTH_LIMIT)
    m_view->jobs().removeRowAt(newRowLocation);
  else if (isGroupLocation(newRowLocation))
    // TODO kill the cells.
    return;
}

void BatchPresenter::notifyRemoveRowsRequested(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
        locationsOfRowsToRemove) {
  m_view->jobs().removeRows(locationsOfRowsToRemove);
}

void BatchPresenter::notifyCopyRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized())
    m_view->jobs().clearSelection();
  else
    // TODO: m_view->invalidSelectionForCopy();
    return;
}

void BatchPresenter::notifyCutRowsRequested() {
  m_clipboard = m_view->jobs().selectedSubtrees();
  if (m_clipboard.is_initialized()) {
    m_view->jobs().removeRows(m_view->jobs().selectedRowLocations());
    m_view->jobs().clearSelection();
  } else {
    // TODO: m_view->invalidSelectionForCut();
  }
}

void BatchPresenter::notifyPasteRowsRequested() {
  auto maybeReplacementRoots = m_view->jobs().selectedSubtreeRoots();
  if (maybeReplacementRoots.is_initialized() && m_clipboard.is_initialized()) {
    auto &replacementRoots = maybeReplacementRoots.get();
    if (!replacementRoots.empty())
      m_view->jobs().replaceRows(replacementRoots, m_clipboard.get());
    else
      m_view->jobs().appendSubtreesAt(
          MantidQt::MantidWidgets::Batch::RowLocation(), m_clipboard.get());
  } else {
    // TODO: m_view->invalidSelectionForPaste();
  }
}

void BatchPresenter::notifyFilterReset() {}
}
}
