#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/Column.h"
#include <sstream>

namespace Mantid {
namespace API {

boost::shared_ptr<Column>
ColumnFactoryImpl::create(const std::string &type) const {
  boost::shared_ptr<Column> c = Kernel::DynamicFactory<Column>::create(type);
  c->m_type = type;
  return c;
}

} // namespace API
} // namespace Mantid
