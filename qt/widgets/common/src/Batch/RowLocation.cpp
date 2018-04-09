#include "MantidQtWidgets/Common/Batch/RowLocation.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

RowLocation::RowLocation(RowPath path) : m_path(std::move(path)) {}
RowPath const &RowLocation::path() const { return m_path; }
int RowLocation::rowRelativeToParent() const { return m_path.back(); }
bool RowLocation::isRoot() const { return m_path.empty(); }

}
}
}
