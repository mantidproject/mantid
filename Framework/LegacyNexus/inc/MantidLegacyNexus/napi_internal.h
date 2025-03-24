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

#pragma once

#include <MantidLegacyNexus/NeXusFile_fwd.h>

/*-----------------------------------------------------------------------
    NAPI internals
------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

using Mantid::LegacyNexus::CONSTCHAR;
using Mantid::LegacyNexus::NXaccess;
using Mantid::LegacyNexus::NXhandle;
using Mantid::LegacyNexus::NXlink;
using Mantid::LegacyNexus::NXname;
using Mantid::LegacyNexus::NXnumtype;
using Mantid::LegacyNexus::NXstatus;

typedef struct {
  NXhandle pNexusData;
  NXstatus (*nxclose)(NXhandle *pHandle);
  NXstatus (*nxopengroup)(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);
  NXstatus (*nxclosegroup)(NXhandle handle);
  NXstatus (*nxopendata)(NXhandle handle, CONSTCHAR *label);
  NXstatus (*nxclosedata)(NXhandle handle);
  NXstatus (*nxputdata)(NXhandle handle, const void *data);
  NXstatus (*nxgetdataID)(NXhandle handle, NXlink *pLink);
  NXstatus (*nxgetdata)(NXhandle handle, void *data);
  NXstatus (*nxgetinfo64)(NXhandle handle, int *rank, int64_t dimension[], NXnumtype *datatype);
  NXstatus (*nxgetnextentry)(NXhandle handle, NXname name, NXname nxclass, NXnumtype *datatype);
  NXstatus (*nxgetnextattr)(NXhandle handle, NXname pName, int *iLength, NXnumtype *iType);
  NXstatus (*nxgetnextattra)(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType);
  NXstatus (*nxgetattr)(NXhandle handle, const char *name, void *data, int *iDataLen, NXnumtype *iType);
  NXstatus (*nxgetattrainfo)(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType);
  NXstatus (*nxgetattrinfo)(NXhandle handle, int *no_items);
  NXstatus (*nxgetgroupID)(NXhandle handle, NXlink *pLink);
  NXstatus (*nxinitgroupdir)(NXhandle handle);
  NXstatus (*nxinitattrdir)(NXhandle handle);
  int stripFlag;
  int checkNameSyntax;
  NXaccess access_mode;
} LgcyFunction, *pLgcyFunction;
/*---------------------*/
extern long nx_cacheSize;

#ifdef __cplusplus
};
#endif /* __cplusplus */
