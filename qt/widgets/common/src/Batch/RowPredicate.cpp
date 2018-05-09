#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"
#include <iostream>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

bool RowPredicate::operator()(RowLocation const &rowLocation) const {
  return rowMeetsCriteria(rowLocation);
}

}
}
}
