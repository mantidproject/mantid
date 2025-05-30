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

#include "MantidNexus/napi.h"
#include "MantidNexus/napi_internal.h"
#include "MantidNexus/nxstack.h"

// this has to be after the other napi includes
#include "MantidNexus/napi5.h"

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
static bool canOpen(char const *filename) {
  FILE *fd = NULL;

  fd = fopen(filename, "r");
  if (fd != NULL) {
    fclose(fd);
    return true;
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------*/

void NXReportError(const char *string) { UNUSED_ARG(string); }

/* ----------------------------------------------------------------------

   Definition of NeXus API

   --------------------------------------------------------------------- */

static int determineFileType(CONSTCHAR *filename) {
  // this is for reading, check for existence first
  if (!canOpen(filename))
    return -1;

  // check that this is indeed hdf5
  if (H5Fis_hdf5(static_cast<const char *>(filename)) > 0) {
    return NXACC_CREATE5;
  } else {
    // file type not recognized
    return 0;
  }
}

/*---------------------------------------------------------------------*/
static pNexusFunction handleToNexusFunc(NXhandle fid) {
  if (pFileStack fileStack = static_cast<pFileStack>(fid)) {
    return peekFileOnStack(fileStack);
  } else {
    return NULL;
  }
}

/*--------------------------------------------------------------------*/
static NXstatus NXinternalopen(CONSTCHAR *userfilename, NXaccess am, pFileStack fileStack);
/*----------------------------------------------------------------------*/
NXstatus NXopen(CONSTCHAR *userfilename, NXaccess am, NXhandle &gHandle) {
  NXstatus status;
  pFileStack fileStack = NULL;

  gHandle = NULL;
  fileStack = makeFileStack();
  if (fileStack == NULL) {
    NXReportError("ERROR: no memory to create filestack");
    return NXstatus::NX_ERROR;
  }
  status = NXinternalopen(userfilename, am, fileStack);
  if (status == NXstatus::NX_OK) {
    gHandle = fileStack;
  }

  return status;
}

/*-----------------------------------------------------------------------*/
static NXstatus NXinternalopen(CONSTCHAR *userfilename, NXaccess am, pFileStack fileStack) {
  pNexusFunction fHandle = static_cast<pNexusFunction>(malloc(sizeof(NexusFunction)));
  if (fHandle == NULL) {
    NXReportError("ERROR: no memory to create Function structure");
    return NXstatus::NX_ERROR;
  }
  memset(fHandle, 0, sizeof(NexusFunction)); /* so any functions we miss are NULL */

  const int my_am = (am & NXACCMASK_REMOVEFLAGS); // remove high bits
  if (my_am != NXACC_CREATE5) {
    // check file type is hdf5 for reading
    const int iRet = determineFileType(userfilename);
    if (iRet != NXACC_CREATE5) {
      char error[1024];
      if (iRet < 0) {
        snprintf(error, 1023, "failed to open %s for reading", userfilename);
      } else if (iRet == 0) {
        snprintf(error, 1023, "failed to determine filetype for %s ", userfilename);
      }
      NXReportError(error);
      free(fHandle);
      return NXstatus::NX_ERROR;
    }
  }
  if (userfilename == NULL) {
    NXReportError("Out of memory in NeXus-API");
    free(fHandle);
    return NXstatus::NX_ERROR;
  }

  NXhandle hdf5_handle = NULL;
  NXstatus retstat = NX5open(userfilename, am, hdf5_handle);
  if (retstat != NXstatus::NX_OK) {
    free(fHandle);
  } else {
    fHandle->pNexusData = hdf5_handle;
    const int backend_type = NXACC_CREATE5; // to get past compiler warning
    fHandle->access_mode = backend_type || (NXACC_READ && am);
    NX5assignFunctions(fHandle);
    pushFileStack(fileStack, fHandle, userfilename);
  }
  return retstat;
}

NXstatus NXreopen(NXhandle pOrigHandle, NXhandle &newHandle) {
  pFileStack newFileStack;
  pFileStack origFileStack = static_cast<pFileStack>(pOrigHandle);
  pNexusFunction fOrigHandle = NULL, fNewHandle = NULL;
  newHandle = NULL;
  newFileStack = makeFileStack();
  if (newFileStack == NULL) {
    NXReportError("ERROR: no memory to create filestack");
    return NXstatus::NX_ERROR;
  }
  // The code below will only open the last file on a stack
  // for the moment raise an error, but this behaviour may be OK
  if (fileStackDepth(origFileStack) > 0) {
    NXReportError("ERROR: handle stack referes to many files - cannot reopen");
    return NXstatus::NX_ERROR;
  }
  fOrigHandle = peekFileOnStack(origFileStack);
  if (fOrigHandle->nxreopen == NULL) {
    NXReportError("ERROR: NXreopen not implemented for this underlying file format");
    return NXstatus::NX_ERROR;
  }
  fNewHandle = static_cast<NexusFunction *>(malloc(sizeof(NexusFunction)));
  memcpy(fNewHandle, fOrigHandle, sizeof(NexusFunction));
  fNewHandle->nxreopen(fOrigHandle->pNexusData, fNewHandle->pNexusData);
  pushFileStack(newFileStack, fNewHandle, peekFilenameOnStack(origFileStack));
  newHandle = newFileStack;
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------------- */

NXstatus NXclose(NXhandle &fid) {
  NXhandle hfil;
  NXstatus status;
  pFileStack fileStack = NULL;
  pNexusFunction pFunc = NULL;
  if (fid == NULL) {
    return NXstatus::NX_OK;
  }
  fileStack = static_cast<pFileStack>(fid);
  pFunc = peekFileOnStack(fileStack);
  hfil = pFunc->pNexusData;
  status = pFunc->nxclose(hfil);
  pFunc->pNexusData = hfil;
  free(pFunc);
  popFileStack(fileStack);
  if (fileStackDepth(fileStack) < 0) {
    killFileStack(fileStack);
    fid = NULL;
  }
  /* we can't set fid to NULL always as the handle points to a stack of files for external file support */
  /*
     Fortify_CheckAllMemory();
   */

  return status;
}

/*-----------------------------------------------------------------------*/

NXstatus NXmakegroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxmakegroup(pFunc->pNexusData, name, nxclass);
}

/*------------------------------------------------------------------------*/

NXstatus NXopengroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  NXstatus status;
  pFileStack fileStack;
  pNexusFunction pFunc = NULL;

  fileStack = static_cast<pFileStack>(fid);
  pFunc = handleToNexusFunc(fid);

  status = pFunc->nxopengroup(pFunc->pNexusData, name, nxclass);
  if (status == NXstatus::NX_OK) {
    pushAddress(fileStack, name);
  }

  return status;
}

/* ------------------------------------------------------------------- */

NXstatus NXclosegroup(NXhandle fid) {
  NXstatus status;
  pFileStack fileStack = NULL;
  NXlink closeID, currentID;

  pNexusFunction pFunc = handleToNexusFunc(fid);
  fileStack = static_cast<pFileStack>(fid);
  if (fileStackDepth(fileStack) == 0) {
    status = pFunc->nxclosegroup(pFunc->pNexusData);
    if (status == NXstatus::NX_OK) {
      popAddress(fileStack);
    }
    return status;
  } else {
    /* we have to check for leaving an external file */
    NXgetgroupID(fid, &currentID);
    peekIDOnStack(fileStack, &closeID);
    if (NXsameID(fid, &closeID, &currentID) == NXstatus::NX_OK) {
      NXclose(fid);
      status = NXclosegroup(fid);
    } else {
      status = pFunc->nxclosegroup(pFunc->pNexusData);
      if (status == NXstatus::NX_OK) {
        popAddress(fileStack);
      }
    }
    return status;
  }
}

/* --------------------------------------------------------------------- */

NXstatus NXmakedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[]) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxmakedata64(pFunc->pNexusData, name, datatype, rank, dimensions);
}

