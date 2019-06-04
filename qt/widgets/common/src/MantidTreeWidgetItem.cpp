// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MantidTreeWidgetItem.h"
#include "MantidQtWidgets/Common/MantidTreeWidget.h"

#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/Workspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace MantidQt {
namespace MantidWidgets {
/**Constructor.
 * Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
 */
MantidTreeWidgetItem::MantidTreeWidgetItem(MantidTreeWidget *parent)
    : QTreeWidgetItem(parent), m_parent(parent), m_sortPos(0) {}

/**Constructor.
 * Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
 */
MantidTreeWidgetItem::MantidTreeWidgetItem(QStringList list,
                                           MantidTreeWidget *parent)
    : QTreeWidgetItem(list), m_parent(parent), m_sortPos(0) {}

/**Overidden operator.
 * Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
 */
bool MantidTreeWidgetItem::operator<(const QTreeWidgetItem &other) const {
  // If this and/or other has been set to have a Qt::UserRole, then
  // it has an accompanying sort order that we must maintain, no matter
  // what the user has seletected in terms of order or scheme.

  bool thisShouldBeSorted = m_sortPos == 0;
  const MantidTreeWidgetItem *mantidOther =
      dynamic_cast<const MantidTreeWidgetItem *>(&other);
  int otherSortPos = mantidOther ? mantidOther->getSortPos() : 0;
  bool otherShouldBeSorted = otherSortPos == 0;

  // just in case m_parent is NULL. I think I saw this once but cannot
  // reproduce.
  if (!m_parent)
    return false;

  if (!thisShouldBeSorted && !otherShouldBeSorted) {
    if (m_parent->getSortOrder() == Qt::AscendingOrder)
      return m_sortPos < otherSortPos;
    else
      return m_sortPos >= otherSortPos;
  } else if (thisShouldBeSorted && !otherShouldBeSorted) {
    if (m_parent->getSortOrder() == Qt::AscendingOrder)
      return false;
    else
      return true;
  } else if (!thisShouldBeSorted && otherShouldBeSorted) {
    if (m_parent->getSortOrder() == Qt::AscendingOrder)
      return true;
    else
      return false;
  }

  // If both should be sorted, and the scheme is set to ByName ...
  if (m_parent->getSortScheme() == MantidItemSortScheme::ByName) {
    return QString::compare(text(0), other.text(0), Qt::CaseInsensitive) < 0;
  }
  // If both should be sorted and the scheme is set to ByMemorySize ...
  else if (m_parent->getSortScheme() == MantidItemSortScheme::ByMemorySize) {
    return this->getMemorySize() < mantidOther->getMemorySize();
  }
  // ... else both should be sorted and the scheme is set to ByLastModified.
  else {
    try {
      if (childCount() > 0 && other.childCount() > 0) {
        const QTreeWidgetItem *other_ptr = &other;

        try {
          return getLastModified(this) < getLastModified(other_ptr);
        } catch (std::out_of_range &e) {
          m_parent->logWarningMessage(e.what());
          return false;
        }
      }
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      ;
    }
    return false;
  }
}

/**Finds the date and time of the last modification made to the workspace who's
 * details
 * are found in the given QTreeWidgetItem.
 */
DateAndTime MantidTreeWidgetItem::getLastModified(const QTreeWidgetItem *item) {
  QVariant userData = item->data(0, Qt::UserRole);
  if (userData.isNull())
    return DateAndTime(); // now

  Workspace_sptr workspace = userData.value<Workspace_sptr>();
  const Mantid::API::WorkspaceHistory &wsHist = workspace->getHistory();
  if (wsHist.empty())
    return DateAndTime(); // now

  const size_t indexOfLast = wsHist.size() - 1;
  const auto lastAlgHist = wsHist.getAlgorithmHistory(indexOfLast);
  return lastAlgHist->executionDate();
}
std::size_t MantidTreeWidgetItem::getMemorySize() const {
  return this->data(0, Qt::UserRole).value<Workspace_sptr>()->getMemorySize();
}
} // namespace MantidWidgets
} // namespace MantidQt
