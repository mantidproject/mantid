#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorVectorString.h"
namespace MantidQt {
namespace MantidWidgets {
/** Create a comma separated list of items from a QStringList.
@param items : The strings in the list.
@return The comma separated list of items.
*/
QString vectorString(const QStringList &items) { return items.join(", "); }
}
}
