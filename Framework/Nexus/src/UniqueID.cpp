#include "MantidNexus/UniqueID.h"
#include "MantidNexus/DllConfig.h"
#include <hdf5.h>

namespace Mantid::Nexus {

FileID::~FileID() {
  if (this->isValid()) {
    H5Fclose(this->m_id);
    // H5garbage_collect();
    this->m_id = INVALID_ID;
  }
}

FileID &FileID::operator=(hid_t const id) {
  this->reset(id);
  return *this;
}

template class MANTID_NEXUS_DLL UniqueID<&H5Gclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Dclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Tclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Sclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Aclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Pclose>;

} // namespace Mantid::Nexus
