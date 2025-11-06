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

#include "MantidLegacyNexus/NeXusFileID.h"
#include "MantidLegacyNexus/napi.h"
#include "MantidLegacyNexus/napi_internal.h"
#include "MantidLegacyNexus/napiconfig.h"

using namespace Mantid::LegacyNexus;

/*---------------------------------------------------------------------
 Recognized and handled napimount URLS
 -----------------------------------------------------------------------*/
#define NXBADURL 0
#define NXFILE 1

/*--------------------------------------------------------------------*/
/* static int iFortifyScope; */
/*----------------------------------------------------------------------
  This is a section with code for searching the NX_LOAD_PATH
  -----------------------------------------------------------------------*/
#ifdef _WIN32
#define LIBSEP ";"
#define PATHSEP "\\"
#define THREAD_LOCAL __declspec(thread)
#else
#define LIBSEP ":"
#define PATHSEP "/"
#define THREAD_LOCAL __thread
#endif

#ifdef WIN32
#define snprintf _snprintf
#define strdup _strdup
#endif

#include "MantidLegacyNexus/nx_stptok.h"

#if HAVE_LIBPTHREAD

#include <pthread.h>

static pthread_mutex_t nx_mutex;

#if defined(PTHREAD_MUTEX_RECURSIVE) || defined(__FreeBSD__)
#define RECURSIVE_LOCK PTHREAD_MUTEX_RECURSIVE
#else
#define RECURSIVE_LOCK PTHREAD_MUTEX_RECURSIVE_NP
#endif /* PTHREAD_MUTEX_RECURSIVE */

static void nx_pthread_init() {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, RECURSIVE_LOCK);
  pthread_mutex_init(&nx_mutex, &attr);
}

