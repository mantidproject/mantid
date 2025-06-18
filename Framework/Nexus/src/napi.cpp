/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format

  Application Program Interface Routines

  Copyright (C) 1997-2014 NeXus International Advisory Committee

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

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <hdf5.h>

#include "MantidNexus/napi.h"

// cppcheck-suppress-begin [constVariablePointer, unmatchedSuppression]

// this has to be after the other napi includes
#include "MantidNexus/napi5.h"
#include "MantidNexus/napi_helper.h"

#define NX_UNKNOWN_GROUP "" /* for when no NX_class attr */

/*--------------------------------------------------------------------*/
/* static int iFortifyScope; */
/*----------------------------------------------------------------------
  This is a section with code for searching the NX_LOAD_PATH
  -----------------------------------------------------------------------*/

#ifdef WIN32
#define snprintf _snprintf
#define strdup _strdup
#endif

/*----------------------------------------------------------------------*/

void NXReportError(const char *string) { UNUSED_ARG(string); }

/* ----------------------------------------------------------------------

   Definition of NeXus API

   --------------------------------------------------------------------- */

static bool canBeOpened(CONSTCHAR *filename) {
  // this is for reading, check for existence first
  FILE *fd = NULL;
  fd = fopen(filename, "r");
  if (fd == NULL) {
    return false;
  }
  fclose(fd);

  // check that this is indeed hdf5
  if (H5Fis_hdf5(static_cast<const char *>(filename)) > 0) {
    return true;
  } else {
    // file type not recognized
    return false;
  }
}

/*----------------------------------------------------------------------*/
NXstatus NXopen(CONSTCHAR *userfilename, NXaccess am, NXhandle &gHandle) {
  // allocate the object on the heap
  gHandle = NULL;
  // determine the filename for opening
  std::string filename(userfilename);
  // determine if the file can be opened
  bool isHDF5 = (am == NXACC_CREATE5 ? true : canBeOpened(filename.c_str()));
  NXstatus status = NXstatus::NX_ERROR;
  if (isHDF5) {
    // call NX5open to set variables on it
    status = NX5open(filename.c_str(), am, gHandle);
  } else {
    NXReportError("ERROR: Format not readable by this NeXus library");
    status = NXstatus::NX_ERROR;
  }
  if (status != NXstatus::NX_OK && gHandle != NULL) {
    delete gHandle;
    gHandle = NULL;
  }

  return status;
}

/*-----------------------------------------------------------------------*/

