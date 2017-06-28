#include "MantidQtMantidWidgets/DataProcessorUI/AbstractDataProcessorTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt {
namespace MantidWidgets {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
* @param tableWorkspace : The table workspace to wrap
* @param whitelist : A DataProcessorWhiteList containing the columns
*/
AbstractDataProcessorTreeModel::AbstractDataProcessorTreeModel(
    ITableWorkspace_sptr tableWorkspace,
    const DataProcessorWhiteList &whitelist)
    : m_tWS(tableWorkspace), m_whitelist(whitelist), m_highlighted(-1, -1){};

AbstractDataProcessorTreeModel::~AbstractDataProcessorTreeModel() {}

/** Returns the number of columns, i.e. elements in the whitelist
* @return : The number of columns
*/
int AbstractDataProcessorTreeModel::columnCount(
    const QModelIndex & /* parent */) const {
  return static_cast<int>(m_whitelist.size());
}

Qt::ItemFlags
AbstractDataProcessorTreeModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

/** Returns the currently highlighted row / group
* @return : The highlighted row / group as a pair of group and row indexes
*/
std::pair<int, int> AbstractDataProcessorTreeModel::getHighlighted() const {

  return m_highlighted;
}

} // namespace MantidWidgets
} // namespace Mantid 