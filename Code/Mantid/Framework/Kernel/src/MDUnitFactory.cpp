#include "MantidKernel/MDUnitFactory.h"
#include "MantidKernel/MDUnit.h"
#include <memory>

namespace Mantid {
namespace Kernel {

/**
 * set successor factory
 * @param successor : successor factory
 * @return ref to successor
 */

std::unique_ptr<MDUnit>
MDUnitFactory::create(const std::string &unitString) const {
  if (this->canInterpret(unitString)) {
    return MDUnit_uptr(this->createRaw(unitString));
  } else {
    if (this->hasSuccessor()) {
      return (*m_successor)->create(unitString);
    } else {
      throw std::invalid_argument("No successor MDUnitFactory");
    }
  }
}

} // namespace Kernel
} // namespace Mantid