/* --------------------------------------------------------------------- */

NXstatus NXcompmakedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[],
                          int compress_type, int64_t const chunk_size[]) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxcompmakedata64(pFunc->pNexusData, name, datatype, rank, dimensions, compress_type, chunk_size);
}

/* --------------------------------------------------------------------- */

NXstatus NXopendata(NXhandle fid, CONSTCHAR *name) {
  NXstatus status;
  pFileStack fileStack;
  pNexusFunction pFunc = NULL;

  fileStack = static_cast<pFileStack>(fid);
  pFunc = handleToNexusFunc(fid);
  status = pFunc->nxopendata(pFunc->pNexusData, name);

  if (status == NXstatus::NX_OK) {
    pushAddress(fileStack, name);
  }

  return status;
}

/* ----------------------------------------------------------------- */

NXstatus NXclosedata(NXhandle fid) {
  NXstatus status;
  pFileStack fileStack = NULL;
  NXlink closeID, currentID;

  pNexusFunction pFunc = handleToNexusFunc(fid);
  fileStack = static_cast<pFileStack>(fid);

  if (fileStackDepth(fileStack) == 0) {
    status = pFunc->nxclosedata(pFunc->pNexusData);
    if (status == NXstatus::NX_OK) {
      popAddress(fileStack);
    }
    return status;
  } else {
    /* we have to check for leaving an external file */
    NXgetdataID(fid, &currentID);
    peekIDOnStack(fileStack, &closeID);
    if (NXsameID(fid, &closeID, &currentID) == NXstatus::NX_OK) {
      NXclose(fid);
      status = NXclosedata(fid);
    } else {
      status = pFunc->nxclosedata(pFunc->pNexusData);
      if (status == NXstatus::NX_OK) {
        popAddress(fileStack);
      }
    }
    return status;
  }
}

