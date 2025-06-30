#pragma once

/* Hide deprecated API from HDF5 versions before 1.8
 * Required to build on Ubuntu 12.04 */

#include <hdf5.h>

/* HDF5 interface */

NXstatus NX5open(CONSTCHAR *filename, NXaccess access_method, NXhandle &handle);

NXstatus NX5putattr(NXhandle handle, CONSTCHAR *name, const void *data, int iDataLen, NXnumtype iType);

NXstatus NX5getdata(NXhandle handle, void *data);
NXstatus NX5getinfo64(NXhandle handle, int *rank, int64_t dimension[], NXnumtype *datatype);
NXstatus NX5getnextentry(NXhandle handle, NXname name, NXname nxclass, NXnumtype *datatype);

NXstatus NX5getnextattr(NXhandle handle, NXname pName, int *iLength, NXnumtype *iType);
NXstatus NX5getattr(NXhandle handle, const char *name, void *data, int *iDataLen, NXnumtype *iType);
NXstatus NX5getattrinfo(NXhandle handle, int *no_items);
NXstatus NX5getgroupinfo(NXhandle handle, int *no_items, NXname name, NXname nxclass);

NXstatus NX5getnextattra(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType);
NXstatus NX5getattrainfo(NXhandle handle, CONSTCHAR *pName, int *rank, int dim[], NXnumtype *iType);

herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *unused, void *opdata);
herr_t group_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *opdata);
herr_t nxgroup_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *op_data);
