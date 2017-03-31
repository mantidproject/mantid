#include "MantidAPI/NonMasterDummyWorkspace.h"

namespace Mantid {
namespace API {

NonMasterDummyWorkspace::NonMasterDummyWorkspace(
    const Parallel::Communicator &communicator)
    : m_communicator(communicator) {
  if (m_communicator.rank() == 0)
    throw std::runtime_error(
        "NonMasterDummyWorkspace cannot be created on the master rank.");
  setStorageMode(Parallel::StorageMode::MasterOnly);
}

const std::string NonMasterDummyWorkspace::id() const {
  return "NonMasterDummyWorkspace";
}

const std::string NonMasterDummyWorkspace::toString() const { return {}; }

size_t NonMasterDummyWorkspace::getMemorySize() const { return 0; }

NonMasterDummyWorkspace *NonMasterDummyWorkspace::doClone() const {
  return new NonMasterDummyWorkspace(*this);
}

} // namespace API
} // namespace Mantid
