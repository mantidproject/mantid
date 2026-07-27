#include "MantidNexus/UniqueID.h"
#include "MantidNexus/DllConfig.h"
#include <hdf5.h>

namespace Mantid::Nexus {

bool H5_id_is_valid(hid_t id) {
  // fail early condition
  if (id <= 0) {
    return false;
  } else {
    return H5Iis_valid(id) > 0;
  }
}

// ******************************************************************
// EXPORTS
// ******************************************************************

template class MANTID_NEXUS_DLL UniqueID<&H5Fclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Gclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Dclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Tclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Sclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Aclose>;
template class MANTID_NEXUS_DLL UniqueID<&H5Pclose>;
// this will be used in Nexus::File
template class SharedID<&H5Fclose>;

} // namespace Mantid::Nexus
