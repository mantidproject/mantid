#include "MantidQtMantidWidgets/DataProcessorUI/AbstractDataProcessorTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt {
namespace MantidWidgets {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor: initialise member variables common to all data processing tree
* model implementations
* @param tableWorkspace : The table workspace to wrap
* @param whitelist : A DataProcessorWhiteList containing the columns
*/
AbstractDataProcessorTreeModel::AbstractDataProcessorTreeModel(
    ITableWorkspace_sptr tableWorkspace,
    const DataProcessorWhiteList &whitelist)
    : m_tWS(tableWorkspace), m_whitelist(whitelist) {}

AbstractDataProcessorTreeModel::~AbstractDataProcessorTreeModel() {}

/** Returns the number of columns, i.e. elements in the whitelist
* @return : The number of columns
*/
int AbstractDataProcessorTreeModel::columnCount(
    const QModelIndex & /* parent */) const {
  return static_cast<int>(m_whitelist.size());
}

/** Returns the flags for a specific data item. If the index is valid, the item
* is always editable.
* @return : The item flags
*/
Qt::ItemFlags
AbstractDataProcessorTreeModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

} // namespace MantidWidgets
} // namespace Mantid