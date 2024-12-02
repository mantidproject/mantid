#ifndef NAPI4_H
#define NAPI4_H

#define NXSIGNATURE 959697

#include "mfhdf.h"

/*
 * HDF4 interface
 */

extern NXstatus NX4open(CONSTCHAR *filename, NXaccess access_method, NXhandle *pHandle);
extern NXstatus NX4close(NXhandle *pHandle);
extern NXstatus NX4flush(NXhandle *pHandle);

extern NXstatus NX4makegroup(NXhandle handle, CONSTCHAR *Vgroup, CONSTCHAR *NXclass);
extern NXstatus NX4opengroup(NXhandle handle, CONSTCHAR *Vgroup, CONSTCHAR *NXclass);
extern NXstatus NX4closegroup(NXhandle handle);

extern NXstatus NX4makedata64(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int64_t dim[]);
extern NXstatus NX4compmakedata64(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int64_t dim[],
                                  int comp_typ, int64_t bufsize[]);
extern NXstatus NX4compress(NXhandle handle, int compr_type);
extern NXstatus NX4opendata(NXhandle handle, CONSTCHAR *label);

extern NXstatus NX4closedata(NXhandle handle);

extern NXstatus NX4getdata(NXhandle handle, void *data);
extern NXstatus NX4getslab64(NXhandle handle, void *data, const int64_t start[], const int64_t size[]);
extern NXstatus NX4getattr(NXhandle handle, const char *name, void *data, int *iDataLen, int *iType);

extern NXstatus NX4putdata(NXhandle handle, const void *data);
extern NXstatus NX4putslab64(NXhandle handle, const void *data, const int64_t start[], const int64_t size[]);
extern NXstatus NX4putattr(NXhandle handle, CONSTCHAR *name, const void *data, int iDataLen, int iType);

extern NXstatus NX4getinfo64(NXhandle handle, int *rank, int64_t dimension[], int *datatype);
extern NXstatus NX4getgroupinfo(NXhandle handle, int *no_items, NXname name, NXname nxclass);
extern NXstatus NX4initgroupdir(NXhandle handle);
extern NXstatus NX4getnextentry(NXhandle handle, NXname name, NXname nxclass, int *datatype);
extern NXstatus NX4getattrinfo(NXhandle handle, int *no_items);
extern NXstatus NX4initattrdir(NXhandle handle);
extern NXstatus NX4getnextattr(NXhandle handle, NXname pName, int *iLength, int *iType);

extern NXstatus NX4getgroupID(NXhandle handle, NXlink *pLink);
extern NXstatus NX4getdataID(NXhandle handle, NXlink *pLink);
extern NXstatus NX4makelink(NXhandle handle, NXlink *pLink);
extern NXstatus NX4printlink(NXhandle handle, NXlink *pLink);

extern NXstatus NX4putattra(NXhandle handle, CONSTCHAR *name, const void *data, const int rank, const int dim[],
                            const int iType);
extern NXstatus NX4getnextattra(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);
extern NXstatus NX4getattra(NXhandle handle, const char *name, void *data);
extern NXstatus NX4getattrainfo(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);

void NX4assignFunctions(pNexusFunction fHandle);

/*
 *  HDF changed from MAX_VAR_DIMS to H4_MAX_VAR_DIMS aronud 9/5/2007
 *  to avoid potential conflicts with NetCDF-3 library
 */
#ifndef H4_MAX_VAR_DIMS
#define H4_MAX_VAR_DIMS MAX_VAR_DIMS
#endif

#endif /* NAPI4_H */
