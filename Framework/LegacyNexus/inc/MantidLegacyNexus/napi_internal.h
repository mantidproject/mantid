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

#include <memory>

using Mantid::LegacyNexus::CONSTCHAR;
using Mantid::LegacyNexus::NXaccess;
using Mantid::LegacyNexus::NXhandle;
using Mantid::LegacyNexus::NXlink;
using Mantid::LegacyNexus::NXname;
using Mantid::LegacyNexus::NXnumtype;
using Mantid::LegacyNexus::NXstatus;

struct LgcyFunction {
  NXhandle pNexusData = nullptr;
  NXstatus (*nxclose)(NXhandle *pHandle) = nullptr;
  NXstatus (*nxopengroup)(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass) = nullptr;
  NXstatus (*nxclosegroup)(NXhandle handle) = nullptr;
  NXstatus (*nxopendata)(NXhandle handle, CONSTCHAR *label) = nullptr;
  NXstatus (*nxclosedata)(NXhandle handle) = nullptr;
  NXstatus (*nxputdata)(NXhandle handle, const void *data) = nullptr;
  NXstatus (*nxgetdataID)(NXhandle handle, NXlink *pLink) = nullptr;
  NXstatus (*nxgetdata)(NXhandle handle, void *data) = nullptr;
  NXstatus (*nxgetinfo64)(NXhandle handle, int *rank, int64_t dimension[], NXnumtype *datatype) = nullptr;
  NXstatus (*nxgetnextentry)(NXhandle handle, NXname name, NXname nxclass, NXnumtype *datatype) = nullptr;
  NXstatus (*nxgetnextattr)(NXhandle handle, NXname pName, int *iLength, NXnumtype *iType) = nullptr;
  NXstatus (*nxgetnextattra)(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType) = nullptr;
  NXstatus (*nxgetattr)(NXhandle handle, const char *name, void *data, int *iDataLen, NXnumtype *iType) = nullptr;
  NXstatus (*nxgetattrainfo)(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType) = nullptr;
  NXstatus (*nxgetattrinfo)(NXhandle handle, int *no_items) = nullptr;
  NXstatus (*nxgetgroupID)(NXhandle handle, NXlink *pLink) = nullptr;
  NXstatus (*nxinitgroupdir)(NXhandle handle) = nullptr;
  NXstatus (*nxinitattrdir)(NXhandle handle) = nullptr;
  int stripFlag = 0;
  int checkNameSyntax = 0;
  NXaccess access_mode = 0;
};

using LgcyFunctionPtr = std::unique_ptr<LgcyFunction>;

/*---------------------*/
extern long nx_cacheSize;