NXstatus NXreopen(NXhandle origHandle, NXhandle &newHandle) {
  pNexusFile5 pNew = NULL, pOrig = NULL;
  newHandle = NULL;
  pOrig = static_cast<pNexusFile5>(origHandle);
  pNew = static_cast<pNexusFile5>(malloc(sizeof(NexusFile5)));
  if (!pNew) {
    NXReportError("ERROR: no memory to create File datastructure");
    return NXstatus::NX_ERROR;
  }
  memset(pNew, 0, sizeof(NexusFile5));
  pNew->iFID = H5Freopen(pOrig->iFID);
  if (pNew->iFID <= 0) {
    NXReportError("cannot clone file");
    free(pNew);
    return NXstatus::NX_ERROR;
  }
  pNew->iNXID = NX5SIGNATURE;
  pNew->iStack5[0].iVref = 0; /* root! */
  newHandle = static_cast<NXhandle>(pNew);
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------------- */

NXstatus NXclose(NXhandle &fid) {
  if (fid == NULL) {
    return NXstatus::NX_OK;
  }
  pNexusFile5 pFile = NXI5assert(fid);
  herr_t iRet = H5Fclose(pFile->iFID);
  if (iRet < 0) {
    NXReportError("ERROR: cannot close HDF file");
  }
  /* release memory */
  NXI5KillDir(pFile);
  free(pFile);
  fid = NULL;
  H5garbage_collect();
  return NXstatus::NX_OK;
}

/*-----------------------------------------------------------------------*/

NXstatus NXmakegroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  pNexusFile5 pFile;
  hid_t iVID;
  hid_t attr1, aid1, aid2;
  std::string pBuffer;

  pFile = NXI5assert(fid);
  /* create and configure the group */
  if (pFile->iCurrentG == 0) {
    pBuffer = "/" + std::string(name);
  } else {
    pBuffer = "/" + std::string(pFile->name_ref) + "/" + std::string(name);
  }
  iVID = H5Gcreate(pFile->iFID, pBuffer.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (iVID < 0) {
    NXReportError("ERROR: could not create Group");
    return NXstatus::NX_ERROR;
  }
  aid2 = H5Screate(H5S_SCALAR);
  aid1 = H5Tcopy(H5T_C_S1);
  H5Tset_size(aid1, strlen(nxclass));
  attr1 = H5Acreate(iVID, "NX_class", aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attr1 < 0) {
    NXReportError("ERROR: failed to store class name");
    return NXstatus::NX_ERROR;
  }
  if (H5Awrite(attr1, aid1, const_cast<char *>(static_cast<const char *>(nxclass))) < 0) {
    NXReportError("ERROR: failed to store class name");
    return NXstatus::NX_ERROR;
  }
  /* close group */
  hid_t iRet = H5Sclose(aid2);
  iRet += H5Tclose(aid1);
  iRet += H5Aclose(attr1);
  iRet += H5Gclose(iVID);
  UNUSED_ARG(iRet);
  // always return that it worked
  return NXstatus::NX_OK;
}

/*------------------------------------------------------------------------*/

NXstatus NXopengroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  pNexusFile5 pFile;
  hid_t attr1, atype, iVID;
  herr_t iRet;
  char pBuffer[NX_MAXADDRESSLEN + 12]; // no idea what the 12 is about

  pFile = NXI5assert(fid);
  if (pFile->iCurrentG == 0) {
    strcpy(pBuffer, name);
  } else {
    sprintf(pBuffer, "%s/%s", pFile->name_tmp, name);
  }
  iVID = H5Gopen(pFile->iFID, static_cast<const char *>(pBuffer), H5P_DEFAULT);
  if (iVID < 0) {
    std::string msg = std::string("ERROR: group ") + pFile->name_tmp + " does not exist";
    NXReportError(const_cast<char *>(msg.c_str()));
    return NXstatus::NX_ERROR;
  }
  pFile->iCurrentG = iVID;
  strcpy(pFile->name_tmp, pBuffer);
  strcpy(pFile->name_ref, pBuffer);

  if ((nxclass != NULL) && (strcmp(nxclass, NX_UNKNOWN_GROUP) != 0)) {
    /* check group attribute */
    iRet = H5Aiterate(pFile->iCurrentG, H5_INDEX_CRT_ORDER, H5_ITER_INC, 0, attr_check, NULL);
    if (iRet < 0) {
      NXReportError("ERROR: iterating through attribute list");
      return NXstatus::NX_ERROR;
    } else if (iRet == 1) {
      /* group attribute was found */
    } else {
      /* no group attribute available */
      NXReportError("ERROR: no group attribute available");
      return NXstatus::NX_ERROR;
    }
    /* check contents of group attribute */
    attr1 = H5Aopen_by_name(pFile->iCurrentG, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attr1 < 0) {
      NXReportError("ERROR: opening NX_class group attribute");
      return NXstatus::NX_ERROR;
    }
    atype = H5Tcopy(H5T_C_S1);
    char data[128];
    H5Tset_size(atype, sizeof(data));
    iRet = readStringAttributeN(attr1, data, sizeof(data));
    if (strcmp(data, nxclass) == 0) {
      /* test OK */
    } else {
      snprintf(pBuffer, sizeof(pBuffer), "ERROR: group class is not identical: \"%s\" != \"%s\"", data, nxclass);
      NXReportError(pBuffer);
      H5Tclose(atype);
      H5Aclose(attr1);
      return NXstatus::NX_ERROR;
    }
    H5Tclose(atype);
    H5Aclose(attr1);
  }

  /* maintain stack */
  pFile->iStackPtr++;
  pFile->iStack5[pFile->iStackPtr].iVref = pFile->iCurrentG;
  strcpy(pFile->iStack5[pFile->iStackPtr].irefn, name);
  pFile->iCurrentIDX = 0;
  pFile->iCurrentD = 0;
  NXI5KillDir(pFile);
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NXclosegroup(NXhandle fid) {
  pNexusFile5 pFile;

  pFile = NXI5assert(fid);
  /* first catch the trivial case: we are at root and cannot get
     deeper into a negative directory hierarchy (anti-directory)
   */
  if (pFile->iCurrentG == 0) {
    NXI5KillDir(pFile);
    return NXstatus::NX_OK;
  } else {
    /* close the current group and decrement name_ref */
    H5Gclose(pFile->iCurrentG);
    size_t i = strlen(pFile->iStack5[pFile->iStackPtr].irefn);
    size_t ii = strlen(pFile->name_ref);
    if (pFile->iStackPtr > 1) {
      ii = ii - i - 1;
    } else {
      ii = ii - i;
    }
    if (ii > 0) {
      char *uname = strdup(pFile->name_ref);
      char *u1name = NULL;
      u1name = static_cast<char *>(malloc((ii + 1) * sizeof(char)));
      memset(u1name, 0, ii);
      for (i = 0; i < ii; i++) {
        *(u1name + i) = *(uname + i);
      }
      *(u1name + i) = '\0';
      /*
         strncpy(u1name, uname, ii);
       */
      strcpy(pFile->name_ref, u1name);
      strcpy(pFile->name_tmp, u1name);
      free(uname);
      free(u1name);
    } else {
      strcpy(pFile->name_ref, "");
      strcpy(pFile->name_tmp, "");
    }
    NXI5KillDir(pFile);
    pFile->iStackPtr--;
    if (pFile->iStackPtr > 0) {
      pFile->iCurrentG = pFile->iStack5[pFile->iStackPtr].iVref;
    } else {
      pFile->iCurrentG = 0;
    }
  }
  return NXstatus::NX_OK;
}

/* --------------------------------------------------------------------- */

NXstatus NXmakedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[]) {
  return NX5makedata64(fid, name, datatype, rank, dimensions);
}

