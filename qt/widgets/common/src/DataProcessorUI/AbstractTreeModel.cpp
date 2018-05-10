#include "MantidQtWidgets/Common/DataProcessorUI/AbstractTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor: initialise member variables common to all data processing tree
 * model implementations
 * @param tableWorkspace : The table workspace to wrap
 * @param whitelist : A WhiteList containing the columns
 */
AbstractTreeModel::AbstractTreeModel(ITableWorkspace_sptr tableWorkspace,
                                     const WhiteList &whitelist)
    : m_tWS(tableWorkspace), m_whitelist(whitelist) {}

AbstractTreeModel::~AbstractTreeModel() {}

/** Returns the number of columns, i.e. elements in the whitelist
 * @return : The number of columns
 */
int AbstractTreeModel::columnCount(const QModelIndex & /* parent */) const {
  return static_cast<int>(m_whitelist.size());
}

/** Returns the flags for a specific data item. If the index is valid, the item
 * is always editable.
 * @return : The item flags
 */
Qt::ItemFlags AbstractTreeModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;

  return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
