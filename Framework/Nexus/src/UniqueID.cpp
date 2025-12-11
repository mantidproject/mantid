#include "MantidNexus/UniqueID.h"
#include "MantidNexus/DllConfig.h"
#include <hdf5.h>

namespace Mantid::Nexus {

template <> void UniqueID<&H5Fclose>::close() {
  if (isValid()) {
    H5Fclose(this->m_id);
    this->m_id = INVALID_ID;
    // call garbage collection to close any and all open objects on this file
    H5garbage_collect();
  }
}

template class MANTID_NEXUS_DLL UniqueID<&H5Gclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Dclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Tclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Sclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Aclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Pclose>;

} // namespace Mantid::Nexus