/* --------------------------------------------------------------------- */

NXstatus NXcompmakedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[],
                          int compress_type, int64_t const chunk_size[]) {
  return NX5compmakedata64(fid, name, datatype, rank, dimensions, compress_type, chunk_size);
}

/* --------------------------------------------------------------------- */

NXstatus NXopendata(NXhandle fid, CONSTCHAR *name) { return NX5opendata(fid, name); }

/* ----------------------------------------------------------------- */

NXstatus NXclosedata(NXhandle fid) { return NX5closedata(fid); }

/* ------------------------------------------------------------------- */

NXstatus NXputdata(NXhandle fid, const void *data) { return NX5putdata(fid, data); }

/* ------------------------------------------------------------------- */

NXstatus NXputattr(NXhandle fid, CONSTCHAR *name, const void *data, int datalen, NXnumtype iType) {
  if (datalen > 1 && iType != NXnumtype::CHAR) {
    NXReportError(
        "NXputattr: numeric arrays are not allowed as attributes - only character strings and single numbers");
    return NXstatus::NX_ERROR;
  }
  return NX5putattr(fid, name, data, datalen, iType);
}

NXstatus NXputslab64(NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[]) {
  return NX5putslab64(fid, data, iStart, iSize);
}

/* ---- --------------------------------------------------------------- */

NXstatus NXgetdataID(NXhandle fid, NXlink *sRes) { return NX5getdataID(fid, sRes); }

/* ------------------------------------------------------------------- */

NXstatus NXmakelink(NXhandle fid, NXlink *sLink) { return NX5makelink(fid, sLink); }

/* ----------------- -------------------------------------------------- */

NXstatus NXmakenamedlink(NXhandle fid, CONSTCHAR *newname, NXlink *sLink) {
  return NX5makenamedlink(fid, newname, sLink);
}

/* -------------------------------------------------------------------- */

/*----------------------------------------------------------------------*/

