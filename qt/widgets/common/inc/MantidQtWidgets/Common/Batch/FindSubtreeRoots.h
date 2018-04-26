#ifndef MANTIDQTMANTIDWIDGETS_FINDSUBTREEROOTS_H_
#define MANTIDQTMANTIDWIDGETS_FINDSUBTREEROOTS_H_
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON FindSubtreeRoots {
public:
  boost::optional<std::vector<RowLocation>>
  operator()(std::vector<RowLocation> region);
  void nodeWasSubtreeRoot(RowLocation const &rowLocation);
  void nodeWasNotSubtreeRoot(RowLocation const &rowLocation);
  bool isChildOfPrevious(RowLocation const &location) const;
  bool isSiblingOfPrevious(RowLocation const &location) const;

private:
  bool previousWasRoot;
  RowLocation previousNode;
};

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
