/*
 * This is the header file for the NeXus XML file driver.
 *
 *   Copyright (C) 2004 Mark Koennecke
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  For further information, see <http://www.nexusformat.org>
 */
#ifndef NEXUSXML
#define NEXUSXML

extern NXstatus NXXopen(CONSTCHAR *filename, NXaccess access_method, NXhandle *pHandle);
extern NXstatus NXXclose(NXhandle *pHandle);
extern NXstatus NXXflush(NXhandle *pHandle);

NXstatus NXXmakegroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass);
NXstatus NXXopengroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass);
NXstatus NXXclosegroup(NXhandle fid);

NXstatus NXXcompmakedata64(NXhandle fid, CONSTCHAR *name, int datatype, int rank, int64_t dimensions[],
                           int compress_type, int64_t chunk_size[]);
NXstatus NXXmakedata64(NXhandle fid, CONSTCHAR *name, int datatype, int rank, int64_t dimensions[]);
NXstatus NXXopendata(NXhandle fid, CONSTCHAR *name);
NXstatus NXXclosedata(NXhandle fid);
NXstatus NXXputdata(NXhandle fid, const void *data);
NXstatus NXXgetdata(NXhandle fid, void *data);
NXstatus NXXgetinfo64(NXhandle fid, int *rank, int64_t dimension[], int *iType);
NXstatus NXXputslab64(NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[]);
NXstatus NXXgetslab64(NXhandle fid, void *data, const int64_t iStart[], const int64_t iSize[]);
NXstatus NXXputattr(NXhandle fid, CONSTCHAR *name, const void *data, int datalen, int iType);
NXstatus NXXgetattr(NXhandle fid, const char *name, void *data, int *datalen, int *iType);

NXstatus NXXgetnextentry(NXhandle fid, NXname name, NXname nxclass, int *datatype);
extern NXstatus NXXgetnextattr(NXhandle handle, NXname pName, int *iLength, int *iType);
extern NXstatus NXXinitgroupdir(NXhandle handle);
extern NXstatus NXXinitattrdir(NXhandle handle);
extern NXstatus NXXgetattrinfo(NXhandle fid, int *iN);
extern NXstatus NXXgetgroupinfo(NXhandle fid, int *iN, NXname pName, NXname pClass);

extern NXstatus NXXgetdataID(NXhandle fid, NXlink *sRes);
extern NXstatus NXXgetgroupID(NXhandle fid, NXlink *sRes);
extern NXstatus NXXmakelink(NXhandle fid, NXlink *sLink);
extern NXstatus NXXprintlink(NXhandle fid, NXlink *sLink);
extern NXstatus NXXsameID(NXhandle fileid, NXlink *pFirstID, NXlink *pSecondID);

extern NXstatus NXXputattra(NXhandle handle, CONSTCHAR *name, const void *data, const int rank, const int dim[],
                            const int iType);
extern NXstatus NXXgetnextattra(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);
extern NXstatus NXXgetattra(NXhandle handle, const char *name, void *data);
extern NXstatus NXXgetattrainfo(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);

void NXXassignFunctions(pNexusFunction fHandle);
#endif