static NXstatus nxilock() {
  static pthread_once_t once_control = PTHREAD_ONCE_INIT;
  if (pthread_once(&once_control, nx_pthread_init) != 0) {
    NXReportError("pthread_once failed");
    return NXstatus::NX_ERROR;
  }
  if (pthread_mutex_lock(&nx_mutex) != 0) {
    NXReportError("pthread_mutex_lock failed");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

static NXstatus nxiunlock(int ret) {
  if (pthread_mutex_unlock(&nx_mutex) != 0) {
    NXReportError("pthread_mutex_unlock failed");
    return NXstatus::NX_ERROR;
  }
  return ret;
}

#define LOCKED_CALL(__call) (nxilock(), nxiunlock(__call))

#elif defined(WIN32)
/*
 *  HDF5 on windows does not do locking for multiple threads conveniently so we will implement it ourselves.
 *  Freddie Akeroyd, 16/06/2011
 */
#include <windows.h>

static CRITICAL_SECTION nx_critical;

static NXstatus nxilock() {
  static int first_call = 1;
  if (first_call) {
    first_call = 0;
    InitializeCriticalSection(&nx_critical);
  }
  EnterCriticalSection(&nx_critical);
  return NXstatus::NX_OK;
}

static NXstatus nxiunlock(NXstatus ret) {
  LeaveCriticalSection(&nx_critical);
  return ret;
}

#define LOCKED_CALL(__call) (nxilock(), nxiunlock(__call))

#else

#define LOCKED_CALL(__call) __call

#endif /* _WIN32 */

/*---------------------------------------------------------------------
 wrapper for getenv. This is a future proofing thing for porting to OS
 which have different ways of accessing environment variables
 --------------------------------------------------------------------*/
static char *nxgetenv(const char *name) { return getenv(name); }

/*----------------------------------------------------------------------*/
static int canOpen(char const *filename) {
  FILE *fd = NULL;

  fd = fopen(filename, "r");
  if (fd != NULL) {
    fclose(fd);
    return 1;
  } else {
    return 0;
  }
}

/*--------------------------------------------------------------------*/
static char *locateNexusFileInPath(char const *const startName) {
  char const *loadPath = NULL, *pPtr = NULL;
  char *testPath = NULL;
  char pathPrefix[256];

  if (canOpen(startName)) {
    return strdup(startName);
  }

  loadPath = nxgetenv("NX_LOAD_PATH");
  if (loadPath == NULL) {
    // file not found will be issued by upper level code
    return strdup(startName);
  }

  pPtr = stptok(loadPath, pathPrefix, 255, LIBSEP);
  while (pPtr != NULL) {
    auto length = strlen(pathPrefix) + strlen(startName) + strlen(PATHSEP) + 2;
    testPath = static_cast<char *>(malloc(length * sizeof(char)));
    if (testPath == NULL) {
      return strdup(startName);
    }
    memset(testPath, 0, length * sizeof(char));
    strcpy(testPath, pathPrefix);
    strcat(testPath, PATHSEP);
    strcat(testPath, startName);
    if (canOpen(testPath)) {
      return (testPath);
    }
    free(testPath);
    pPtr = stptok(pPtr, pathPrefix, 255, LIBSEP);
  }
  return strdup(startName);
}

/*------------------------------------------------------------------------
HDF-5 cache size special stuff
-------------------------------------------------------------------------*/
long nx_cacheSize = 1024000; /* 1MB, HDF-5 default */

/*-------------------------------------------------------------------------*/
static void NXNXNoReport(void *pData, const char *string) {
  // do nothing but declare the variables unused
  UNUSED_ARG(pData);
  UNUSED_ARG(string);
}

/*---------------------------------------------------------------------*/

static void *NXEHpData = NULL;
// default to no logging
static ErrFunc NXEHIReportError = NXNXNoReport;
#ifdef HAVE_TLS
static THREAD_LOCAL void *NXEHpTData = NULL;
static THREAD_LOCAL ErrFunc NXEHIReportTError = NULL;
#endif

void NXIReportError(void *pData, const char *string) {
  UNUSED_ARG(pData);
  fprintf(
      stderr,
      "Your application uses NXIReportError, but its first parameter is ignored now - you should use NXReportError.");
  NXReportError(string);
}

void NXReportError(const char *string) {
#ifdef HAVE_TLS
  if (NXEHIReportTError) {
    (*NXEHIReportTError)(NXEHpTData, string);
    return;
  }
#endif
  if (NXEHIReportError) {
    (*NXEHIReportError)(NXEHpData, string);
  }
}

/*---------------------------------------------------------------------*/
extern void NXMSetError(void *pData, ErrFunc NewError) {
  NXEHpData = pData;
  NXEHIReportError = NewError;
}

/*----------------------------------------------------------------------*/
extern void NXMSetTError(void *pData, ErrFunc NewError) {
#ifdef HAVE_TLS
  NXEHpTData = pData;
  NXEHIReportTError = NewError;
#else
  NXMSetError(pData, NewError);
#endif
}

/*----------------------------------------------------------------------*/

static ErrFunc last_global_errfunc = NXEHIReportError;
#ifdef HAVE_TLS
static THREAD_LOCAL ErrFunc last_thread_errfunc = NULL;
#endif

extern void NXMDisableErrorReporting() {
#ifdef HAVE_TLS
  if (NXEHIReportTError) {
    last_thread_errfunc = NXEHIReportTError;
    NXEHIReportTError = NXNXNoReport;
    return;
  }
#endif
  if (NXEHIReportError) {
    last_global_errfunc = NXEHIReportError;
    NXEHIReportError = NXNXNoReport;
  }
}

extern void NXMEnableErrorReporting() {
#ifdef HAVE_TLS
  if (last_thread_errfunc) {
    NXEHIReportTError = last_thread_errfunc;
    last_thread_errfunc = NULL;
    return;
  }
#endif
  if (last_global_errfunc) {
    NXEHIReportError = last_global_errfunc;
    last_global_errfunc = NULL;
  }
}

/*----------------------------------------------------------------------*/
#ifdef WITH_HDF5
#include "MantidLegacyNexus/napi5.h"
#endif
#ifdef WITH_HDF4
#include "MantidLegacyNexus/napi4.h"
#endif
/* ----------------------------------------------------------------------

   Definition of NeXus API

   --------------------------------------------------------------------- */
static int determineFileTypeImpl(CONSTCHAR *filename) {
  FILE *fd = NULL;
  int iRet;

  /*
     this is for reading, check for existence first
   */
  fd = fopen(filename, "r");
  if (fd == NULL) {
    return -1;
  }
  fclose(fd);
#ifdef WITH_HDF5
  iRet = H5Fis_hdf5(static_cast<const char *>(filename));
  if (iRet > 0) {
    return NXACC_CREATE5;
  }
#endif
#ifdef WITH_HDF4
  iRet = Hishdf(static_cast<const char *>(filename));
  if (iRet > 0) {
    return NXACC_CREATE4;
  }
#endif
  /*
     file type not recognized
   */
  return 0;
}

static int determineFileType(CONSTCHAR *filename) { return determineFileTypeImpl(filename); }

/*---------------------------------------------------------------------*/
static const LgcyFunction &handleToNexusFunc(NXhandle fid) {
  const NexusFileID *fileID = static_cast<NexusFileID *>(fid);
  return fileID->getNexusFunctions();
}

/*--------------------------------------------------------------------*/
static NXstatus NXinternalopen(NXaccess am, NexusFileID *fileRecord);
/*----------------------------------------------------------------------*/
NXstatus NXopen(CONSTCHAR *userfilename, NXaccess am, NXhandle *gHandle) {
  NXstatus status;

  *gHandle = NULL;
  auto fileRecord = new NexusFileID(userfilename);
  status = NXinternalopen(am, fileRecord);
  if (status == NXstatus::NX_OK) {
    *gHandle = fileRecord;
  }

  return status;
}

/*-----------------------------------------------------------------------*/
static NXstatus NXinternalopenImpl(NXaccess am, NexusFileID *fileRecord) {
  int backend_type, iRet;
  char error[1024];
  char *filename = NULL;
  int my_am = (am & NXACCMASK_REMOVEFLAGS);

  /* configure fortify
     iFortifyScope = Fortify_EnterScope();
     Fortify_CheckAllMemory();
   */

  auto fHandle = std::make_unique<LgcyFunction>();
  /*
     test the strip flag. Elimnate it for the rest of the tests to work
   */
  fHandle->stripFlag = 1;
  if (am & NXACC_NOSTRIP) {
    fHandle->stripFlag = 0;
    am = (NXaccess)(am & ~NXACC_NOSTRIP);
  }
  fHandle->checkNameSyntax = 0;
  if (am & NXACC_CHECKNAMESYNTAX) {
    fHandle->checkNameSyntax = 1;
    am = (NXaccess)(am & ~NXACC_CHECKNAMESYNTAX);
  }

  switch (my_am) {
  case NXACC_CREATE4:
  case NXACC_CREATE:
  case NXACC_CREATE5:
  case NXACC_CREATEXML:
  case NXACC_RDWR:
    NXReportError("Write operations have been deprecated from LegacyNexus");
    return NXstatus::NX_ERROR;
  default:
    filename = locateNexusFileInPath(fileRecord->getUserFilePath().c_str());
    if (filename == NULL) {
      NXReportError("Out of memory in NeXus-API");
      return NXstatus::NX_ERROR;
    }
    /* check file type hdf4/hdf5/XML for reading */
    iRet = determineFileType(filename);
    if (iRet < 0) {
      snprintf(error, 1023, "failed to open %s for reading", filename);
      NXReportError(error);
      free(filename);
      return NXstatus::NX_ERROR;
    }
    if (iRet == 0) {
      snprintf(error, 1023, "failed to determine filetype for %s ", filename);
      NXReportError(error);
      free(filename);
      return NXstatus::NX_ERROR;
    }
    backend_type = iRet;
    break;
  }
  if (filename == NULL) {
    NXReportError("Out of memory in NeXus-API");
    return NXstatus::NX_ERROR;
  }

  NXstatus retstat = NXstatus::NX_ERROR;
  if (backend_type == NXACC_CREATE4) {
    /* HDF4 type */
#ifdef WITH_HDF4
    NXhandle hdf4_handle = NULL;
    retstat = NX4open(static_cast<const char *>(filename), am, &hdf4_handle);
    if (retstat != NXstatus::NX_OK) {
      free(filename);
      return retstat;
    }
    fHandle->pNexusData = hdf4_handle;
    fHandle->access_mode = backend_type || (NXACC_READ && am);
    NX4assignFunctions(*fHandle);
    fileRecord->setFilePath(filename);
    fileRecord->setNexusFunctions(std::move(fHandle));
#else
    NXReportError("ERROR: Attempt to create HDF4 file when not linked with HDF4");
    retstat = NXstatus::NX_ERROR;
#endif /* HDF4 */
    free(filename);
    return retstat;
  } else if (backend_type == NXACC_CREATE5) {
    /* HDF5 type */
#ifdef WITH_HDF5
    NXhandle hdf5_handle = NULL;
    retstat = NX5open(filename, am, &hdf5_handle);
    if (retstat != NXstatus::NX_OK) {
      free(filename);
      return retstat;
    }
    fHandle->pNexusData = hdf5_handle;
    fHandle->access_mode = backend_type || (NXACC_READ && am);
    NX5assignFunctions(*fHandle);
    fileRecord->setFilePath(filename);
    fileRecord->setNexusFunctions(std::move(fHandle));
#else
    NXReportError("ERROR: Attempt to create HDF5 file when not linked with HDF5");
    retstat = NXstatus::NX_ERROR;
#endif /* HDF5 */
    free(filename);
    return retstat;
  } else if (backend_type == NXACC_CREATEXML) {
    /*
       XML type
     */
    NXReportError("ERROR: Attempt to create XML file when not linked with XML");
    retstat = NXstatus::NX_ERROR;
  } else {
    NXReportError("ERROR: Format not readable by this NeXus library");
    retstat = NXstatus::NX_ERROR;
  }
  if (filename != NULL) {
    free(filename);
  }
  return retstat;
}

static NXstatus NXinternalopen(NXaccess am, NexusFileID *fileRecord) {
  return LOCKED_CALL(NXinternalopenImpl(am, fileRecord));
}

/* ------------------------------------------------------------------------- */

NXstatus NXclose(NXhandle *fid) {
  NXhandle hfil;
  NXstatus status;
  if (*fid == NULL) {
    return NXstatus::NX_OK;
  }
  NexusFileID *fileID = static_cast<NexusFileID *>(*fid);
  auto &nexusFuncs = fileID->getNexusFunctions();
  hfil = nexusFuncs.pNexusData;
  status = LOCKED_CALL(nexusFuncs.nxclose(&hfil));
  nexusFuncs.pNexusData = hfil;
  *fid = NULL;
  delete fileID;

  return status;
}

/*------------------------------------------------------------------------*/

NXstatus NXopengroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  NXnumtype type = NXnumtype::CHAR;
  int length = 1023;
  NXstatus status, attStatus;

  NexusFileID *fileID = static_cast<NexusFileID *>(fid);
  auto &nexusFuncs = fileID->getNexusFunctions();
  status = LOCKED_CALL(nexusFuncs.nxopengroup(nexusFuncs.pNexusData, name, nxclass));
  if (status == NXstatus::NX_OK) {
    fileID->pushNexusPath(name);
  }
  NXMDisableErrorReporting();
  char nxurl[1024];
  attStatus = NXgetattr(fid, "napimount", nxurl, &length, &type);
  NXMEnableErrorReporting();
  if (attStatus == NXstatus::NX_OK) {
    char pError[256];
    snprintf(pError, 255, "ERROR: Support for Externally Linking files has been removed from LegacyNexus: %s", name);
    NXReportError(pError);
  }
  return status;
}

/* ------------------------------------------------------------------- */

NXstatus NXclosegroup(NXhandle fid) {
  NXstatus status;

  NexusFileID *fileID = static_cast<NexusFileID *>(fid);
  auto &nexusFuncs = fileID->getNexusFunctions();
  status = LOCKED_CALL(nexusFuncs.nxclosegroup(nexusFuncs.pNexusData));
  if (status == NXstatus::NX_OK) {
    fileID->popNexusPath();
  }
  return status;
}

NXstatus NXopendata(NXhandle fid, CONSTCHAR *name) {
  NXstatus status;

  NexusFileID *fileID = static_cast<NexusFileID *>(fid);
  auto &nexusFuncs = fileID->getNexusFunctions();
  status = LOCKED_CALL(nexusFuncs.nxopendata(nexusFuncs.pNexusData, name));

  if (status == NXstatus::NX_OK) {
    fileID->pushNexusPath(name);
  }
  return status;
}

/* ----------------------------------------------------------------- */

NXstatus NXclosedata(NXhandle fid) {
  NXstatus status;

  NexusFileID *fileID = static_cast<NexusFileID *>(fid);
  auto &nexusFuncs = fileID->getNexusFunctions();
  status = LOCKED_CALL(nexusFuncs.nxclosedata(nexusFuncs.pNexusData));
  if (status == NXstatus::NX_OK) {
    fileID->popNexusPath();
  }
  return status;
}

/* ------------------------------------------------------------------- */

NXstatus NXgetdataID(NXhandle fid, NXlink *sRes) {
  const auto &nexusFuncs = handleToNexusFunc(fid);
  return LOCKED_CALL(nexusFuncs.nxgetdataID(nexusFuncs.pNexusData, sRes));
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetnextentry(NXhandle fid, NXname name, NXname nxclass, NXnumtype *datatype) {
  const auto &nexusFuncs = handleToNexusFunc(fid);
  return LOCKED_CALL(nexusFuncs.nxgetnextentry(nexusFuncs.pNexusData, name, nxclass, datatype));
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

  const auto &nexusFuncs = handleToNexusFunc(fid);
  LOCKED_CALL(nexusFuncs.nxgetinfo64(nexusFuncs.pNexusData, &rank, iDim, &type)); /* unstripped size if string */
  /* only strip one dimensional strings */
  if ((type == NXnumtype::CHAR) && (nexusFuncs.stripFlag == 1) && (rank == 1)) {
    char *pPtr;
    pPtr = static_cast<char *>(malloc((size_t)iDim[0] + 5));
    memset(pPtr, 0, (size_t)iDim[0] + 5);
    status = LOCKED_CALL(nexusFuncs.nxgetdata(nexusFuncs.pNexusData, pPtr));
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
    status = LOCKED_CALL(nexusFuncs.nxgetdata(nexusFuncs.pNexusData, data));
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetinfo(NXhandle fid, int *rank, int dimension[], NXnumtype *iType) {
  int i;
  NXstatus status;
  int64_t dims64[NX_MAXRANK];
  status = NXgetinfo64(fid, rank, dims64, iType);
  for (i = 0; i < *rank; ++i) {
    dimension[i] = (int)dims64[i];
  }
  return status;
}

NXstatus NXgetinfo64(NXhandle fid, int *rank, int64_t dimension[], NXnumtype *iType) {
  NXstatus status;
  char *pPtr = NULL;
  const auto &nexusFuncs = handleToNexusFunc(fid);
  *rank = 0;
  status = LOCKED_CALL(nexusFuncs.nxgetinfo64(nexusFuncs.pNexusData, rank, dimension, iType));
  /*
     the length of a string may be trimmed....
   */
  /* only strip one dimensional strings */
  if ((*iType == NXnumtype::CHAR) && (nexusFuncs.stripFlag == 1) && (*rank == 1)) {
    pPtr = static_cast<char *>(malloc(static_cast<size_t>(dimension[0] + 1) * sizeof(char)));
    if (pPtr != NULL) {
      memset(pPtr, 0, static_cast<size_t>(dimension[0] + 1) * sizeof(char));
      LOCKED_CALL(nexusFuncs.nxgetdata(nexusFuncs.pNexusData, pPtr));
      dimension[0] = static_cast<int64_t>(strlen(nxitrim(pPtr)));
      free(pPtr);
    }
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetattr(NXhandle fid, const char *name, void *data, int *datalen, NXnumtype *iType) {
  const auto &nexusFuncs = handleToNexusFunc(fid);
  return LOCKED_CALL(nexusFuncs.nxgetattr(nexusFuncs.pNexusData, name, data, datalen, iType));
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetgroupID(NXhandle fid, NXlink *sRes) {
  const auto &nexusFuncs = handleToNexusFunc(fid);
  return LOCKED_CALL(nexusFuncs.nxgetgroupID(nexusFuncs.pNexusData, sRes));
}

/*-------------------------------------------------------------------------*/

NXstatus NXinitattrdir(NXhandle fid) {
  const auto &nexusFuncs = handleToNexusFunc(fid);
  return LOCKED_CALL(nexusFuncs.nxinitattrdir(nexusFuncs.pNexusData));
}

/*-------------------------------------------------------------------------*/

NXstatus NXinitgroupdir(NXhandle fid) {
  const auto &nexusFuncs = handleToNexusFunc(fid);
  return LOCKED_CALL(nexusFuncs.nxinitgroupdir(nexusFuncs.pNexusData));
}

/*------------------------------------------------------------------------
  Implementation of NXopenpath
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
  copies the next path element into element.
  returns a pointer into path beyond the extracted path
  ---------------------------------------------------------------------*/
static char *extractNextAddress(char *path, NXname element) {
  char *pStart = path; // cppcheck-suppress constVariablePointer
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
       this is the last path element
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
static int isRelative(char const *path) {
  if (path[0] == '.' && path[1] == '.')
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
  returns a pointer to the remaining path string to move up
  --------------------------------------------------------------------*/
static char *moveDown(NXhandle hfil, char *path, NXstatus *code) {
  *code = NXstatus::NX_OK;

  if (path[0] == '/') {
    *code = gotoRoot(hfil);
    return path;
  } else {
    char *pPtr;
    pPtr = path;
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
     necessary. This can happen with paths like ../
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
  snprintf(pBueffel, 255, "ERROR: NXopenpath cannot step into %s", name);
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
     necessary. This can happen with paths like ../
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
  snprintf(pBueffel, 255, "ERROR: NXopengrouppath cannot step into %s", name);
  NXReportError(pBueffel);
  return NXstatus::NX_ERROR;
}

/*---------------------------------------------------------------------*/
NXstatus NXopenpath(NXhandle hfil, CONSTCHAR *path) {
  NXstatus status;
  int run = 1;
  NXname pathElement;
  char *pPtr;

  if (hfil == NULL || path == NULL) {
    NXReportError("ERROR: NXopendata needs both a file handle and a path string");
    return NXstatus::NX_ERROR;
  }

  pPtr = moveDown(hfil, const_cast<char *>(static_cast<const char *>(path)), &status);
  if (status != NXstatus::NX_OK) {
    NXReportError("ERROR: NXopendata failed to move down in hierarchy");
    return status;
  }

  while (run == 1) {
    pPtr = extractNextAddress(pPtr, pathElement);
    status = stepOneUp(hfil, pathElement);
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
NXstatus NXopengrouppath(NXhandle hfil, CONSTCHAR *path) {
  NXstatus status;
  NXname pathElement;
  char *pPtr;
  char buffer[256];

  if (hfil == NULL || path == NULL) {
    NXReportError("ERROR: NXopengrouppath needs both a file handle and a path string");
    return NXstatus::NX_ERROR;
  }

  pPtr = moveDown(hfil, const_cast<char *>(static_cast<const char *>(path)), &status);
  if (status != NXstatus::NX_OK) {
    NXReportError("ERROR: NXopengrouppath failed to move down in hierarchy");
    return status;
  }

  do {
    pPtr = extractNextAddress(pPtr, pathElement);
    status = stepOneGroupUp(hfil, pathElement);
    if (status == NXstatus::NX_ERROR) {
      sprintf(buffer, "ERROR: NXopengrouppath cannot reach path %s", path);
      NXReportError(buffer);
      return NXstatus::NX_ERROR;
    }
  } while (pPtr != NULL && status != NXstatus::NX_EOD);
  return NXstatus::NX_OK;
}

/*----------------------------------------------------------------------*/
std::string NXgetpath(NXhandle fid) {
  const NexusFileID *fileID = static_cast<NexusFileID *>(fid);
  return fileID->getFullNexusPath();
}

NXstatus NXgetnextattra(NXhandle fid, NXname pName, int *rank, int dim[], NXnumtype *iType) {
  const auto &nexusFuncs = handleToNexusFunc(fid);
  return LOCKED_CALL(nexusFuncs.nxgetnextattra(nexusFuncs.pNexusData, pName, rank, dim, iType));
}

/*--------------------------------------------------------------------*/

const char *NXgetversion() { return NEXUS_VERSION; }
