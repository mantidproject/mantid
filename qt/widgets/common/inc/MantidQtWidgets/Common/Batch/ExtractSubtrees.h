#ifndef MANTIDQTMANTIDWIDGETS_EXTRACTSUBTREES_H_
#define MANTIDQTMANTIDWIDGETS_EXTRACTSUBTREES_H_
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

using Row = std::vector<std::string>;
using Subtree = std::vector<std::pair<RowLocation, Row>>;

class EXPORT_OPT_MANTIDQT_COMMON ExtractSubtrees {
public:
  using Row = std::vector<std::string>;
  using Subtree = std::vector<std::pair<RowLocation, Row>>;

  boost::optional<std::vector<Subtree>>
  operator()(std::vector<RowLocation> region, std::vector<Row> regionData);
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
