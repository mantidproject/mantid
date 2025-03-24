#pragma once

#define NX5SIGNATURE 959695

/* Hide deprecated API from HDF5 versions before 1.8
 * Required to build on Ubuntu 12.04 */

#include <hdf5.h>

using namespace Mantid::LegacyNexus;

/* HDF5 interface */

extern NXstatus NX5open(CONSTCHAR *filename, NXaccess access_method, NXhandle *pHandle);

extern NXstatus NX5close(NXhandle *pHandle);

extern NXstatus NX5opengroup(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);
extern NXstatus NX5closegroup(NXhandle handle);

extern NXstatus NX5opendata(NXhandle handle, CONSTCHAR *label);
extern NXstatus NX5closedata(NXhandle handle);

extern NXstatus NX5getdataID(NXhandle handle, NXlink *pLink);

extern NXstatus NX5getdata(NXhandle handle, void *data);
extern NXstatus NX5getinfo64(NXhandle handle, int *rank, int64_t dimension[], NXnumtype *datatype);
extern NXstatus NX5getnextentry(NXhandle handle, NXname name, NXname nxclass, NXnumtype *datatype);

extern NXstatus NX5getnextattr(NXhandle handle, NXname pName, int *iLength, NXnumtype *iType);
extern NXstatus NX5getattr(NXhandle handle, const char *name, void *data, int *iDataLen, NXnumtype *iType);
extern NXstatus NX5getgroupID(NXhandle handle, NXlink *pLink);

extern NXstatus NX5initgroupdir(NXhandle handle);
extern NXstatus NX5initattrdir(NXhandle handle);

extern NXstatus NX5getattrainfo(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType);

void NX5assignFunctions(pLgcyFunction fHandle);

herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *unused, void *opdata);
herr_t group_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *opdata);
herr_t nxgroup_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *op_data);
