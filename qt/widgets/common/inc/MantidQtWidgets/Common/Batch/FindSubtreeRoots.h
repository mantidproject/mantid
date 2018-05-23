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
private:
  void removeIfDepthNotEqualTo(std::vector<RowLocation> &region,
                               int expectedDepth) const;
};

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
