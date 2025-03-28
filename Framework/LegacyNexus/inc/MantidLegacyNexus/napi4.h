#pragma once

#define NXSIGNATURE 959697

#include "mfhdf.h"

using namespace Mantid::LegacyNexus;

/*
 * HDF4 interface
 */

extern NXstatus NX4open(CONSTCHAR *filename, NXaccess access_method, NXhandle *pHandle);
extern NXstatus NX4close(NXhandle *pHandle);

extern NXstatus NX4opengroup(NXhandle handle, CONSTCHAR *Vgroup, CONSTCHAR *NXclass);
extern NXstatus NX4closegroup(NXhandle handle);

extern NXstatus NX4opendata(NXhandle handle, CONSTCHAR *label);

extern NXstatus NX4closedata(NXhandle handle);

extern NXstatus NX4getdata(NXhandle handle, void *data);
extern NXstatus NX4getattr(NXhandle handle, const char *name, void *data, int *iDataLen, NXnumtype *iType);

extern NXstatus NX4getinfo64(NXhandle handle, int *rank, int64_t dimension[], NXnumtype *datatype);
extern NXstatus NX4initgroupdir(NXhandle handle);
extern NXstatus NX4getnextentry(NXhandle handle, NXname name, NXname nxclass, NXnumtype *datatype);
extern NXstatus NX4initattrdir(NXhandle handle);
extern NXstatus NX4getnextattr(NXhandle handle, NXname pName, int *iLength, NXnumtype *iType);

extern NXstatus NX4getgroupID(NXhandle handle, NXlink *pLink);
extern NXstatus NX4getdataID(NXhandle handle, NXlink *pLink);

extern NXstatus NX4getnextattra(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType);

void NX4assignFunctions(LgcyFunction &fHandle);

/*
 *  HDF changed from MAX_VAR_DIMS to H4_MAX_VAR_DIMS aronud 9/5/2007
 *  to avoid potential conflicts with NetCDF-3 library
 */
#ifndef H4_MAX_VAR_DIMS
#define H4_MAX_VAR_DIMS MAX_VAR_DIMS
#endif
