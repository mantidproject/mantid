#pragma once

#define NX5SIGNATURE 959695

/* Hide deprecated API from HDF5 versions before 1.8
 * Required to build on Ubuntu 12.04 */

#include <hdf5.h>

/* HDF5 interface */

NXstatus NX5open(CONSTCHAR *filename, NXaccess access_method, NXhandle &handle);
NXstatus NX5reopen(NXhandle origHandle, NXhandle &newHandle);

NXstatus NX5close(NXhandle &handle);
NXstatus NX5flush(NXhandle &handle);

NXstatus NX5makegroup(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);
NXstatus NX5opengroup(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);
NXstatus NX5closegroup(NXhandle handle);

NXstatus NX5makedata64(NXhandle handle, CONSTCHAR *label, NXnumtype datatype, int rank, int64_t dim[]);
NXstatus NX5compmakedata64(NXhandle handle, CONSTCHAR *label, NXnumtype datatype, int rank, int64_t dim[], int comp_typ,
                           int64_t const bufsize[]);
NXstatus NX5compress(NXhandle handle, int compr_type);
NXstatus NX5opendata(NXhandle handle, CONSTCHAR *label);
NXstatus NX5closedata(NXhandle handle);
NXstatus NX5putdata(NXhandle handle, const void *data);

NXstatus NX5putattr(NXhandle handle, CONSTCHAR *name, const void *data, int iDataLen, NXnumtype iType);
NXstatus NX5putslab64(NXhandle handle, const void *data, const int64_t start[], const int64_t size[]);

NXstatus NX5getdataID(NXhandle handle, NXlink *pLink);
NXstatus NX5makelink(NXhandle handle, NXlink *pLink);
NXstatus NX5makenamedlink(NXhandle fid, CONSTCHAR *name, NXlink *sLink);
NXstatus NX5printlink(NXhandle handle, NXlink const *pLink);
NXstatus NX5sameID(NXhandle fileid, NXlink const *pFirstID, NXlink const *pSecondID);

NXstatus NX5getdata(NXhandle handle, void *data);
NXstatus NX5getinfo64(NXhandle handle, int *rank, int64_t dimension[], NXnumtype *datatype);
NXstatus NX5getnextentry(NXhandle handle, NXname name, NXname nxclass, NXnumtype *datatype);

NXstatus NX5getslab64(NXhandle handle, void *data, const int64_t start[], const int64_t size[]);
NXstatus NX5getnextattr(NXhandle handle, NXname pName, int *iLength, NXnumtype *iType);
NXstatus NX5getattr(NXhandle handle, const char *name, void *data, int *iDataLen, NXnumtype *iType);
NXstatus NX5getattrinfo(NXhandle handle, int *no_items);
NXstatus NX5getgroupID(NXhandle handle, NXlink *pLink);
NXstatus NX5getgroupinfo(NXhandle handle, int *no_items, NXname name, NXname nxclass);

NXstatus NX5initgroupdir(NXhandle handle);
NXstatus NX5initattrdir(NXhandle handle);

NXstatus NX5getnextattra(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType);
NXstatus NX5getattrainfo(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType);

herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *unused, void *opdata);
herr_t group_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *opdata);
herr_t nxgroup_info(hid_t loc_id, const char *name, const H5L_info_t *unused, void *op_data);

hid_t nxToHDF5Type(NXnumtype datatype);
