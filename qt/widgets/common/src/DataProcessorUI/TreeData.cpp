#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
bool canPostprocess(GroupData const &group) { return group.size() > 1; }
}
}
}
