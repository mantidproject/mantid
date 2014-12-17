//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoryView.h"

namespace Mantid {
namespace API {

HistoryView::HistoryView(const WorkspaceHistory &wsHist)
    : m_wsHist(wsHist), m_historyItems() {
  // add all of the top level algorithms to the view by default
  const auto algorithms = wsHist.getAlgorithmHistories();
  AlgorithmHistories::const_iterator iter = algorithms.begin();
  for (; iter != algorithms.end(); ++iter) {
    HistoryItem item(*iter);
    m_historyItems.push_back(item);
  }
}

/**
 * Unroll an algorithm history to export its child algorithms.
 *
 * This places each of the child algorithm histories into the
 * HistoryView object. The parent is retained as a marker so we can
 * "roll" the history back up if we want. This method does nothing if
 * the history object has no children
 *
 * @param index :: index of the history object to unroll
 * @throws std::out_of_range if the index is larger than the number of history
 *items.
 */
void HistoryView::unroll(size_t index) {
  if (index >= m_historyItems.size()) {
    throw std::out_of_range("HistoryView::unroll() - Index out of range");
  }

  // advance to the item at the index
  auto it = m_historyItems.begin();
  std::advance(it, index);

  unroll(it);
}

/**
 * Unroll an algorithm history to export its child algorithms.
 *
 * This places each of the child algorithm histories into the
 * HistoryView object. The parent is retained as a marker so we can
 * "roll" the history back up if we want. This method does nothing if
 * the history object has no children
 *
 * @param it :: iterator to the list of history item objects at the position to
 *unroll
 */
void HistoryView::unroll(std::list<HistoryItem>::iterator it) {
  const auto history = it->getAlgorithmHistory();
  const auto childHistories = history->getChildHistories();

  if (!it->isUnrolled() && childHistories.size() > 0) {
    // mark this record as being ignored by the script builder
    it->unrolled(true);

    ++it; // move iterator forward to insertion position
    // insert each of the records, in order, at this position
    for (auto childIter = childHistories.begin();
         childIter != childHistories.end(); ++childIter) {
      HistoryItem item(*childIter);
      m_historyItems.insert(it, item);
    }
  }
}

/**
 * Unroll the entire algorithm history.
 *
 * This is method will unroll the entire history for every algorithm by calling
 *unroll
 * on each element in the list. Every child algorithm with be visible after
 *calling this method.
 */
void HistoryView::unrollAll() {
  for (auto it = m_historyItems.begin(); it != m_historyItems.end(); ++it) {
    unroll(it);
  }
}

/**
 * Roll the entire algorithm history back up.
 *
 * This is method will roll up the entire history for every algorithm by calling
 *roll
 * on each element in the list. Only top level algorithms with be visible after
 *calling this method.
 */
void HistoryView::rollAll() {
  for (auto it = m_historyItems.begin(); it != m_historyItems.end(); ++it) {
    roll(it);
  }
}

/**
 * Roll an unrolled algorithm history item and remove its children from the
 *view.
 *
 * This removes each of the child algorithm histories (if any) and marks
 * the parent as being "rolled up". Note that this will recursively "roll up"
 *any child
 * history objects that are also unrolled. This method does nothing if
 * the history object has no children.
 *
 * @param index :: index of the history object to unroll
 * @throws std::out_of_range if the index is larger than the number of history
 *items.
 */
void HistoryView::roll(size_t index) {
  if (index >= m_historyItems.size()) {
    throw std::out_of_range("HistoryView::roll() - Index out of range");
  }

  // advance to the item at the index
  auto it = m_historyItems.begin();
  std::advance(it, index);

  roll(it);
}

/**
 * Roll an unrolled algorithm history item and remove its children from the
 *view.
 *
 * This removes each of the child algorithm histories (if any) and marks
 * the parent as being "rolled up". Note that this will recursively "roll up"
 *any child
 * history objects that are also unrolled. This method does nothing if
 * the history object has no children.
 *
 * @param it :: iterator to the list of history item objects at the positon to
 *roll
 */
void HistoryView::roll(std::list<HistoryItem>::iterator it) {

  // the number of records after this position
  const size_t numChildren = it->numberOfChildren();
  if (it->isUnrolled() && numChildren > 0) {
    // mark this record as not being ignored by the script builder
    it->unrolled(false);
    ++it; // move to first child

    // remove each of the children from the list
    for (size_t i = 0; i < numChildren; ++i) {
      // check if our children are unrolled and
      // roll them back up if so.
      if (it->isUnrolled()) {
        roll(it);
      }
      // Then just remove the item from the list
      it = m_historyItems.erase(it);
    }
  }
}

/**
 * Filter the list of history items to remove any anlgorithms whos start
 * time is outside of the given range.
 *
 * @param start Start of time range
 * @param end End of time range
 */
void HistoryView::filterBetweenExecDate(Mantid::Kernel::DateAndTime start,
                                        Mantid::Kernel::DateAndTime end) {
  for (auto it = m_historyItems.begin(); it != m_historyItems.end();) {
    Mantid::Kernel::DateAndTime algExecutionDate =
        it->getAlgorithmHistory()->executionDate();

    // If the algorithm is outside of the time range, remove it and keep
    // iterating
    if (algExecutionDate < start || algExecutionDate > end) {
      it = m_historyItems.erase(it);
    } else {
      ++it;
    }
  }
}

/**
 * Get the list of History Items for this view.
 *
 * @returns vector of history items for this view.
 */
const std::vector<HistoryItem> HistoryView::getAlgorithmsList() const {
  std::vector<HistoryItem> histories;
  histories.reserve(size());
  std::copy(m_historyItems.begin(), m_historyItems.end(),
            std::back_inserter(histories));
  return histories;
}

} // namespace API
} // namespace Mantid