NXstatus NXflush(NXhandle &fid) {
  pNexusFile5 pFile = NULL;
  herr_t iRet;

  pFile = NXI5assert(fid);
  if (pFile->iCurrentD != 0) {
    iRet = H5Fflush(pFile->iCurrentD, H5F_SCOPE_LOCAL);
  } else if (pFile->iCurrentG != 0) {
    iRet = H5Fflush(pFile->iCurrentG, H5F_SCOPE_LOCAL);
  } else {
    iRet = H5Fflush(pFile->iFID, H5F_SCOPE_LOCAL);
  }
  if (iRet < 0) {
    NXReportError("ERROR: The object cannot be flushed");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

static int64_t *dupDimsArray(int const *dims_array, int rank) {
  int64_t *dims64 = static_cast<int64_t *>(malloc(static_cast<size_t>(rank) * sizeof(int64_t)));
  if (dims64 != NULL) {
    for (int i = 0; i < rank; ++i) {
      dims64[i] = dims_array[i];
    }
  }
  return dims64;
}

NXstatus NXmalloc(void **data, int rank, const int dimensions[], NXnumtype datatype) {
  int64_t *dims64 = dupDimsArray(static_cast<const int *>(dimensions), rank);
  NXstatus status = NXmalloc64(data, rank, dims64, datatype);
  free(dims64);
  return status;
}

NXstatus NXmalloc64(void **data, int rank, const int64_t dimensions[], NXnumtype datatype) {
  int i;
  size_t size = 1;
  *data = NULL;
  for (i = 0; i < rank; i++) {
    size *= (size_t)dimensions[i];
  }
  if ((datatype == NXnumtype::CHAR) || (datatype == NXnumtype::INT8) || (datatype == NXnumtype::UINT8)) {
    /* allow for terminating \0 */
    size += 2;
  } else if ((datatype == NXnumtype::INT16) || (datatype == NXnumtype::UINT16)) {
    size *= 2;
  } else if ((datatype == NXnumtype::INT32) || (datatype == NXnumtype::UINT32) || (datatype == NXnumtype::FLOAT32)) {
    size *= 4;
  } else if ((datatype == NXnumtype::INT64) || (datatype == NXnumtype::UINT64)) {
    size *= 8;
  } else if (datatype == NXnumtype::FLOAT64) {
    size *= 8;
  } else {
    NXReportError("ERROR: NXmalloc - unknown data type in array");
    return NXstatus::NX_ERROR;
  }
  *data = malloc(size);
  if (*data != NULL) {
    memset(*data, 0, size);
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NXfree(void **data) {
  if (data == NULL) {
    NXReportError("ERROR: passing NULL to NXfree");
    return NXstatus::NX_ERROR;
  }
  if (*data == NULL) {
    NXReportError("ERROR: passing already freed pointer to NXfree");
    return NXstatus::NX_ERROR;
  }
  free(*data);
  *data = NULL;
  return NXstatus::NX_OK;
}

/* --------------------------------------------------------------------- */

NXstatus NXgetnextentry(NXhandle fid, NXname name, NXname nxclass, NXnumtype *datatype) {
  return NX5getnextentry(fid, name, nxclass, datatype);
}

/*----------------------------------------------------------------------*/
/*
**  TRIM.C - Remove leading, trailing, & excess embedded spaces
**
**  public domain by Bob Stout
*/
#define NUL '\0'

char *nxitrim(char *str) {
  // Trap NULL
  if (str) {
    //  Remove leading spaces (from RMLEAD.C)
    char *ibuf;
    for (ibuf = str; *ibuf && isspace(*ibuf); ++ibuf)
      ;
    str = ibuf;

    // Remove trailing spaces (from RMTRAIL.C)
    int i = (int)strlen(str);
    while (--i >= 0) {
      if (!isspace(str[i]))
        break;
    }
    str[++i] = NUL;
  }
  return str;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetdata(NXhandle fid, void *data) {
  NXstatus status;
  NXnumtype type;
  int rank;
  int64_t iDim[NX_MAXRANK];

  NX5getinfo64(fid, &rank, iDim, &type); /* unstripped size if string */
  /* only strip one dimensional strings */
  if ((type == NXnumtype::CHAR) && (rank == 1)) {
    char *pPtr;
    pPtr = static_cast<char *>(malloc((size_t)iDim[0] + 5));
    memset(pPtr, 0, (size_t)iDim[0] + 5);
    status = NX5getdata(fid, pPtr);
    char const *pPtr2;
    pPtr2 = nxitrim(pPtr);
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
    strncpy(static_cast<char *>(data), pPtr2, strlen(pPtr2)); /* not NULL terminated by default */
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic pop
#endif
    free(pPtr);
  } else {
    status = NX5getdata(fid, data);
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetinfo64(NXhandle fid, int *rank, int64_t dimension[], NXnumtype *iType) {
  NXstatus status;
  char *pPtr = NULL;
  *rank = 0;
  status = NX5getinfo64(fid, rank, dimension, iType);
  /*
     the length of a string may be trimmed....
   */
  /* only strip one dimensional strings */
  if ((*iType == NXnumtype::CHAR) && (*rank == 1)) {
    pPtr = static_cast<char *>(malloc(static_cast<size_t>(dimension[0] + 1) * sizeof(char)));
    if (pPtr != NULL) {
      memset(pPtr, 0, static_cast<size_t>(dimension[0] + 1) * sizeof(char));
      NX5getdata(fid, pPtr);
      dimension[0] = static_cast<int64_t>(strlen(nxitrim(pPtr)));
      free(pPtr);
    }
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetslab64(NXhandle fid, void *data, const int64_t iStart[], const int64_t iSize[]) {
  return NX5getslab64(fid, data, iStart, iSize);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetattr(NXhandle fid, const char *name, void *data, int *datalen, NXnumtype *iType) {
  return NX5getattr(fid, name, data, datalen, iType);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetattrinfo(NXhandle fid, int *iN) { return NX5getattrinfo(fid, iN); }

NXstatus NXgetattrainfo(NXhandle handle, CONSTCHAR *name, int *rank, int dim[], NXnumtype *iType) {
  return NX5getattrainfo(handle, name, rank, dim, iType);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetgroupID(NXhandle fileid, NXlink *sRes) { return NX5getgroupID(fileid, sRes); }

/*-------------------------------------------------------------------------*/

NXstatus NXgetgroupinfo(NXhandle fid, int *iN, NXname pName, NXname pClass) {
  return NX5getgroupinfo(fid, iN, pName, pClass);
}

/*-------------------------------------------------------------------------*/

NXstatus NXsameID(NXhandle fileid, NXlink const *pFirstID, NXlink const *pSecondID) {
  return NX5sameID(fileid, pFirstID, pSecondID);
}

/*-------------------------------------------------------------------------*/

NXstatus NXinitattrdir(NXhandle fid) { return NX5initattrdir(fid); }

/*-------------------------------------------------------------------------*/

NXstatus NXinitgroupdir(NXhandle fid) { return NX5initgroupdir(fid); }

/*------------------------------------------------------------------------
  Implementation of NXopenaddress
  --------------------------------------------------------------------------*/
static int isDataSetOpen(NXhandle hfil) {
  NXlink id;

  /*
     This uses the (sensible) feauture that NXgetdataID returns NX_ERROR
     when no dataset is open
   */
  if (NXgetdataID(hfil, &id) == NXstatus::NX_ERROR) {
    return 0;
  } else {
    return 1;
  }
}

/*----------------------------------------------------------------------*/
static int isRoot(NXhandle hfil) {
  NXlink id;

  /*
     This uses the feauture that NXgetgroupID returns NX_ERROR
     when we are at root level
   */
  if (NXgetgroupID(hfil, &id) == NXstatus::NX_ERROR) {
    return 1;
  } else {
    return 0;
  }
}

/*--------------------------------------------------------------------
  copies the next address element into element.
  returns a pointer into address beyond the extracted address
  ---------------------------------------------------------------------*/
static char *extractNextAddress(char *address, NXname element) {
  char *pStart = address;
  /*
     skip over leading /
   */
  if (*pStart == '/') {
    pStart++;
  }

  /*
     find next /
   */
  char *pNext = strchr(pStart, '/');
  if (pNext == NULL) {
    /*
       this is the last address element
     */
    strcpy(element, pStart);
    return NULL;
  } else {
    size_t length = (size_t)(pNext - pStart);
    strncpy(element, pStart, length);
    element[length] = '\0';
  }
  return pNext++;
}

/*-------------------------------------------------------------------*/
static NXstatus gotoRoot(NXhandle hfil) {
  NXstatus status;

  if (isDataSetOpen(hfil)) {
    status = NXclosedata(hfil);
    if (status == NXstatus::NX_ERROR) {
      return status;
    }
  }
  while (!isRoot(hfil)) {
    status = NXclosegroup(hfil);
    if (status == NXstatus::NX_ERROR) {
      return status;
    }
  }
  return NXstatus::NX_OK;
}

/*--------------------------------------------------------------------*/
static int isRelative(char const *address) {
  if (address[0] == '.' && address[1] == '.')
    return 1;
  else
    return 0;
}

/*------------------------------------------------------------------*/
static NXstatus moveOneDown(NXhandle hfil) {
  if (isDataSetOpen(hfil)) {
    return NXclosedata(hfil);
  } else {
    return NXclosegroup(hfil);
  }
}

/*-------------------------------------------------------------------
  returns a pointer to the remaining address string to move up
  --------------------------------------------------------------------*/
static char *moveDown(NXhandle hfil, char *address, NXstatus *code) {
  *code = NXstatus::NX_OK;

  if (address[0] == '/') {
    *code = gotoRoot(hfil);
    return address;
  } else {
    char *pPtr;
    pPtr = address;
    while (isRelative(pPtr)) {
      NXstatus status = moveOneDown(hfil);
      if (status == NXstatus::NX_ERROR) {
        *code = status;
        return pPtr;
      }
      pPtr += 3;
    }
    return pPtr;
  }
}

/*--------------------------------------------------------------------*/
static NXstatus stepOneUp(NXhandle hfil, char const *name) {
  NXnumtype datatype;
  NXname name2, xclass;
  char pBueffel[256];

  /*
     catch the case when we are there: i.e. no further stepping
     necessary. This can happen with address like ../
   */
  if (strlen(name) < 1) {
    return NXstatus::NX_OK;
  }

  NXinitgroupdir(hfil);

  while (NXgetnextentry(hfil, name2, xclass, &datatype) != NXstatus::NX_EOD) {
    if (strcmp(name2, name) == 0) {
      if (strcmp(xclass, "SDS") == 0) {
        return NXopendata(hfil, name);
      } else {
        return NXopengroup(hfil, name, xclass);
      }
    }
  }
  snprintf(pBueffel, 255, "ERROR: NXopenaddress cannot step into %s", name);
  NXReportError(pBueffel);
  return NXstatus::NX_ERROR;
}

/*--------------------------------------------------------------------*/
static NXstatus stepOneGroupUp(NXhandle hfil, char const *name) {
  NXnumtype datatype;
  NXname name2, xclass;
  char pBueffel[256];

  /*
     catch the case when we are there: i.e. no further stepping
     necessary. This can happen with address like ../
   */
  if (strlen(name) < 1) {
    return NXstatus::NX_OK;
  }

  NXinitgroupdir(hfil);
  while (NXgetnextentry(hfil, name2, xclass, &datatype) != NXstatus::NX_EOD) {

    if (strcmp(name2, name) == 0) {
      if (strcmp(xclass, "SDS") == 0) {
        return NXstatus::NX_EOD;
      } else {
        return NXopengroup(hfil, name, xclass);
      }
    }
  }
  snprintf(pBueffel, 255, "ERROR: NXopengroupaddress cannot step into %s", name);
  NXReportError(pBueffel);
  return NXstatus::NX_ERROR;
}

/*---------------------------------------------------------------------*/
NXstatus NXopenaddress(NXhandle hfil, CONSTCHAR *address) {
  NXstatus status;
  int run = 1;
  NXname addressElement;
  char *pPtr;

  if (hfil == NULL || address == NULL) {
    NXReportError("ERROR: NXopendata needs both a file handle and a address string");
    return NXstatus::NX_ERROR;
  }

  pPtr = moveDown(hfil, const_cast<char *>(static_cast<const char *>(address)), &status);
  if (status != NXstatus::NX_OK) {
    NXReportError("ERROR: NXopendata failed to move down in hierarchy");
    return status;
  }

  while (run == 1) {
    pPtr = extractNextAddress(pPtr, addressElement);
    status = stepOneUp(hfil, addressElement);
    if (status != NXstatus::NX_OK) {
      return status;
    }
    if (pPtr == NULL) {
      run = 0;
    }
  }
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------*/
NXstatus NXopengroupaddress(NXhandle hfil, CONSTCHAR *address) {
  NXstatus status;
  NXname addressElement;
  char *pPtr;
  char buffer[256];

  if (hfil == NULL || address == NULL) {
    NXReportError("ERROR: NXopengroupaddress needs both a file handle and a address string");
    return NXstatus::NX_ERROR;
  }

  pPtr = moveDown(hfil, const_cast<char *>(static_cast<const char *>(address)), &status);
  if (status != NXstatus::NX_OK) {
    NXReportError("ERROR: NXopengroupaddress failed to move down in hierarchy");
    return status;
  }

  do {
    pPtr = extractNextAddress(pPtr, addressElement);
    status = stepOneGroupUp(hfil, addressElement);
    if (status == NXstatus::NX_ERROR) {
      sprintf(buffer, "ERROR: NXopengroupaddress cannot reach address %s", address);
      NXReportError(buffer);
      return NXstatus::NX_ERROR;
    }
  } while (pPtr != NULL && status != NXstatus::NX_EOD);
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------*/
NXstatus NXIprintlink(NXhandle fid, NXlink const *link) { return NX5printlink(fid, link); }

/*----------------------------------------------------------------------*/
NXstatus NXgetaddress(NXhandle fid, std::string &address) {
  hid_t current;
  if (fid->iCurrentD != 0) {
    current = fid->iCurrentD;
  } else if (fid->iCurrentG != 0) {
    current = fid->iCurrentG;
  } else {
    current = fid->iFID;
  }
  char caddr[NX_MAXADDRESSLEN];
  H5Iget_name(current, caddr, NX_MAXADDRESSLEN);
  address = std::string(caddr);
  return NXstatus::NX_OK;
}

NXstatus NXgetnextattra(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType) {
  return NX5getnextattra(handle, pName, rank, dim, iType);
}

/*--------------------------------------------------------------------
  format NeXus time. Code needed in every NeXus file driver
  ---------------------------------------------------------------------*/
char *NXIformatNeXusTime() {
  time_t timer;
  char *time_buffer = NULL;
  struct tm *time_info;
  const char *time_format;
  long gmt_offset;
#ifdef USE_FTIME
  struct timeb timeb_struct;
#endif

  time_buffer = static_cast<char *>(malloc(64 * sizeof(char)));
  if (!time_buffer) {
    NXReportError("Failed to allocate buffer for time data");
    return NULL;
  }
#ifdef NEED_TZSET
  tzset();
#endif
  time(&timer);
#ifdef USE_FTIME
  ftime(&timeb_struct);
  gmt_offset = -timeb_struct.timezone * 60;
  if (timeb_struct.dstflag != 0) {
    gmt_offset += 3600;
  }
#else
  time_info = gmtime(&timer);
  if (time_info != NULL) {
    gmt_offset = (long)difftime(timer, mktime(time_info));
  } else {
    NXReportError("Your gmtime() function does not work ... timezone information will be incorrect\n");
    gmt_offset = 0;
  }
#endif
  time_info = localtime(&timer);
  if (time_info != NULL) {
    if (gmt_offset < 0) {
      time_format = "%04d-%02d-%02dT%02d:%02d:%02d-%02ld:%02ld";
    } else {
      time_format = "%04d-%02d-%02dT%02d:%02d:%02d+%02ld:%02ld";
    }
    sprintf(time_buffer, time_format, 1900 + time_info->tm_year, 1 + time_info->tm_mon, time_info->tm_mday,
            time_info->tm_hour, time_info->tm_min, time_info->tm_sec, labs(gmt_offset / 3600),
            labs((gmt_offset % 3600) / 60));
  } else {
    strcpy(time_buffer, "1970-01-01T00:00:00+00:00");
  }
  return time_buffer;
}

const char *NXgetversion() { return NEXUS_VERSION; }

// cppcheck-suppress-end [constVariablePointer, unmatchedSuppression]