/* ------------------------------------------------------------------- */

NXstatus NXputdata(NXhandle fid, const void *data) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxputdata(pFunc->pNexusData, data);
}

/* ------------------------------------------------------------------- */

NXstatus NXputattr(NXhandle fid, CONSTCHAR *name, const void *data, int datalen, NXnumtype iType) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  if (datalen > 1 && iType != NXnumtype::CHAR) {
    NXReportError(
        "NXputattr: numeric arrays are not allowed as attributes - only character strings and single numbers");
    return NXstatus::NX_ERROR;
  }
  return pFunc->nxputattr(pFunc->pNexusData, name, data, datalen, iType);
}

/* ------------------------------------------------------------------- */

NXstatus NXputslab(NXhandle fid, const void *data, const int iStart[], const int iSize[]) {
  int i, rank;
  NXnumtype iType;
  int64_t iStart64[NX_MAXRANK], iSize64[NX_MAXRANK];
  if (NXgetinfo64(fid, &rank, iStart64, &iType) != NXstatus::NX_OK) {
    return NXstatus::NX_ERROR;
  }
  for (i = 0; i < rank; ++i) {
    iStart64[i] = iStart[i];
    iSize64[i] = iSize[i];
  }
  return NXputslab64(fid, data, iStart64, iSize64);
}

NXstatus NXputslab64(NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[]) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxputslab64(pFunc->pNexusData, data, iStart, iSize);
}

/* ------------------------------------------------------------------- */

NXstatus NXgetdataID(NXhandle fid, NXlink *sRes) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxgetdataID(pFunc->pNexusData, sRes);
}

/* ------------------------------------------------------------------- */

NXstatus NXmakelink(NXhandle fid, NXlink *sLink) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxmakelink(pFunc->pNexusData, sLink);
}

/* ------------------------------------------------------------------- */

NXstatus NXmakenamedlink(NXhandle fid, CONSTCHAR *newname, NXlink *sLink) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxmakenamedlink(pFunc->pNexusData, newname, sLink);
}

/* -------------------------------------------------------------------- */
NXstatus NXopensourcegroup(NXhandle fid) {
  char target_address[512];
  NXnumtype type = NXnumtype::CHAR;
  int length = 511;
  NXstatus status;

  status = NXgetattr(fid, "target", target_address, &length, &type);
  if (status != NXstatus::NX_OK) {
    NXReportError("ERROR: item not linked");
    return NXstatus::NX_ERROR;
  }
  return NXopengroupaddress(fid, target_address);
}

/*----------------------------------------------------------------------*/

