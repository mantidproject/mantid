#ifndef MANTIDQTMANTIDWIDGETS_ROWPREDICATE_H_
#define MANTIDQTMANTIDWIDGETS_ROWPREDICATE_H_
#include "MantidQtWidgets/Common/Batch/RowLocation.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
class RowPredicate {
public:
  bool operator()(RowLocation const &row) const;
  virtual ~RowPredicate() = default;

protected:
  virtual bool rowMeetsCriteria(RowLocation const &row) const = 0;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_ROWPREDICATE_H_
