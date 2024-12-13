/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format

  Application Program Interface Header File

  Copyright (C) 2015 NeXus International Advisory Committee

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  For further information, see <http://www.nexusformat.org>

 ----------------------------------------------------------------------------*/

#ifndef NEXUS_INTERNAL_API
#define NEXUS_INTERNAL_API

#include <MantidNexusCpp/napi.h>

/*-----------------------------------------------------------------------
    NAPI internals
------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  NXhandle pNexusData;
  NXstatus (*nxreopen)(NXhandle pOrigHandle, NXhandle *pNewHandle);
  NXstatus (*nxclose)(NXhandle *pHandle);
  NXstatus (*nxflush)(NXhandle *pHandle);
  NXstatus (*nxmakegroup)(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);
  NXstatus (*nxopengroup)(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);
  NXstatus (*nxclosegroup)(NXhandle handle);
  NXstatus (*nxmakedata64)(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int64_t dim[]);
  NXstatus (*nxcompmakedata64)(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int64_t dim[], int comp_typ,
                               int64_t bufsize[]);
  NXstatus (*nxcompress)(NXhandle handle, int compr_type);
  NXstatus (*nxopendata)(NXhandle handle, CONSTCHAR *label);
  NXstatus (*nxclosedata)(NXhandle handle);
  NXstatus (*nxputdata)(NXhandle handle, const void *data);
  NXstatus (*nxputattr)(NXhandle handle, CONSTCHAR *name, const void *data, int iDataLen, int iType);
  NXstatus (*nxputattra)(NXhandle handle, CONSTCHAR *name, const void *data, const int rank, const int dim[],
                         const int iType);
  NXstatus (*nxputslab64)(NXhandle handle, const void *data, const int64_t start[], const int64_t size[]);
  NXstatus (*nxgetdataID)(NXhandle handle, NXlink *pLink);
  NXstatus (*nxmakelink)(NXhandle handle, NXlink *pLink);
  NXstatus (*nxmakenamedlink)(NXhandle handle, CONSTCHAR *newname, NXlink *pLink);
  NXstatus (*nxgetdata)(NXhandle handle, void *data);
  NXstatus (*nxgetinfo64)(NXhandle handle, int *rank, int64_t dimension[], int *datatype);
  NXstatus (*nxgetnextentry)(NXhandle handle, NXname name, NXname nxclass, int *datatype);
  NXstatus (*nxgetslab64)(NXhandle handle, void *data, const int64_t start[], const int64_t size[]);
  NXstatus (*nxgetnextattr)(NXhandle handle, NXname pName, int *iLength, int *iType);
  NXstatus (*nxgetnextattra)(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);
  NXstatus (*nxgetattr)(NXhandle handle, const char *name, void *data, int *iDataLen, int *iType);
  NXstatus (*nxgetattra)(NXhandle handle, const char *name, void *data);
  NXstatus (*nxgetattrainfo)(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);
  NXstatus (*nxgetattrinfo)(NXhandle handle, int *no_items);
  NXstatus (*nxgetgroupID)(NXhandle handle, NXlink *pLink);
  NXstatus (*nxgetgroupinfo)(NXhandle handle, int *no_items, NXname name, NXname nxclass);
  NXstatus (*nxsameID)(NXhandle handle, NXlink *pFirstID, NXlink *pSecondID);
  NXstatus (*nxinitgroupdir)(NXhandle handle);
  NXstatus (*nxinitattrdir)(NXhandle handle);
  NXstatus (*nxsetnumberformat)(NXhandle handle, const int type, const char *format);
  NXstatus (*nxprintlink)(NXhandle handle, NXlink *link);
  NXstatus (*nxnativeexternallink)(NXhandle handle, CONSTCHAR *name, CONSTCHAR *externalfile, CONSTCHAR *remotetarget);
  NXstatus (*nxnativeinquirefile)(NXhandle handle, char *externalfile, const int filenamelength);
  NXstatus (*nxnativeisexternallink)(NXhandle handle, CONSTCHAR *name, char *url, int urllen);
  int stripFlag;
  int checkNameSyntax;
  NXaccess access_mode;
} NexusFunction, *pNexusFunction;
/*---------------------*/
extern long nx_cacheSize;

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* NEXUS_INTERNAL_API */