NXstatus NXflush(NXhandle &handle) {
  NXhandle hfil;
  pFileStack fileStack = NULL;
  NXstatus status;

  pNexusFunction pFunc = NULL;
  fileStack = static_cast<pFileStack>(handle);
  pFunc = peekFileOnStack(fileStack);
  hfil = pFunc->pNexusData;
  status = pFunc->nxflush(hfil);
  pFunc->pNexusData = hfil;
  return status;
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
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxgetnextentry(pFunc->pNexusData, name, nxclass, datatype);
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

  pNexusFunction pFunc = handleToNexusFunc(fid);
  pFunc->nxgetinfo64(pFunc->pNexusData, &rank, iDim, &type); /* unstripped size if string */
  /* only strip one dimensional strings */
  if ((type == NXnumtype::CHAR) && (rank == 1)) {
    char *pPtr;
    pPtr = static_cast<char *>(malloc((size_t)iDim[0] + 5));
    memset(pPtr, 0, (size_t)iDim[0] + 5);
    status = pFunc->nxgetdata(pFunc->pNexusData, pPtr);
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
    status = pFunc->nxgetdata(pFunc->pNexusData, data);
  }
  return status;
}

/*---------------------------------------------------------------------------*/
NXstatus NXgetrawinfo64(NXhandle fid, int *rank, int64_t dimension[], NXnumtype *iType) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxgetinfo64(pFunc->pNexusData, rank, dimension, iType);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetinfo64(NXhandle fid, int *rank, int64_t dimension[], NXnumtype *iType) {
  NXstatus status;
  char *pPtr = NULL;
  pNexusFunction pFunc = handleToNexusFunc(fid);
  *rank = 0;
  status = pFunc->nxgetinfo64(pFunc->pNexusData, rank, dimension, iType);
  /*
     the length of a string may be trimmed....
   */
  /* only strip one dimensional strings */
  if ((*iType == NXnumtype::CHAR) && (*rank == 1)) {
    pPtr = static_cast<char *>(malloc(static_cast<size_t>(dimension[0] + 1) * sizeof(char)));
    if (pPtr != NULL) {
      memset(pPtr, 0, static_cast<size_t>(dimension[0] + 1) * sizeof(char));
      pFunc->nxgetdata(pFunc->pNexusData, pPtr);
      dimension[0] = static_cast<int64_t>(strlen(nxitrim(pPtr)));
      free(pPtr);
    }
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetslab64(NXhandle fid, void *data, const int64_t iStart[], const int64_t iSize[]) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxgetslab64(pFunc->pNexusData, data, iStart, iSize);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetattr(NXhandle fid, const char *name, void *data, int *datalen, NXnumtype *iType) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxgetattr(pFunc->pNexusData, name, data, datalen, iType);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetattrinfo(NXhandle fid, int *iN) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxgetattrinfo(pFunc->pNexusData, iN);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetgroupID(NXhandle fileid, NXlink *sRes) {
  pNexusFunction pFunc = handleToNexusFunc(fileid);
  return pFunc->nxgetgroupID(pFunc->pNexusData, sRes);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetgroupinfo(NXhandle fid, int *iN, NXname pName, NXname pClass) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxgetgroupinfo(pFunc->pNexusData, iN, pName, pClass);
}

/*-------------------------------------------------------------------------*/

NXstatus NXsameID(NXhandle fileid, NXlink const *pFirstID, NXlink const *pSecondID) {
  pNexusFunction pFunc = handleToNexusFunc(fileid);
  return pFunc->nxsameID(pFunc->pNexusData, pFirstID, pSecondID);
}

/*-------------------------------------------------------------------------*/

NXstatus NXinitattrdir(NXhandle fid) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxinitattrdir(pFunc->pNexusData);
}

/*-------------------------------------------------------------------------*/

NXstatus NXinitgroupdir(NXhandle fid) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxinitgroupdir(pFunc->pNexusData);
}

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
NXstatus NXIprintlink(NXhandle fid, NXlink const *link) {
  pNexusFunction pFunc = handleToNexusFunc(fid);
  return pFunc->nxprintlink(pFunc->pNexusData, link);
}

/*----------------------------------------------------------------------*/
NXstatus NXgetaddress(NXhandle fid, char *address, int addresslen) {
  int status;
  pFileStack fileStack = NULL;

  fileStack = static_cast<pFileStack>(fid);
  status = buildAddress(fileStack, address, addresslen);
  if (status != 1) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

NXstatus NXgetnextattra(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType) {
  pNexusFunction pFunc = handleToNexusFunc(handle);
  return pFunc->nxgetnextattra(pFunc->pNexusData, pName, rank, dim, iType);
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
