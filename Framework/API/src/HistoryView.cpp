// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoryView.h"
#include <algorithm>
#include <iterator>

namespace Mantid {
namespace API {

HistoryView::HistoryView(const WorkspaceHistory &wsHist)
    : m_wsHist(wsHist), m_historyItems() {
  // add all of the top level algorithms to the view by default
  const auto &algorithms = wsHist.getAlgorithmHistories();
  m_historyItems =
      std::vector<HistoryItem>(algorithms.begin(), algorithms.end());
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
void HistoryView::unroll(std::vector<HistoryItem>::iterator &it) {
  const auto history = it->getAlgorithmHistory();
  const auto childHistories = history->getChildHistories();

  if (!it->isUnrolled() && !childHistories.empty()) {
    // mark this record as being ignored by the script builder
    it->unrolled(true);

    // insert each of the records, in order, at this position
    std::vector<HistoryItem> tmpHistory(childHistories.cbegin(),
                                        childHistories.cend());
    // since we are using a std::vector, do all insertions at the same time.
    ++it; // move iterator forward to insertion position
    it = m_historyItems.insert(it, tmpHistory.begin(), tmpHistory.end());
  } else
    ++it;
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
  auto it = m_historyItems.begin();
  while (it != m_historyItems.end()) {
    // iterator passed by reference to prevent iterator invalidation.
    // iterator incremented within function.
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
  auto it = m_historyItems.begin();
  while (it != m_historyItems.end()) {
    // iterator incremented within function.
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
 * Check if our children are unrolled and if so roll them back up.
 *
 * @param it :: iterator pointing to the item whose children will be rolled up.
 */
void HistoryView::rollChildren(std::vector<HistoryItem>::iterator it) {
  const size_t numChildren = it->numberOfChildren();
  ++it;
  for (size_t i = 0; i < numChildren; ++i) {
    if (it->isUnrolled())
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
 * @param it :: iterator to the list of history item objects at the positon to
 *roll
 */
void HistoryView::roll(std::vector<HistoryItem>::iterator &it) {

  // the number of records after this position
  const size_t numChildren = it->numberOfChildren();
  if (it->isUnrolled() && numChildren > 0) {
    // mark this record as not being ignored by the script builder
    it->unrolled(false);
    this->rollChildren(it);
    // Then just remove the children from the list
    ++it;
    it = m_historyItems.erase(it, it + numChildren);
  } else
    ++it;
}

/**
 * Filter the list of history items to remove any anlgorithms whos start
 * time is outside of the given range.
 *
 * @param start Start of time range
 * @param end End of time range
 */
void HistoryView::filterBetweenExecDate(Mantid::Types::Core::DateAndTime start,
                                        Mantid::Types::Core::DateAndTime end) {
  auto lastItem = std::remove_if(
      m_historyItems.begin(), m_historyItems.end(),
      [&start, &end](const HistoryItem &item) {
        Mantid::Types::Core::DateAndTime algExecutionDate =
            item.getAlgorithmHistory()->executionDate();
        return algExecutionDate < start || algExecutionDate > end;
      });
  m_historyItems.erase(lastItem, m_historyItems.end());
}

} // namespace API
} // namespace Mantid
