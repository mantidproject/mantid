#pragma once

/* Hide deprecated API from HDF5 versions before 1.8
 * Required to build on Ubuntu 12.04 */

#include "MantidNexus/NexusFile_fwd.h"
#include <hdf5.h>

/* HDF5 interface */

NXstatus NX5open(std::string const &filename, NXaccess access_method, NXhandle &handle);

NXstatus NX5putattr(NXhandle handle, std::string const &name, const void *data, std::size_t const iDataLen,
                    NXnumtype const iType);

NXstatus NX5getdata(NXhandle handle, void *data);
NXstatus NX5getinfo64(NXhandle handle, std::size_t &rank, Mantid::Nexus::DimVector &dims, NXnumtype &datatype);
NXstatus NX5getnextentry(NXhandle handle, std::string &name, std::string &nxclass, NXnumtype &datatype);

NXstatus NX5getnextattr(NXhandle handle, std::string &pName, std::size_t &iLength, NXnumtype &iType);
NXstatus NX5getattr(NXhandle handle, std::string const &name, void *data, std::size_t &iDataLen, NXnumtype &iType);
NXstatus NX5getattrinfo(NXhandle handle, std::size_t &no_items);
NXstatus NX5getgroupinfo(NXhandle handle, std::size_t &no_items, std::string &name, std::string &nxclass);

NXstatus NX5getnextattra(NXhandle handle, std::string &pName, std::size_t &rank, Mantid::Nexus::DimVector &dim,
                         NXnumtype &iType);
NXstatus NX5getattrainfo(NXhandle handle, std::string const &pName, std::size_t &rank, Mantid::Nexus::DimVector &dim,
                         NXnumtype &iType);

herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *unused, void *opdata);
herr_t group_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *opdata);
herr_t nxgroup_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *op_data);
