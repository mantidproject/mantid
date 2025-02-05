/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format

  Application Program Interface (HDF5) Routines

  Copyright (C) 1997-2014 NIAC

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

#include <string>
#define H5Aiterate_vers 2

#include "MantidNexusCpp/napiconfig.h"

#ifdef WITH_HDF5

#include <assert.h>
#include <cstring>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// clang-format off
#include "MantidNexusCpp/napiconfig.h"
#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napi_internal.h"
#include "MantidNexusCpp/napi5.h"
// clang-format on

#ifdef H5_VERSION_GE
#if !H5_VERSION_GE(1, 8, 0)
#error HDF5 Version must be 1.8.0 or higher
#endif
#endif

#ifdef WIN32
#define snprintf _snprintf
#define strdup _strdup
#endif

#define NX_UNKNOWN_GROUP "" /* for when no NX_class attr */

extern void *NXpData;

typedef struct __NexusFile5 {
  struct iStack5 {
    char irefn[1024];
    hid_t iVref;
    hsize_t iCurrentIDX;
  } iStack5[NXMAXSTACK];
  struct iStack5 iAtt5;
  hid_t iFID;
  hid_t iCurrentG;
  hid_t iCurrentD;
  hid_t iCurrentS;
  hid_t iCurrentT;
  hid_t iCurrentA;
  int iNX;
  int iNXID;
  int iStackPtr;
  char *iCurrentLGG;
  char *iCurrentLD;
  char name_ref[1024];
  char name_tmp[1024];
  char iAccess[2];
} NexusFile5, *pNexusFile5;

/* forward declaration of NX5closegroup in order to get rid of a nasty warning */

NXstatus NX5closegroup(NXhandle fid);

/*-------------------------------------------------------------------*/

static pNexusFile5 NXI5assert(NXhandle fid) {
  pNexusFile5 pRes;

  assert(fid != NULL);
  pRes = static_cast<pNexusFile5>(fid);
  assert(pRes->iNXID == NX5SIGNATURE);
  return pRes;
}

/*--------------------------------------------------------------------*/

static void NXI5KillDir(pNexusFile5 self) { self->iStack5[self->iStackPtr].iCurrentIDX = 0; }

static herr_t readStringAttribute(hid_t attr, char **data) {
  herr_t iRet = 0;
  hid_t atype = -1;
  hid_t space;
  int ndims;
  hsize_t thedims[H5S_MAX_RANK], sdim;

  atype = H5Aget_type(attr);
  sdim = H5Tget_size(atype);
  space = H5Aget_space(attr);
  ndims = H5Sget_simple_extent_dims(space, thedims, NULL);

  if (ndims == 0) {
    if (H5Tis_variable_str(atype)) {
      hid_t btype = H5Tget_native_type(atype, H5T_DIR_ASCEND);
      iRet = H5Aread(attr, btype, data);
      H5Tclose(btype);
    } else {
      *data = static_cast<char *>(malloc(sdim + 1));
      iRet = H5Aread(attr, atype, *data);
      (*data)[sdim] = '\0';
    }
  } else if (ndims == 1) {
    char **strings;

    strings = static_cast<char **>(malloc(thedims[0] * sizeof(char *)));

    if (!H5Tis_variable_str(atype)) {
      strings[0] = static_cast<char *>(malloc(thedims[0] * sdim * sizeof(char)));
      for (hsize_t i = 1; i < thedims[0]; i++) {
        strings[i] = strings[0] + i * sdim;
      }
    }

    iRet = H5Aread(attr, atype, strings[0]);
    *data = static_cast<char *>(calloc((sdim + 2) * thedims[0], sizeof(char)));
    for (hsize_t i = 0; i < thedims[0]; i++) {
      if (i == 0) {
        strncpy(*data, strings[i], sdim);
      } else {
        strcat(*data, ", ");
        strncat(*data, strings[i], sdim);
      }
    }
    if (H5Tis_variable_str(atype)) {
      H5Dvlen_reclaim(atype, space, H5P_DEFAULT, strings);
    } else {
      free(strings[0]);
    }

    free(strings);
  } else {
    *data = strdup(" higher dimensional string array");
  }

  H5Tclose(atype);
  H5Sclose(space);
  if (iRet < 0)
    return static_cast<herr_t>(NXstatus::NX_ERROR);
  return static_cast<herr_t>(NXstatus::NX_OK);
}

static herr_t readStringAttributeN(hid_t attr, char *data, int maxlen) {
  herr_t iRet;
  char *vdat = NULL;
  iRet = readStringAttribute(attr, &vdat);
  if (iRet >= 0) {
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
    // there is a danger of overflowing output string
    std::strncpy(data, vdat, static_cast<size_t>(maxlen));
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic pop
#endif
    free(vdat);
  }
  data[maxlen - 1] = '\0';
  return iRet;
}

/*--------------------------------------------------------------------*/

static void NXI5KillAttDir(pNexusFile5 self) { self->iAtt5.iCurrentIDX = 0; }

/*---------------------------------------------------------------------*/
static void buildCurrentPath(pNexusFile5 self, char *pathBuffer, // cppcheck-suppress constParameterPointer
                             int pathBufferLen) {

  memset(pathBuffer, 0, static_cast<size_t>(pathBufferLen));
  if (self->iCurrentG != 0) {
    strcpy(pathBuffer, "/");
    if ((int)strlen(self->name_ref) + 1 < pathBufferLen) {
      strcat(pathBuffer, self->name_ref);
    }
  }
  if (self->iCurrentD != 0) {
    strcat(pathBuffer, "/");
    if ((int)strlen(self->iCurrentLD) + (int)strlen(pathBuffer) < pathBufferLen) {
      strcat(pathBuffer, self->iCurrentLD);
    }
  }
}

/* ----------------------------------------------------------------------

   Definition of NeXus API

   --------------------------------------------------------------------- */

NXstatus NX5reopen(NXhandle pOrigHandle, NXhandle *pNewHandle) {
  pNexusFile5 pNew = NULL, pOrig = NULL;
  *pNewHandle = NULL;
  pOrig = static_cast<pNexusFile5>(pOrigHandle);
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
  strcpy(pNew->iAccess, pOrig->iAccess);
  pNew->iNXID = NX5SIGNATURE;
  pNew->iStack5[0].iVref = 0; /* root! */
  *pNewHandle = static_cast<NXhandle>(pNew);
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------
 * private functions used in NX5open
 */

pNexusFile5 create_file_struct() {
  pNexusFile5 pNew = static_cast<pNexusFile5>(malloc(sizeof(NexusFile5)));
  if (!pNew) {
    NXReportError("ERROR: not enough memory to create file structure");
  } else {
    memset(pNew, 0, sizeof(NexusFile5));
  }

  return pNew;
}

hid_t create_file_access_plist(CONSTCHAR *filename) {
  char pBuffer[512];
  hid_t fapl = -1;

  /* create file access property list - required in all cases*/
  if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0) {
    sprintf(pBuffer,
            "Error: failed to create file access property "
            "list for file %s",
            filename);
    NXReportError(pBuffer);
    return fapl;
  }

  /* set file close policy - need this in all cases*/
  if (H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0) {
    sprintf(pBuffer,
            "Error: cannot set close policy for file "
            "%s",
            filename);
    NXReportError(pBuffer);
    return fapl;
  }

  return fapl;
}

herr_t set_file_cache(hid_t fapl, CONSTCHAR *filename) {
  char pBuffer[512];
  int mdc_nelmts;
  size_t rdcc_nelmts;
  size_t rdcc_nbytes;
  double rdcc_w0;
  herr_t error = -1;

  error = H5Pget_cache(fapl, &mdc_nelmts, &rdcc_nelmts, &rdcc_nbytes, &rdcc_w0);

  if (error < 0) {
    sprintf(pBuffer,
            "Error: cannot obtain HDF5 cache size"
            " for file %s",
            filename);
    NXReportError(pBuffer);
    return error;
  }

  rdcc_nbytes = (size_t)nx_cacheSize;
  error = H5Pset_cache(fapl, mdc_nelmts, rdcc_nelmts, rdcc_nbytes, rdcc_w0);
  if (error < 0) {
    sprintf(pBuffer,
            "Error: cannot set cache size "
            "for file %s",
            filename);
    NXReportError(pBuffer);
    return error;
  }
  return error;
}

herr_t set_str_attribute(hid_t parent_id, CONSTCHAR *name, CONSTCHAR *buffer) {
  char pBuffer[512];
  hid_t attr_id;
  hid_t space_id = H5Screate(H5S_SCALAR);
  hid_t type_id = H5Tcopy(H5T_C_S1);

  H5Tset_size(type_id, strlen(buffer));

  attr_id = H5Acreate(parent_id, name, type_id, space_id, H5P_DEFAULT, H5P_DEFAULT);
  if (attr_id < 0) {
    sprintf(pBuffer, "ERROR: failed to create %s attribute", name);
    NXReportError(pBuffer);
    return -1;
  }

  if (H5Awrite(attr_id, type_id, buffer) < 0) {
    sprintf(pBuffer, "ERROR: failed writting %s attribute", name);
    NXReportError(pBuffer);
    return -1;
  }

  H5Tclose(type_id);
  H5Sclose(space_id);
  H5Aclose(attr_id);

  return 0;
}

NXstatus NX5open(CONSTCHAR *filename, NXaccess am, NXhandle *pHandle) {
  hid_t root_id;
  pNexusFile5 pNew = NULL;
  char pBuffer[512];
  char *time_buffer = NULL;
  char version_nr[10];
  unsigned int vers_major, vers_minor, vers_release, am1;
  hid_t fapl = -1;

  *pHandle = NULL;

  if (H5get_libversion(&vers_major, &vers_minor, &vers_release) < 0) {
    NXReportError("ERROR: cannot determine HDF5 library version");
    return NXstatus::NX_ERROR;
  }
  if (vers_major == 1 && vers_minor < 8) {
    NXReportError("ERROR: HDF5 library 1.8.0 or higher required");
    return NXstatus::NX_ERROR;
  }

  /* mask of any options for now */
  am = (NXaccess)(am & NXACCMASK_REMOVEFLAGS);

  /* turn off the automatic HDF error handling */
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);
#ifdef USE_FTIME
  struct timeb timeb_struct;
#endif

  /* create the new file structure */
  if (!(pNew = create_file_struct()))
    return NXstatus::NX_ERROR;

  /* create the file access property list*/
  if ((fapl = create_file_access_plist(filename)) < 0) {
    free(pNew);
    return NXstatus::NX_ERROR;
  }

  /* start HDF5 interface */
  if (am == NXACC_CREATE5) {

    /* set the cache size for the file */
    if (set_file_cache(fapl, filename) < 0) {
      free(pNew);
      return NXstatus::NX_ERROR;
    }

    am1 = H5F_ACC_TRUNC;
    pNew->iFID = H5Fcreate(filename, am1, H5P_DEFAULT, fapl);
  } else {
    if (am == NXACC_READ)
      am1 = H5F_ACC_RDONLY;
    else
      am1 = H5F_ACC_RDWR;

    pNew->iFID = H5Fopen(filename, am1, fapl);
  }

  if (fapl != -1)
    H5Pclose(fapl); /*close file access property list*/

  if (pNew->iFID <= 0) {
    snprintf(pBuffer, sizeof(pBuffer) - 1, "ERROR: cannot open file: %s", filename);
    NXReportError(pBuffer);
    free(pNew);
    return NXstatus::NX_ERROR;
  }

  /*
   * need to create global attributes         file_name file_time NeXus_version
   * at some point for new files
   */

  if (am == NXACC_CREATE5) {
    root_id = H5Gopen(pNew->iFID, "/", H5P_DEFAULT);
    if (set_str_attribute(root_id, "NeXus_version", NEXUS_VERSION) < 0) {
      H5Gclose(root_id);
      H5Fclose(pNew->iFID);
      free(pNew);
      return NXstatus::NX_ERROR;
    }

    if (set_str_attribute(root_id, "file_name", filename) < 0) {
      H5Gclose(root_id);
      H5Fclose(pNew->iFID);
      free(pNew);
      return NXstatus::NX_ERROR;
    }

    sprintf(version_nr, "%u.%u.%u", vers_major, vers_minor, vers_release);
    if (set_str_attribute(root_id, "HDF5_Version", version_nr) < 0) {
      H5Gclose(root_id);
      H5Fclose(pNew->iFID);
      free(pNew);
      return NXstatus::NX_ERROR;
    }

    time_buffer = NXIformatNeXusTime();
    if (time_buffer != NULL) {
      if (set_str_attribute(root_id, "file_time", time_buffer) < 0) {
        H5Gclose(root_id);
        H5Fclose(pNew->iFID);
        free(pNew);
        free(time_buffer);
        return NXstatus::NX_ERROR;
      }
      free(time_buffer);
    }

    /*finally we set the NXroot NX_class attribute*/
    if (set_str_attribute(root_id, "NX_class", "NXroot") < 0) {
      H5Gclose(root_id);
      H5Fclose(pNew->iFID);
      free(pNew);
      return NXstatus::NX_ERROR;
    }

    H5Gclose(root_id);
  }

  /* Set HDFgroup access mode */
  if (am1 == H5F_ACC_RDONLY) {
    strcpy(pNew->iAccess, "r");
  } else {
    strcpy(pNew->iAccess, "w");
  }
  pNew->iNXID = NX5SIGNATURE;
  pNew->iStack5[0].iVref = 0; /* root! */
  *pHandle = static_cast<NXhandle>(pNew);
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------------- */

NXstatus NX5close(NXhandle *fid) {
  pNexusFile5 pFile = NULL;
  herr_t iRet;

  pFile = NXI5assert(*fid);

  iRet = 0;
  /*
     printf("HDF5 object count before close: %d\n",
     H5Fget_obj_count(pFile->iFID,H5F_OBJ_ALL));
   */
  iRet = H5Fclose(pFile->iFID);

  /*
     leave this here: it helps in debugging leakage problems
     printf("HDF5 object count after close: %d\n",
     H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_ALL));
     printf("HDF5 dataset count after close: %d\n",
     H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_DATASET));
     printf("HDF5 group count after close: %d\n",
     H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_GROUP));
     printf("HDF5 datatype count after close: %d\n",
     H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_DATATYPE));
     printf("HDF5 attribute count after close: %d\n",
     H5Fget_obj_count(H5F_OBJ_ALL,H5F_OBJ_ATTR));
   */

  if (iRet < 0) {
    NXReportError("ERROR: cannot close HDF file");
  }
  /* release memory */
  NXI5KillDir(pFile);
  if (pFile->iCurrentLGG != NULL) {
    free(pFile->iCurrentLGG);
  }
  if (pFile->iCurrentLD != NULL) {
    free(pFile->iCurrentLD);
  }
  free(pFile);
  *fid = NULL;
  H5garbage_collect();
  return NXstatus::NX_OK;
}

/*-----------------------------------------------------------------------*/

NXstatus NX5makegroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
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

herr_t attr_check(hid_t loc_id, const char *member_name, const H5A_info_t *unused, void *opdata) {
  UNUSED_ARG(loc_id);
  UNUSED_ARG(unused);
  UNUSED_ARG(opdata);
  char attr_name[8 + 1]; /* need to leave space for \0 as well */

  strcpy(attr_name, "NX_class");
  return strstr(member_name, attr_name) ? 1 : 0;
}

/*------------------------------------------------------------------------*/
NXstatus NX5opengroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {

  pNexusFile5 pFile;
  hid_t attr1, atype, iVID;
  herr_t iRet;
  char pBuffer[NX_MAXPATHLEN + 12]; // no idea what the 12 is about

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
  pFile->iAtt5.iCurrentIDX = 0;
  pFile->iCurrentD = 0;
  if (pFile->iCurrentLGG != NULL) {
    free(pFile->iCurrentLGG);
  }
  pFile->iCurrentLGG = strdup(name);
  NXI5KillDir(pFile);
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5closegroup(NXhandle fid) {
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

/*-----------------------------------------------------------------------*/
std::map<NXnumtype, hid_t> const nxToHDF5Map{{NXnumtype::CHAR, H5T_C_S1},
                                             {NXnumtype::INT8, H5T_NATIVE_CHAR},
                                             {NXnumtype::UINT8, H5T_NATIVE_UCHAR},
                                             {NXnumtype::INT16, H5T_NATIVE_SHORT},
                                             {NXnumtype::UINT16, H5T_NATIVE_USHORT},
                                             {NXnumtype::INT32, H5T_NATIVE_INT},
                                             {NXnumtype::UINT32, H5T_NATIVE_UINT},
                                             {NXnumtype::INT64, H5T_NATIVE_INT64},
                                             {NXnumtype::UINT64, H5T_NATIVE_UINT64},
                                             {NXnumtype::FLOAT32, H5T_NATIVE_FLOAT},
                                             {NXnumtype::FLOAT64, H5T_NATIVE_DOUBLE}};

static hid_t nxToHDF5Type(NXnumtype type) {
  auto const iter = nxToHDF5Map.find(type);
  if (iter != nxToHDF5Map.cend()) {
    return iter->second;
  } else {
    NXReportError("ERROR: nxToHDF5Type: unknown type");
    return -1;
  }
}

/* --------------------------------------------------------------------- */

NXstatus NX5compmakedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[],
                           int compress_type, int64_t const chunk_size[]) {
  hid_t datatype1, dataspace, iNew;
  hid_t type, cparms = -1;
  pNexusFile5 pFile;
  char pBuffer[256];
  size_t byte_zahl = 0;
  hsize_t chunkdims[H5S_MAX_RANK];
  hsize_t mydim[H5S_MAX_RANK], mydim1[H5S_MAX_RANK];
  hsize_t size[H5S_MAX_RANK];
  hsize_t maxdims[H5S_MAX_RANK];
  unsigned int compress_level;
  int unlimiteddim = 0;

  pFile = NXI5assert(fid);
  if (pFile->iCurrentG <= 0) {
    sprintf(pBuffer, "ERROR: no group open for makedata on %s", name);
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }

  if (rank <= 0) {
    sprintf(pBuffer, "ERROR: invalid rank specified %s", name);
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }

  type = nxToHDF5Type(datatype);

  /*
     Check dimensions for consistency. Dimension may be -1
     thus denoting an unlimited dimension.
   */
  for (int i = 0; i < rank; i++) {
    chunkdims[i] = static_cast<hsize_t>(chunk_size[i]);
    mydim[i] = static_cast<hsize_t>(dimensions[i]);
    maxdims[i] = static_cast<hsize_t>(dimensions[i]);
    size[i] = static_cast<hsize_t>(dimensions[i]);
    if (dimensions[i] <= 0) {
      mydim[i] = 1;
      maxdims[i] = H5S_UNLIMITED;
      size[i] = 1;
      unlimiteddim = 1;
    } else {
      mydim[i] = static_cast<hsize_t>(dimensions[i]);
      maxdims[i] = static_cast<hsize_t>(dimensions[i]);
      size[i] = static_cast<hsize_t>(dimensions[i]);
    }
  }

  if (datatype == NXnumtype::CHAR) {
    /*
     *  This assumes string lenght is in the last dimensions and
     *  the logic must be the same as used in NX5getslab and NX5getinfo
     *
     *  search for tests on H5T_STRING
     */
    byte_zahl = (size_t)mydim[rank - 1];
    for (int i = 0; i < rank; i++) {
      mydim1[i] = mydim[i];
      if (dimensions[i] <= 0) {
        mydim1[0] = 1;
        maxdims[0] = H5S_UNLIMITED;
      }
    }
    mydim1[rank - 1] = 1;
    if (mydim[rank - 1] > 1) {
      mydim[rank - 1] = maxdims[rank - 1] = size[rank - 1] = 1;
    }
    if (chunkdims[rank - 1] > 1) {
      chunkdims[rank - 1] = 1;
    }
    dataspace = H5Screate_simple(rank, mydim1, maxdims);
  } else {
    if (unlimiteddim) {
      dataspace = H5Screate_simple(rank, mydim, maxdims);
    } else {
      /* dataset creation */
      dataspace = H5Screate_simple(rank, mydim, NULL);
    }
  }
  datatype1 = H5Tcopy(type);
  if (datatype == NXnumtype::CHAR) {
    H5Tset_size(datatype1, byte_zahl);
    /*       H5Tset_strpad(H5T_STR_SPACEPAD); */
  }
  compress_level = 6;
  if ((compress_type / 100) == NX_COMP_LZW) {
    compress_level = static_cast<unsigned int>(compress_type % 100);
    compress_type = NX_COMP_LZW;
  }
  hid_t dID;
  if (compress_type == NX_COMP_LZW) {
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    iNew = H5Pset_chunk(cparms, rank, chunkdims);
    if (iNew < 0) {
      NXReportError("ERROR: size of chunks could not be set");
      return NXstatus::NX_ERROR;
    }
    H5Pset_shuffle(cparms); // mrt: improves compression
    H5Pset_deflate(cparms, compress_level);
    dID = H5Dcreate(pFile->iCurrentG, static_cast<const char *>(name), datatype1, dataspace, H5P_DEFAULT, cparms,
                    H5P_DEFAULT);
  } else if (compress_type == NX_COMP_NONE) {
    if (unlimiteddim) {
      cparms = H5Pcreate(H5P_DATASET_CREATE);
      iNew = H5Pset_chunk(cparms, rank, chunkdims);
      if (iNew < 0) {
        NXReportError("ERROR: size of chunks could not be set");
        return NXstatus::NX_ERROR;
      }
      dID = H5Dcreate(pFile->iCurrentG, static_cast<const char *>(name), datatype1, dataspace, H5P_DEFAULT, cparms,
                      H5P_DEFAULT);
    } else {
      dID = H5Dcreate(pFile->iCurrentG, static_cast<const char *>(name), datatype1, dataspace, H5P_DEFAULT, H5P_DEFAULT,
                      H5P_DEFAULT);
    }
  } else if (compress_type == NX_CHUNK) {
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    iNew = H5Pset_chunk(cparms, rank, chunkdims);
    if (iNew < 0) {
      NXReportError("ERROR: size of chunks could not be set");
      return NXstatus::NX_ERROR;
    }
    dID = H5Dcreate(pFile->iCurrentG, static_cast<const char *>(name), datatype1, dataspace, H5P_DEFAULT, cparms,
                    H5P_DEFAULT);

  } else {
    NXReportError("HDF5 doesn't support selected compression method! Dataset created without compression");
    dID = H5Dcreate(pFile->iCurrentG, static_cast<const char *>(name), datatype1, dataspace, H5P_DEFAULT, H5P_DEFAULT,
                    H5P_DEFAULT);
  }
  if (dID < 0) {
    NXReportError("ERROR: creating chunked dataset failed");
    return NXstatus::NX_ERROR;
  } else {
    pFile->iCurrentD = dID;
  }
  if (unlimiteddim) {
    iNew = H5Dset_extent(pFile->iCurrentD, size);
    if (iNew < 0) {
      sprintf(pBuffer, "ERROR: cannot create dataset %s", name);
      NXReportError(pBuffer);
      return NXstatus::NX_ERROR;
    }
  }
  herr_t iRet;
  if (cparms != -1) {
    H5Pclose(cparms);
  }
  iRet = H5Sclose(dataspace);
  iRet += H5Tclose(datatype1);
  iRet += H5Dclose(pFile->iCurrentD);
  pFile->iCurrentD = 0;
  if (iRet < 0) {
    NXReportError("ERROR: HDF cannot close dataset");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* --------------------------------------------------------------------- */

NXstatus NX5makedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[]) {
  int64_t chunk_size[H5S_MAX_RANK];
  int i;

  NXI5assert(fid);

  memset(chunk_size, 0, H5S_MAX_RANK * sizeof(int64_t));
  memcpy(chunk_size, dimensions, static_cast<size_t>(rank) * sizeof(int64_t));
  for (i = 0; i < rank; i++) {
    if (dimensions[i] == NX_UNLIMITED || dimensions[i] <= 0) {
      chunk_size[i] = 1;
    }
  }
  return NX5compmakedata64(fid, name, datatype, rank, dimensions, NX_COMP_NONE, chunk_size);
}

/* --------------------------------------------------------------------- */

NXstatus NX5compress(NXhandle fid, int compress_type) {
  UNUSED_ARG(fid);
  UNUSED_ARG(compress_type);
  printf(" NXcompress ERROR: NeXus API  based  on  HDF5  doesn't support\n");
  printf("                   NXcompress  function!  Using  HDF5 library,\n");
  printf("                   the NXcompmakedata function can be applied\n");
  printf("                   for compression of data!\n");
  return NXstatus::NX_ERROR;
}

/* --------------------------------------------------------------------- */

NXstatus NX5opendata(NXhandle fid, CONSTCHAR *name) {
  pNexusFile5 pFile;

  pFile = NXI5assert(fid);
  /* clear pending attribute directories first */
  NXI5KillAttDir(pFile);

  /* find the ID number and open the dataset */
  pFile->iCurrentD = H5Dopen(pFile->iCurrentG, name, H5P_DEFAULT);
  if (pFile->iCurrentD < 0) {
    char pBuffer[256];
    sprintf(pBuffer, "ERROR: dataset \"%s\" not found at this level", name);
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }
  /* find the ID number of datatype */
  pFile->iCurrentT = H5Dget_type(pFile->iCurrentD);
  if (pFile->iCurrentT < 0) {
    NXReportError("ERROR: error opening dataset");
    pFile->iCurrentT = 0;
    return NXstatus::NX_ERROR;
  }
  /* find the ID number of dataspace */
  pFile->iCurrentS = H5Dget_space(pFile->iCurrentD);
  if (pFile->iCurrentS < 0) {
    NXReportError("ERROR:HDF error opening dataset");
    pFile->iCurrentS = 0;
    return NXstatus::NX_ERROR;
  }
  if (pFile->iCurrentLD != NULL) {
    free(pFile->iCurrentLD);
  }
  pFile->iCurrentLD = strdup(name);

  return NXstatus::NX_OK;
}

/* ----------------------------------------------------------------- */

NXstatus NX5closedata(NXhandle fid) {
  pNexusFile5 pFile;
  herr_t iRet;

  pFile = NXI5assert(fid);
  iRet = H5Sclose(pFile->iCurrentS);
  iRet += H5Tclose(pFile->iCurrentT);
  iRet += H5Dclose(pFile->iCurrentD);
  if (iRet < 0) {
    NXReportError("ERROR: cannot end access to dataset");
    return NXstatus::NX_ERROR;
  }
  pFile->iCurrentD = 0;
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5putdata(NXhandle fid, const void *data) {
  pNexusFile5 pFile;
  herr_t iRet;
  int64_t myStart[H5S_MAX_RANK];
  int64_t mySize[H5S_MAX_RANK];
  hsize_t thedims[H5S_MAX_RANK], maxdims[H5S_MAX_RANK];
  int i, rank, unlimiteddim = 0;

  pFile = NXI5assert(fid);
  rank = H5Sget_simple_extent_ndims(pFile->iCurrentS);
  if (rank < 0) {
    NXReportError("ERROR: Cannot determine dataset rank");
    return NXstatus::NX_ERROR;
  }
  iRet = H5Sget_simple_extent_dims(pFile->iCurrentS, thedims, maxdims);
  if (iRet < 0) {
    NXReportError("ERROR: Cannot determine dataset dimensions");
    return NXstatus::NX_ERROR;
  }
  for (i = 0; i < rank; ++i) {
    myStart[i] = 0;
    mySize[i] = static_cast<int64_t>(thedims[i]);
    if (maxdims[i] == H5S_UNLIMITED) {
      unlimiteddim = 1;
      myStart[i] = static_cast<int64_t>(thedims[i] + 1);
      mySize[i] = 1;
    }
  }
  /* If we are using putdata on an unlimied dimesnion dataset, assume we want to append one single new slab */
  if (unlimiteddim) {
    return NX5putslab64(fid, data, myStart, mySize);
  } else {
    iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (iRet < 0) {
      char pError[512] = "";
      snprintf(pError, sizeof(pError) - 1, "ERROR: failure to write data");
      NXReportError(pError);
      return NXstatus::NX_ERROR;
    }
  }
  return NXstatus::NX_OK;
}

/*------------------------------------------------------------------*/
static hid_t getAttVID(pNexusFile5 pFile) {
  hid_t vid;
  if (pFile->iCurrentG == 0 && pFile->iCurrentD == 0) {
    /* global attribute */
    vid = H5Gopen(pFile->iFID, "/", H5P_DEFAULT);
  } else if (pFile->iCurrentD != 0) {
    /* dataset attribute */
    vid = pFile->iCurrentD;
  } else {
    /* group attribute */;
    vid = pFile->iCurrentG;
  }
  return vid;
}

/*---------------------------------------------------------------*/
static void killAttVID(const pNexusFile5 pFile, hid_t vid) {
  if (pFile->iCurrentG == 0 && pFile->iCurrentD == 0) {
    H5Gclose(vid);
  }
}

/* ------------------------------------------------------------------- */

NXstatus NX5putattr(NXhandle fid, CONSTCHAR *name, const void *data, int datalen, NXnumtype iType) {
  pNexusFile5 pFile;
  hid_t attr1, aid1, aid2;
  hid_t type;
  herr_t iRet = 0;
  hid_t vid, attRet;

  pFile = NXI5assert(fid);

  type = nxToHDF5Type(iType);

  /* determine vid */
  vid = getAttVID(pFile);
  attRet = H5Aopen_by_name(vid, ".", name, H5P_DEFAULT, H5P_DEFAULT);
  if (attRet > 0) {
    H5Aclose(attRet);
    iRet = H5Adelete(vid, name);
    if (iRet < 0) {
      NXReportError("ERROR: old attribute cannot be removed! ");
      killAttVID(pFile, vid);
      return NXstatus::NX_ERROR;
    }
  }
  aid2 = H5Screate(H5S_SCALAR);
  aid1 = H5Tcopy(type);
  if (iType == NXnumtype::CHAR) {
    H5Tset_size(aid1, static_cast<size_t>(datalen));
  }
  attr1 = H5Acreate(vid, name, aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attr1 < 0) {
    NXReportError("ERROR: attribute cannot created! ");
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }
  if (H5Awrite(attr1, aid1, data) < 0) {
    NXReportError("ERROR: failed to store attribute ");
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }
  /* Close attribute dataspace */
  iRet += H5Tclose(aid1);
  iRet += H5Sclose(aid2);
  /* Close attribute  */
  iRet += H5Aclose(attr1);
  killAttVID(pFile, vid);
  // TODO QUESTION always return that it is ok?
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5putslab64(NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[]) {
  pNexusFile5 pFile;
  int iRet, rank;
  hsize_t myStart[H5S_MAX_RANK];
  hsize_t mySize[H5S_MAX_RANK];
  hsize_t size[H5S_MAX_RANK], thedims[H5S_MAX_RANK], maxdims[H5S_MAX_RANK];
  hid_t filespace, dataspace;
  int unlimiteddim = 0;

  pFile = NXI5assert(fid);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    NXReportError("ERROR: no dataset open");
    return NXstatus::NX_ERROR;
  }
  rank = H5Sget_simple_extent_ndims(pFile->iCurrentS);
  if (rank < 0) {
    NXReportError("ERROR: cannot get rank");
    return NXstatus::NX_ERROR;
  }
  iRet = H5Sget_simple_extent_dims(pFile->iCurrentS, thedims, maxdims);
  if (iRet < 0) {
    NXReportError("ERROR: cannot get dimensions");
    return NXstatus::NX_ERROR;
  }

  for (int i = 0; i < rank; i++) {
    myStart[i] = static_cast<hsize_t>(iStart[i]);
    mySize[i] = static_cast<hsize_t>(iSize[i]);
    size[i] = static_cast<hsize_t>(iStart[i] + iSize[i]);
    if (maxdims[i] == H5S_UNLIMITED) {
      unlimiteddim = 1;
    }
  }
  if (H5Tget_class(pFile->iCurrentT) == H5T_STRING) {
    mySize[rank - 1] = 1;
    myStart[rank - 1] = 0;
    size[rank - 1] = 1;
  }
  dataspace = H5Screate_simple(rank, mySize, NULL);
  if (unlimiteddim) {
    for (int i = 0; i < rank; i++) {
      if (size[i] < thedims[i]) {
        size[i] = thedims[i];
      }
    }
    iRet = H5Dset_extent(pFile->iCurrentD, size);
    if (iRet < 0) {
      NXReportError("ERROR: extend slab failed");
      return NXstatus::NX_ERROR;
    }

    filespace = H5Dget_space(pFile->iCurrentD);

    /* define slab */
    iRet = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, myStart, NULL, mySize, NULL);
    /* deal with HDF errors */
    if (iRet < 0) {
      NXReportError("ERROR: selecting slab failed");
      return NXstatus::NX_ERROR;
    }
    /* write slab */
    iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, dataspace, filespace, H5P_DEFAULT, data);
    if (iRet < 0) {
      NXReportError("ERROR: writing slab failed");
    }
    /* update with new size */
    iRet = H5Sclose(pFile->iCurrentS);
    if (iRet < 0) {
      NXReportError("ERROR: updating size failed");
    }
    pFile->iCurrentS = filespace;
  } else {
    /* define slab */
    iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, myStart, NULL, mySize, NULL);
    /* deal with HDF errors */
    if (iRet < 0) {
      NXReportError("ERROR: selecting slab failed");
      return NXstatus::NX_ERROR;
    }
    /* write slab */
    iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, dataspace, pFile->iCurrentS, H5P_DEFAULT, data);
    if (iRet < 0) {
      NXReportError("ERROR: writing slab failed");
    }
  }
  /* deal with HDF errors */
  iRet = H5Sclose(dataspace);
  if (iRet < 0) {
    NXReportError("ERROR: closing slab failed");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5getdataID(NXhandle fid, NXlink *sRes) {
  pNexusFile5 pFile;
  int datalen;
  NXnumtype type = NXnumtype::CHAR;

  pFile = NXI5assert(fid);

  /* we cannot return ID's when no datset is open */
  if (pFile->iCurrentD <= 0) {
    return NXstatus::NX_ERROR;
  }

  /*
     this means: if the item is already linked: use the target attribute else,
     the path to the current node
   */
  NXMDisableErrorReporting();
  datalen = 1024;
  memset(&sRes->targetPath, 0, static_cast<size_t>(datalen) * sizeof(char));
  if (NX5getattr(fid, "target", &sRes->targetPath, &datalen, &type) != NXstatus::NX_OK) {
    buildCurrentPath(pFile, sRes->targetPath, 1024);
  }
  NXMEnableErrorReporting();
  sRes->linkType = 1;
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5printlink(NXhandle fid, NXlink const *sLink) {
  NXI5assert(fid);
  printf("HDF5 link: targetPath = \"%s\", linkType = \"%d\"\n", sLink->targetPath, sLink->linkType);
  return NXstatus::NX_OK;
}

/*--------------------------------------------------------------------*/
static NXstatus NX5settargetattribute(pNexusFile5 pFile, NXlink *sLink) {
  hid_t dataID, aid2, aid1, attID;
  char name[] = "target";

  /*
     set the target attribute
   */
  if (sLink->linkType > 0) {
    dataID = H5Dopen(pFile->iFID, sLink->targetPath, H5P_DEFAULT);
  } else {
    dataID = H5Gopen(pFile->iFID, sLink->targetPath, H5P_DEFAULT);
  }
  if (dataID < 0) {
    NXReportError("Internal error, path to link does not exist");
    return NXstatus::NX_ERROR;
  }
  hid_t status = H5Aopen_by_name(dataID, ".", name, H5P_DEFAULT, H5P_DEFAULT);
  if (status > 0) {
    H5Aclose(status);
    status = H5Adelete(dataID, name);
    if (status < 0) {
      return NXstatus::NX_OK;
    }
  }
  aid2 = H5Screate(H5S_SCALAR);
  aid1 = H5Tcopy(H5T_C_S1);
  H5Tset_size(aid1, strlen(sLink->targetPath));
  attID = H5Acreate(dataID, name, aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attID < 0) {
    return NXstatus::NX_OK;
  }
  UNUSED_ARG(H5Awrite(attID, aid1, sLink->targetPath));
  H5Tclose(aid1);
  H5Sclose(aid2);
  H5Aclose(attID);
  if (sLink->linkType > 0) {
    H5Dclose(dataID);
  } else {
    H5Gclose(dataID);
  }
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------*/

NXstatus NX5makenamedlink(NXhandle fid, CONSTCHAR *name, NXlink *sLink) {
  pNexusFile5 pFile;
  char linkTarget[NX_MAXPATHLEN];

  pFile = NXI5assert(fid);
  if (pFile->iCurrentG == 0) { /* root level, can not link here */
    return NXstatus::NX_ERROR;
  }

  /*
     build pathname to link from our current group and the name
     of the thing to link
   */
  if (strlen(pFile->name_ref) + strlen(name) + 2 < NX_MAXPATHLEN) {
    strcpy(linkTarget, "/");
    strcat(linkTarget, pFile->name_ref);
    strcat(linkTarget, "/");
    strcat(linkTarget, name);
  } else {
    NXReportError("ERROR: path string to long");
    return NXstatus::NX_ERROR;
  }

  H5Lcreate_hard(pFile->iFID, sLink->targetPath, H5L_SAME_LOC, linkTarget, H5P_DEFAULT, H5P_DEFAULT);

  return NX5settargetattribute(pFile, sLink);
}

/* ------------------------------------------------------------------- */

NXstatus NX5makelink(NXhandle fid, NXlink *sLink) {
  pNexusFile5 pFile;
  char linkTarget[NX_MAXPATHLEN];
  char *itemName = NULL;

  pFile = NXI5assert(fid);
  if (pFile->iCurrentG == 0) { /* root level, can not link here */
    return NXstatus::NX_ERROR;
  }

  /*
     locate name of the element to link
   */
  itemName = strrchr(sLink->targetPath, '/');
  if (itemName == NULL) {
    NXReportError("ERROR: bad link structure");
    return NXstatus::NX_ERROR;
  }
  itemName++;

  /*
     build pathname to link from our current group and the name
     of the thing to link
   */
  if (strlen(pFile->name_ref) + strlen(itemName) + 2 < NX_MAXPATHLEN) {
    strcpy(linkTarget, "/");
    strcat(linkTarget, pFile->name_ref);
    strcat(linkTarget, "/");
    strcat(linkTarget, itemName);
  } else {
    NXReportError("ERROR: path string to long");
    return NXstatus::NX_ERROR;
  }

  H5Lcreate_hard(pFile->iFID, sLink->targetPath, H5L_SAME_LOC, linkTarget, H5P_DEFAULT, H5P_DEFAULT);

  return NX5settargetattribute(pFile, sLink);
}

/*----------------------------------------------------------------------*/

NXstatus NX5flush(NXhandle *pHandle) {
  pNexusFile5 pFile = NULL;
  herr_t iRet;

  pFile = NXI5assert(*pHandle);
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

/* Operator function. */

herr_t nxgroup_info(hid_t loc_id, const char *name, const H5L_info_t *statbuf, void *op_data) {
  UNUSED_ARG(statbuf);
  pinfo self = static_cast<pinfo>(op_data);
  H5O_info1_t object_info;
  // TODO use new version of method rather than v2
  H5Oget_info_by_name2(loc_id, name, &object_info, H5O_INFO_ALL, H5P_DEFAULT);
  switch ((object_info).type) {
  case H5O_TYPE_GROUP:
    self->iname = strdup(name);
    self->type = H5O_TYPE_GROUP;
    break;
  case H5O_TYPE_DATASET:
    self->iname = strdup(name);
    self->type = H5O_TYPE_DATASET;
    break;
  default:
    // TODO defaults to group. not what we would want?
    self->type = 0;
    break;
  }
  return 1;
}

/* --------------------------------------------------------------------- */

/* Operator function. */

herr_t group_info(hid_t loc_id, const char *name, const H5L_info_t *statbuf, void *opdata) {
  UNUSED_ARG(statbuf);
  int iNX = *(static_cast<int *>(opdata));
  H5O_info1_t object_info;
  // TODO use new version of method rather than v2
  H5Oget_info_by_name2(loc_id, name, &object_info, H5O_INFO_ALL, H5P_DEFAULT);
  switch ((object_info).type) {
  case H5O_TYPE_GROUP:
    iNX++;
    *(static_cast<int *>(opdata)) = iNX;
    break;
  case H5O_TYPE_DATASET:
    iNX++;
    *(static_cast<int *>(opdata)) = iNX;
    break;
  default:
    break;
  }
  return 0;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getgroupinfo_recurse(NXhandle fid, int *iN, NXname pName, NXname pClass) {
  pNexusFile5 pFile;
  hid_t atype, attr_id, grp;

  pFile = NXI5assert(fid);
  /* check if there is a group open */
  if (pFile->iCurrentG == 0) {
    strcpy(pName, "root");
    strcpy(pClass, "NXroot");
    pFile->iNX = 0;
    grp = H5Gopen(pFile->iFID, "/", H5P_DEFAULT);
    H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, 0, group_info, &pFile->iNX);
    H5Gclose(grp);
    *iN = pFile->iNX;
  } else {
    strcpy(pName, pFile->name_ref);
    attr_id = H5Aopen_by_name(pFile->iCurrentG, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
      strcpy(pClass, NX_UNKNOWN_GROUP);
    } else {
      atype = H5Tcopy(H5T_C_S1);
      char data[64];
      H5Tset_size(atype, sizeof(data));
      readStringAttributeN(attr_id, data, sizeof(data));
      strcpy(pClass, data);
      pFile->iNX = 0;
      grp = H5Gopen(pFile->iFID, pFile->name_ref, H5P_DEFAULT);
      H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, 0, group_info, &pFile->iNX);
      H5Gclose(grp);
      *iN = pFile->iNX;
      H5Aclose(attr_id);
    }
  }
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------------*/
static int countObjectsInGroup(hid_t loc_id) {
  int count = 0;
  H5G_info_t numobj;

  herr_t status;

  status = H5Gget_info(loc_id, &numobj);
  if (status < 0) {
    NXReportError("Internal error, failed to retrieve no of objects");
    return 0;
  }

  count = (int)numobj.nlinks;
  return count;
}

/*----------------------------------------------------------------------------*/
NXstatus NX5getgroupinfo(NXhandle fid, int *iN, NXname pName, NXname pClass) {
  pNexusFile5 pFile;
  hid_t atype, attr_id, gid;

  pFile = NXI5assert(fid);
  /* check if there is a group open */
  if (pFile->iCurrentG == 0) {
    strcpy(pName, "root");
    strcpy(pClass, "NXroot");
    gid = H5Gopen(pFile->iFID, "/", H5P_DEFAULT);
    *iN = countObjectsInGroup(gid);
    H5Gclose(gid);
  } else {
    strcpy(pName, pFile->name_ref);
    attr_id = H5Aopen_by_name(pFile->iCurrentG, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
      strcpy(pClass, NX_UNKNOWN_GROUP);
    } else {
      atype = H5Tcopy(H5T_C_S1);
      char data[64];
      H5Tset_size(atype, sizeof(data));
      readStringAttributeN(attr_id, data, sizeof(data));
      strcpy(pClass, data);
      H5Aclose(attr_id);
    }
    pFile->iNX = 0;
    *iN = countObjectsInGroup(pFile->iCurrentG);
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------
 * Function: hdf5ToNXType
 *
 * Purpose:	Convert a HDF5 class to a NeXus type;  it handles the following HDF5 classes
 *  H5T_STRING
 *  H5T_INTEGER
 *  H5T_FLOAT
 *
 * Return: the NeXus type
 *
 *-------------------------------------------------------------------------
 */
static int hdf5ToNXType(H5T_class_t tclass, hid_t atype) {
  int iPtype = -1;
  size_t size;
  H5T_sign_t sign;

  if (tclass == H5T_STRING) {
    iPtype = NX_CHAR;
  } else if (tclass == H5T_INTEGER) {
    size = H5Tget_size(atype);
    sign = H5Tget_sign(atype);
    if (size == 1) {
      if (sign == H5T_SGN_2) {
        iPtype = NX_INT8;
      } else {
        iPtype = NX_UINT8;
      }
    } else if (size == 2) {
      if (sign == H5T_SGN_2) {
        iPtype = NX_INT16;
      } else {
        iPtype = NX_UINT16;
      }
    } else if (size == 4) {
      if (sign == H5T_SGN_2) {
        iPtype = NX_INT32;
      } else {
        iPtype = NX_UINT32;
      }
    } else if (size == 8) {
      if (sign == H5T_SGN_2) {
        iPtype = NX_INT64;
      } else {
        iPtype = NX_UINT64;
      }
    }
  } else if (tclass == H5T_FLOAT) {
    size = H5Tget_size(atype);
    if (size == 4) {
      iPtype = NX_FLOAT32;
    } else if (size == 8) {
      iPtype = NX_FLOAT64;
    }
  }
  if (iPtype == -1) {
    char message[80];
    snprintf(message, 79, "ERROR: hdf5ToNXtype: invalid type (%d)", tclass);
    NXReportError(message);
  }

  return iPtype;
}

/*--------------------------------------------------------------------------*/
static hid_t h5MemType(hid_t atype) {
  hid_t memtype_id = -1;
  size_t size;
  H5T_sign_t sign;
  H5T_class_t tclass;

  tclass = H5Tget_class(atype);

  if (tclass == H5T_INTEGER) {
    size = H5Tget_size(atype);
    sign = H5Tget_sign(atype);
    if (size == 1) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT8;
      } else {
        memtype_id = H5T_NATIVE_UINT8;
      }
    } else if (size == 2) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT16;
      } else {
        memtype_id = H5T_NATIVE_UINT16;
      }
    } else if (size == 4) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT32;
      } else {
        memtype_id = H5T_NATIVE_UINT32;
      }
    } else if (size == 8) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT64;
      } else {
        memtype_id = H5T_NATIVE_UINT64;
      }
    }
  } else if (tclass == H5T_FLOAT) {
    size = H5Tget_size(atype);
    if (size == 4) {
      memtype_id = H5T_NATIVE_FLOAT;
    } else if (size == 8) {
      memtype_id = H5T_NATIVE_DOUBLE;
    }
  }
  if (memtype_id == -1) {
    NXReportError("ERROR: h5MemType: invalid type");
  }
  return memtype_id;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getnextentry(NXhandle fid, NXname name, NXname nxclass, NXnumtype *datatype) {
  pNexusFile5 pFile;
  hid_t grp, attr1, type, atype;
  herr_t iRet;
  hsize_t idx;
  H5T_class_t tclass;
  info_type op_data;
  herr_t iRet_iNX = -1;

  pFile = NXI5assert(fid);
  op_data.iname = NULL;

  /*
     iterate to next entry in group list
   */
  idx = pFile->iStack5[pFile->iStackPtr].iCurrentIDX;
  if (strlen(pFile->name_ref) == 0) {
    /* root group */
    strcpy(pFile->name_ref, "/");
  }
  grp = H5Gopen(pFile->iFID, pFile->name_ref, H5P_DEFAULT);
  // index can be wrong here
  iRet = H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, &idx, nxgroup_info, &op_data);
  H5Gclose(grp);
  strcpy(nxclass, NX_UNKNOWN_GROUP);

  /*
     figure out the number of items in the current group. We need this in order to
     find out if we are at the end of the search.
   */
  if (pFile->iCurrentG == 0) {
    // if pFile->iCurrentG == 0 would not pFile->name_ref be "/" already, so we could skip that if statement ?
    pFile->iNX = 0;
    grp = H5Gopen(pFile->iFID, "/", H5P_DEFAULT);
    iRet_iNX = H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, 0, group_info, &pFile->iNX);
    H5Gclose(grp);
  } else {
    pFile->iNX = 0;
    grp = H5Gopen(pFile->iFID, pFile->name_ref, H5P_DEFAULT);
    // index can be wrong here
    iRet_iNX = H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, 0, group_info, &pFile->iNX);
    H5Gclose(grp);
  }
  if (idx == static_cast<hsize_t>(pFile->iNX)) {
    // why 2?
    iRet_iNX = 2;
  }

  if (iRet > 0) {
    pFile->iStack5[pFile->iStackPtr].iCurrentIDX++;
    if (op_data.iname != NULL) {
      strcpy(name, op_data.iname);
      free(op_data.iname);
    } else {
      pFile->iStack5[pFile->iStackPtr].iCurrentIDX = 0;
      return NXstatus::NX_EOD;
    }
    if (op_data.type == H5O_TYPE_GROUP) {
      /*
         open group and find class name attribute
       */
      size_t const Nbuff(2048), Nname(1024);
      char ph_name[Nname];
      strcpy(ph_name, "");
      for (int i = 1; i < (pFile->iStackPtr + 1); i++) {
        strcat(ph_name, pFile->iStack5[i].irefn);
        strcat(ph_name, "/");
      }
      strcat(ph_name, name);
      grp = H5Gopen(pFile->iFID, ph_name, H5P_DEFAULT);
      if (grp < 0) {
        char pBuffer[Nbuff];
        snprintf(pBuffer, Nbuff, "ERROR: group %s does not exist", ph_name);
        NXReportError(pBuffer);
        return NXstatus::NX_ERROR;
      }
      attr1 = H5Aopen_by_name(grp, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
      if (attr1 < 0) {
        strcpy(nxclass, NX_UNKNOWN_GROUP);
      } else {
        type = H5T_C_S1;
        atype = H5Tcopy(type);
        char data[128];
        H5Tset_size(atype, sizeof(data));
        if (readStringAttributeN(attr1, data, sizeof(data)) < 0) {
          char pBuffer[Nbuff];
          snprintf(pBuffer, Nbuff, "ERROR: reading attribute");
          NXReportError(pBuffer);
          return NXstatus::NX_ERROR;
        }
        strcpy(nxclass, data);
        H5Tclose(atype);
        H5Aclose(attr1);
      }
      H5Gclose(grp);
    } else if (op_data.type == H5O_TYPE_DATASET) {
      /*
         open dataset and find type
       */
      if (pFile->iCurrentG == 0) {
        grp = H5Dopen(pFile->iFID, name, H5P_DEFAULT);
      } else {
        grp = H5Dopen(pFile->iCurrentG, name, H5P_DEFAULT);
      }
      type = H5Dget_type(grp);
      atype = H5Tcopy(type);
      tclass = H5Tget_class(atype);
      int iPtype = hdf5ToNXType(tclass, atype);
      *datatype = static_cast<NXnumtype>(iPtype);
      strcpy(nxclass, "SDS");
      H5Tclose(atype);
      H5Tclose(type);
      H5Dclose(grp);
    }
    return NXstatus::NX_OK;
  } else {
    /*
       we are at the end of the search: clear the data structure and reset
       iCurrentIDX to 0
     */
    if (iRet_iNX == 2) {
      if (op_data.iname != NULL) {
        free(op_data.iname);
      }
      pFile->iStack5[pFile->iStackPtr].iCurrentIDX = 0;
      return NXstatus::NX_EOD;
    }
    if (op_data.iname != NULL) {
      free(op_data.iname);
    }
    NXReportError("ERROR: iterating through group not successful");
    return NXstatus::NX_ERROR;
  }
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getdata(NXhandle fid, void *data) {
  pNexusFile5 pFile;
  int iStart[H5S_MAX_RANK], status;
  hid_t memtype_id;
  H5T_class_t tclass;
  hsize_t ndims, dims[H5S_MAX_RANK];
  char **vstrdata = NULL;

  pFile = NXI5assert(fid);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    NXReportError("ERROR: no dataset open");
    return NXstatus::NX_ERROR;
  }
  ndims = static_cast<hsize_t>(H5Sget_simple_extent_dims(pFile->iCurrentS, dims, NULL));

  if (ndims == 0) { /* SCALAR dataset */
    hid_t datatype = H5Dget_type(pFile->iCurrentD);
    hid_t filespace = H5Dget_space(pFile->iCurrentD);

    tclass = H5Tget_class(datatype);

    if (H5Tis_variable_str(pFile->iCurrentT)) {
      char *strdata = static_cast<char *>(calloc(512, sizeof(char)));
      status = H5Dread(pFile->iCurrentD, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &strdata);
      if (status >= 0) {
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
        // there is a danger of overflowing output string
        std::strncpy(static_cast<char *>(data), strdata, strlen(strdata));
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic pop
#endif
      }
      free(strdata);
    } else {
      memtype_id = H5Screate(H5S_SCALAR);
      H5Sselect_all(filespace);
      status = H5Dread(pFile->iCurrentD, datatype, memtype_id, filespace, H5P_DEFAULT, data);
      H5Sclose(memtype_id);
    }

    H5Sclose(filespace);
    H5Tclose(datatype);
    if (status < 0)
      return NXstatus::NX_ERROR;
    return NXstatus::NX_OK;
  }

  memset(iStart, 0, H5S_MAX_RANK * sizeof(int));
  /* map datatypes of other plateforms */
  tclass = H5Tget_class(pFile->iCurrentT);
  if (H5Tis_variable_str(pFile->iCurrentT)) {
    vstrdata = static_cast<char **>(malloc((size_t)dims[0] * sizeof(char *)));
    memtype_id = H5Tcopy(H5T_C_S1);
    H5Tset_size(memtype_id, H5T_VARIABLE);
    status = H5Dread(pFile->iCurrentD, memtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, vstrdata);
    static_cast<char *>(data)[0] = '\0';
    if (status >= 0) {
      for (hsize_t i = 0; i < dims[0]; ++i) {
        if (vstrdata[i] != NULL) {
          strcat(static_cast<char *>(data), vstrdata[i]);
        }
      }
    }
    H5Dvlen_reclaim(memtype_id, pFile->iCurrentS, H5P_DEFAULT, vstrdata);
    free(vstrdata);
    H5Tclose(memtype_id);
  } else if (tclass == H5T_STRING) {
    status = H5Dread(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  } else {
    memtype_id = h5MemType(pFile->iCurrentT);
    status = H5Dread(pFile->iCurrentD, memtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  }
  if (status < 0) {
    NXReportError("ERROR: failed to transfer dataset");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getinfo64(NXhandle fid, int *rank, int64_t dimension[], NXnumtype *iType) {
  pNexusFile5 pFile;
  int i, iRank, mType;
  hsize_t myDim[H5S_MAX_RANK];
  H5T_class_t tclass;
  hid_t memType;
  char *vlData = NULL;

  pFile = NXI5assert(fid);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    NXReportError("ERROR: no dataset open");
    return NXstatus::NX_ERROR;
  }

  /* read information */
  tclass = H5Tget_class(pFile->iCurrentT);
  mType = hdf5ToNXType(tclass, pFile->iCurrentT);
  iRank = H5Sget_simple_extent_dims(pFile->iCurrentS, myDim, NULL);
  if (iRank == 0) {
    iRank = 1; /* we pretend */
    myDim[0] = 1;
  } else {
    H5Sget_simple_extent_dims(pFile->iCurrentS, myDim, NULL);
    hsize_t total_dims_size = 1;
    for (i = 0; i < iRank; ++i) {
      total_dims_size *= myDim[i];
    }
    UNUSED_ARG(total_dims_size);
  }
  /* conversion to proper ints for the platform */
  *iType = static_cast<NXnumtype>(mType);
  if (tclass == H5T_STRING && myDim[iRank - 1] == 1) {
    if (H5Tis_variable_str(pFile->iCurrentT)) {
      /* this will not work for arrays of strings */
      memType = H5Tcopy(H5T_C_S1);
      H5Tset_size(memType, H5T_VARIABLE);
      H5Dread(pFile->iCurrentD, memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vlData);
      if (vlData != NULL) {
        myDim[iRank - 1] = strlen(vlData) + 1;
        H5Dvlen_reclaim(memType, pFile->iCurrentS, H5P_DEFAULT, &vlData);
      }
      H5Tclose(memType);
    } else {
      myDim[iRank - 1] = H5Tget_size(pFile->iCurrentT);
    }
  }
  *rank = (int)iRank;
  for (i = 0; i < iRank; i++) {
    dimension[i] = (int64_t)myDim[i];
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getslab64(NXhandle fid, void *data, const int64_t iStart[], const int64_t iSize[]) {
  pNexusFile5 pFile;
  hsize_t myStart[H5S_MAX_RANK];
  hsize_t mySize[H5S_MAX_RANK];
  hsize_t mStart[H5S_MAX_RANK];
  hid_t memspace, iRet;
  H5T_class_t tclass;
  hid_t memtype_id;
  char *tmp_data = NULL;
  int iRank;

  pFile = NXI5assert(fid);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    NXReportError("ERROR: no dataset open");
    return NXstatus::NX_ERROR;
  }
  tclass = H5Tget_class(pFile->iCurrentT);
  /* map datatypes of other platforms */
  if (tclass == H5T_STRING) {
    memtype_id = pFile->iCurrentT;
  } else {
    memtype_id = h5MemType(pFile->iCurrentT);
  }

  iRank = H5Sget_simple_extent_ndims(pFile->iCurrentS);

  if (iRank == 0) {
    /* this is an unslabbale SCALAR */
    hid_t filespace = H5Dget_space(pFile->iCurrentD);
    memspace = H5Screate(H5S_SCALAR);
    H5Sselect_all(filespace);
    iRet = H5Dread(pFile->iCurrentD, memtype_id, memspace, filespace, H5P_DEFAULT, data);
    H5Sclose(filespace);
  } else {

    for (int i = 0; i < iRank; i++) {
      myStart[i] = static_cast<hsize_t>(iStart[i]);
      mySize[i] = static_cast<hsize_t>(iSize[i]);
      mStart[i] = static_cast<hsize_t>(0);
    }

    /*
     * this does not work for multidimensional string arrays.
     */
    int mtype = 0;
    if (tclass == H5T_STRING) {
      mtype = NX_CHAR;
      if (mySize[0] == 1) {
        mySize[0] = H5Tget_size(pFile->iCurrentT);
      }
      tmp_data = static_cast<char *>(malloc((size_t)mySize[0]));
      memset(tmp_data, 0, sizeof(mySize[0]));
      iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, mStart, NULL, mySize, NULL);
    } else {
      iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, myStart, NULL, mySize, NULL);
    }
    /* define slab */
    /* deal with HDF errors */
    if (iRet < 0) {
      NXReportError("ERROR: selecting slab failed");
      return NXstatus::NX_ERROR;
    }

    memspace = H5Screate_simple(iRank, mySize, NULL);
    iRet = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, mStart, NULL, mySize, NULL);
    if (iRet < 0) {
      NXReportError("ERROR: selecting memspace failed");
      return NXstatus::NX_ERROR;
    }
    /* read slab */
    if (mtype == NX_CHAR) {
      iRet = H5Dread(pFile->iCurrentD, memtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, tmp_data);
      char const *data1;
      data1 = tmp_data + myStart[0];
      strncpy(static_cast<char *>(data), data1, (size_t)iSize[0]);
      free(tmp_data);
    } else {
      iRet = H5Dread(pFile->iCurrentD, memtype_id, memspace, pFile->iCurrentS, H5P_DEFAULT, data);
    }
  }
  /* cleanup */
  H5Sclose(memspace);
  H5Tclose(tclass);

  if (iRet < 0) {
    NXReportError("ERROR: reading slab failed");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

/* Operator function. */

herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *unused, void *opdata) {
  UNUSED_ARG(loc_id);
  UNUSED_ARG(unused);
  *(static_cast<char **>(opdata)) = strdup(name);

  return 1;
}

NXstatus NX5getnextattr(NXhandle fileid, NXname pName, int *iLength, NXnumtype *iType) {
  int rank;
  NXstatus status;
  int mydim[H5S_MAX_RANK];

  status = NX5getnextattra(fileid, pName, &rank, mydim, iType);

  if (status != NXstatus::NX_OK)
    return status;

  if (rank == 0 || (rank == 1 && mydim[0] == 1)) {
    *iLength = 1;
    return NXstatus::NX_OK;
  }

  if (rank == 1 && *iType == NXnumtype::CHAR) {
    *iLength = mydim[0];
    return NXstatus::NX_OK;
  }

  NXReportError("ERROR iterating through attributes found array attribute not understood by this api");
  return NXstatus::NX_ERROR;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getattr(NXhandle fid, const char *name, void *data, int *datalen, NXnumtype *iType) {
  pNexusFile5 pFile;
  hid_t vid, iNew;
  hsize_t dims[H5S_MAX_RANK], totalsize;
  herr_t iRet;
  hid_t type, filespace;
  char pBuffer[256];

  pFile = NXI5assert(fid);

  type = nxToHDF5Type(*iType);

  vid = getAttVID(pFile);
  iNew = H5Aopen_by_name(vid, ".", name, H5P_DEFAULT, H5P_DEFAULT);
  if (iNew < 0) {
    sprintf(pBuffer, "ERROR: attribute \"%s\" not found", name);
    killAttVID(pFile, vid);
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }
  pFile->iCurrentA = iNew;
  filespace = H5Aget_space(pFile->iCurrentA);
  totalsize = 1;
  const auto ndims = H5Sget_simple_extent_dims(filespace, dims, NULL);
  for (int i = 0; i < ndims; i++) {
    totalsize *= dims[i];
  }
  if (ndims != 0 && totalsize > 1) {
    NXReportError("ERROR: attribute arrays not supported by this api");
    return NXstatus::NX_ERROR;
  }

  /* finally read the data */
  if (type == H5T_C_S1) {
    iRet = readStringAttributeN(pFile->iCurrentA, static_cast<char *>(data), *datalen);
    *datalen = static_cast<int>(strlen(static_cast<char *>(data)));
  } else {
    iRet = H5Aread(pFile->iCurrentA, type, data);
    *datalen = 1;
  }

  if (iRet < 0) {
    sprintf(pBuffer, "ERROR: could not read attribute data for \"%s\"", name);
    NXReportError(pBuffer);
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }

  H5Aclose(pFile->iCurrentA);

  killAttVID(pFile, vid);
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getattrinfo(NXhandle fid, int *iN) {
  pNexusFile5 pFile;
  int idx;
  hid_t vid;
  H5O_info1_t oinfo;

  pFile = NXI5assert(fid);
  idx = 0;
  *iN = idx;

  vid = getAttVID(pFile);

  // TODO use new version of method rather than v1
  H5Oget_info1(vid, &oinfo);
  idx = static_cast<int>(oinfo.num_attrs);
  if (idx > 0) {
    if (pFile->iCurrentG > 0 && pFile->iCurrentD == 0) {
      *iN = idx - 1;
    } else {
      *iN = idx;
    }
  } else {
    *iN = 0;
  }
  killAttVID(pFile, vid);
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/
NXstatus NX5getgroupID(NXhandle fileid, NXlink *sRes) {
  pNexusFile5 pFile;
  NXnumtype type = NXnumtype::CHAR;

  pFile = NXI5assert(fileid);
  if (pFile->iCurrentG == 0) {
    return NXstatus::NX_ERROR;
  } else {
    /*
       this means: if the item is already linked: use the target attribute, else
       the path to the current node
     */
    NXMDisableErrorReporting();
    int datalen = 1024;
    memset(sRes->targetPath, 0, static_cast<size_t>(datalen) * sizeof(char));
    if (NX5getattr(fileid, "target", sRes->targetPath, &datalen, &type) != NXstatus::NX_OK) {
      buildCurrentPath(pFile, sRes->targetPath, datalen);
    }
    NXMEnableErrorReporting();
    sRes->linkType = 0;
    return NXstatus::NX_OK;
  }
  /* not reached */
  return NXstatus::NX_ERROR;
}

/* ------------------------------------------------------------------- */

NXstatus NX5nativeexternallink(NXhandle fileid, const char *name, const char *externalfile, const char *remotetarget) {
  hid_t openwhere;

  const pNexusFile5 pFile = NXI5assert(fileid);

  if (pFile->iCurrentG <= 0) {
    openwhere = pFile->iFID;
  } else {
    openwhere = pFile->iCurrentG;
  }

  const herr_t iRet = H5Lcreate_external(externalfile, remotetarget, openwhere, name, H5P_DEFAULT, H5P_DEFAULT);
  if (iRet < 0) {
    NXReportError("ERROR: making external link failed");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5nativeinquirefile(NXhandle fileid, char *externalfile, const int filenamelen) {
  hid_t openthing;

  const pNexusFile5 pFile = NXI5assert(fileid);
  if (pFile->iCurrentD > 0) {
    openthing = pFile->iCurrentD;
  } else if (pFile->iCurrentG > 0) {
    openthing = pFile->iCurrentG;
  } else {
    openthing = pFile->iFID;
  }

  // Check for failure again
  if (H5Fget_name(openthing, externalfile, static_cast<size_t>(filenamelen)) < 0) {
    NXReportError("ERROR: retrieving file name");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5nativeisexternallink(NXhandle fileid, const char *name, char *url, const int urllen) {
  pNexusFile5 pFile; // cppcheck-suppress constVariablePointer
  herr_t ret;
  H5L_info_t link_buff;
  char linkval_buff[NX_MAXPATHLEN];
  const char *filepath = NULL, *objpath = NULL;
  size_t val_size;
  hid_t openthing;

  pFile = NXI5assert(fileid);
  memset(url, 0, static_cast<size_t>(urllen));

  if (pFile->iCurrentG > 0) {
    openthing = pFile->iCurrentG;
  } else {
    openthing = pFile->iFID;
  }

  ret = H5Lget_info(openthing, name, &link_buff, H5P_DEFAULT);
  if (ret < 0 || link_buff.type != H5L_TYPE_EXTERNAL) {
    return NXstatus::NX_ERROR;
  }

  val_size = link_buff.u.val_size;
  if (val_size > sizeof(linkval_buff)) {
    NXReportError("ERROR: linkval_buff too small");
    return NXstatus::NX_ERROR;
  }

  ret = H5Lget_val(openthing, name, linkval_buff, val_size, H5P_DEFAULT);
  if (ret < 0) {
    NXReportError("ERROR: H5Lget_val failed");
    return NXstatus::NX_ERROR;
  }

  ret = H5Lunpack_elink_val(linkval_buff, val_size, NULL, &filepath, &objpath);
  if (ret < 0) {
    NXReportError("ERROR: H5Lunpack_elink_val failed");
    return NXstatus::NX_ERROR;
  }

  snprintf(url, static_cast<size_t>(urllen - 1), "nxfile://%s#%s", filepath, objpath);
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NX5sameID(NXhandle fileid, NXlink const *pFirstID, NXlink const *pSecondID) {
  NXI5assert(fileid);
  if ((strcmp(pFirstID->targetPath, pSecondID->targetPath) == 0)) {
    return NXstatus::NX_OK;
  } else {
    return NXstatus::NX_ERROR;
  }
}

/*-------------------------------------------------------------------------*/

NXstatus NX5initattrdir(NXhandle fid) {
  pNexusFile5 pFile;

  pFile = NXI5assert(fid);
  NXI5KillAttDir(pFile);
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5initgroupdir(NXhandle fid) {
  pNexusFile5 pFile;

  pFile = NXI5assert(fid);
  NXI5KillDir(pFile);
  return NXstatus::NX_OK;
}

/*------------------------------------------------------------------------*/
NXstatus NX5putattra(NXhandle handle, CONSTCHAR *name, const void *data, const int rank, const int dim[],
                     const NXnumtype iType) {
  hid_t datatype1, dataspace;
  hid_t type;
  pNexusFile5 pFile;
  int i;
  hid_t vid, iATT;
  herr_t iRet;
  hsize_t mydim[H5S_MAX_RANK];

  pFile = NXI5assert(handle);

  for (i = 0; i < rank; i++) {
    mydim[i] = static_cast<hsize_t>(dim[i]);
  }

  /* determine vid */
  vid = getAttVID(pFile);
  iATT = H5Aopen_by_name(vid, ".", name, H5P_DEFAULT, H5P_DEFAULT);
  if (iATT > 0) {
    H5Aclose(iATT);
    iRet = H5Adelete(vid, name);
    if (iRet < 0) {
      NXReportError("ERROR: old attribute cannot be removed! ");
      killAttVID(pFile, vid);
      return NXstatus::NX_ERROR;
    }
  }

  if (rank < 0) {
    char pBuffer[256];
    sprintf(pBuffer, "ERROR: invalid rank specified %s", name);
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }

  type = nxToHDF5Type(iType);

  datatype1 = H5Tcopy(type);
  if (iType == NXnumtype::CHAR) {
    H5Tset_size(datatype1, static_cast<size_t>(dim[rank - 1]));
    dataspace = H5Screate_simple(rank - 1, mydim, NULL);
  } else {
    dataspace = H5Screate_simple(rank, mydim, NULL);
  }

  iATT = H5Acreate(vid, static_cast<const char *>(name), datatype1, dataspace, H5P_DEFAULT, H5P_DEFAULT);

  if (iATT < 0) {
    NXReportError("ERROR: creating attribute failed");
    return NXstatus::NX_ERROR;
  } else {
    pFile->iCurrentA = iATT;
  }

  iRet = H5Awrite(pFile->iCurrentA, datatype1, data);
  if (iRet < 0) {
    NXReportError("ERROR: failure to write attribute");
    return NXstatus::NX_ERROR;
  }

  // less than zero is an error so just accumulate them
  iRet += H5Sclose(dataspace);
  iRet += H5Tclose(datatype1);
  iRet += H5Aclose(pFile->iCurrentA);
  pFile->iCurrentA = 0;
  if (iRet < 0) {
    NXReportError("ERROR: HDF cannot close attribute");
    return NXstatus::NX_ERROR;
  }

  return NXstatus::NX_OK;
}

/*------------------------------------------------------------------------*/
NXstatus NX5getnextattra(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType) {
  pNexusFile5 pFile;
  herr_t iRet;
  char *iname = NULL;
  hid_t vid;
  H5O_info1_t oinfo;

  pFile = NXI5assert(handle);

  vid = getAttVID(pFile);

  pName[0] = '\0';
  hsize_t idx = pFile->iAtt5.iCurrentIDX;
  iRet = 0;

  // TODO use new version of method rather than v2
  H5Oget_info2(vid, &oinfo, H5O_INFO_ALL);
  hsize_t intern_idx = oinfo.num_attrs;
  if (intern_idx == idx) {
    killAttVID(pFile, vid);
    return NXstatus::NX_EOD;
  }

  if (intern_idx > idx) {
    iRet = H5Aiterate(vid, H5_INDEX_CRT_ORDER, H5_ITER_INC, &idx, attr_info, &iname);
  } else {
    iRet = 0;
  }
  if (iRet < 0) {
    NXReportError("ERROR: iterating through attribute list");
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }
  pFile->iAtt5.iCurrentIDX++;
  if (iname != NULL) {
    if (strcmp(iname, "NX_class") == 0 && pFile->iCurrentG != 0 && pFile->iCurrentD == 0) {
      /*
         skip NXclass attribute which is internal
       */
      free(iname);
      iname = NULL;
      killAttVID(pFile, vid);
      return NX5getnextattra(handle, pName, rank, dim, iType);
    }
    strcpy(pName, iname);
    free(iname);
    iname = NULL;
  } else {
    NXReportError("ERROR: encountered nameless attribute");
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }

  killAttVID(pFile, vid);
  return NX5getattrainfo(handle, pName, rank, dim, iType);
}
/*------------------------------------------------------------------------*/
NXstatus NX5getattra(NXhandle handle, const char *name, void *data) {
  pNexusFile5 pFile;
  int iStart[H5S_MAX_RANK], status;
  hid_t vid;
  hid_t memtype_id, filespace, datatype;
  H5T_class_t tclass;
  hsize_t dims[H5S_MAX_RANK];
  htri_t is_vlen_str = 0; /* false */
  char **vstrdata = NULL;

  pFile = NXI5assert(handle);

  vid = getAttVID(pFile);
  pFile->iCurrentA = H5Aopen_by_name(vid, ".", name, H5P_DEFAULT, H5P_DEFAULT);
  if (pFile->iCurrentA < 0) {
    pFile->iCurrentA = 0;
    NXReportError("ERROR: unable to open attribute");
    return NXstatus::NX_ERROR;
  }
  filespace = H5Aget_space(pFile->iCurrentA);
  datatype = H5Aget_type(pFile->iCurrentA);
  auto ndims = H5Sget_simple_extent_dims(filespace, dims, NULL);

  is_vlen_str = H5Tis_variable_str(datatype);
  if (ndims == 0 && is_vlen_str) {
    /* this assumes a fixed size - is this dangerous? */
    char *strdata = static_cast<char *>(calloc(512, sizeof(char)));
    status = H5Aread(pFile->iCurrentA, H5S_ALL, &strdata);
    if (status >= 0) {
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
      std::strncpy(static_cast<char *>(data), strdata, strlen(strdata));
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic pop
#endif
    }
    free(strdata);

    H5Sclose(filespace);
    H5Tclose(datatype);
    if (status < 0)
      return NXstatus::NX_ERROR;
    return NXstatus::NX_OK;
  }
  tclass = H5Tget_class(datatype);
  /* stop gap kludge for fixed length strings */
  if (tclass == H5T_STRING && !is_vlen_str) {
    char *datatmp = NULL;
    status = readStringAttribute(pFile->iCurrentA, &datatmp);
    if (status < 0)
      return NXstatus::NX_ERROR;
    strcpy(static_cast<char *>(data), datatmp);
    free(datatmp);
    return NXstatus::NX_OK;
  }

  memset(iStart, 0, H5S_MAX_RANK * sizeof(int));
  /* map datatypes of other plateforms */
  if (is_vlen_str) {
    vstrdata = static_cast<char **>(malloc((size_t)dims[0] * sizeof(char *)));
    memtype_id = H5Tcopy(H5T_C_S1);
    H5Tset_size(memtype_id, H5T_VARIABLE);
    status = H5Aread(pFile->iCurrentA, memtype_id, vstrdata);
    (static_cast<char *>(data))[0] = '\0';
    if (status >= 0) {
      for (hsize_t i = 0; i < dims[0]; ++i) {
        if (vstrdata[i] != NULL) {
          strcat(static_cast<char *>(data), vstrdata[i]);
        }
      }
    }
    H5Dvlen_reclaim(memtype_id, pFile->iCurrentS, H5P_DEFAULT, vstrdata);
    free(vstrdata);
    H5Tclose(memtype_id);
  } else if (tclass == H5T_STRING) {
    status = H5Aread(pFile->iCurrentA, datatype, data);
  } else {
    memtype_id = h5MemType(datatype);
    status = H5Aread(pFile->iCurrentA, memtype_id, data);
  }
  if (status < 0) {
    NXReportError("ERROR: failed to read attribute");
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}
/*------------------------------------------------------------------------*/
NXstatus NX5getattrainfo(NXhandle handle, NXname name, int *rank, int dim[], NXnumtype *iType) {
  pNexusFile5 pFile;
  int iRet, mType;
  hid_t vid;
  hid_t filespace, attrt, memtype;
  hsize_t myDim[H5S_MAX_RANK];
  H5T_class_t tclass;
  char *vlStr = NULL;

  pFile = NXI5assert(handle);

  vid = getAttVID(pFile);
  pFile->iCurrentA = H5Aopen_by_name(vid, ".", name, H5P_DEFAULT, H5P_DEFAULT);
  if (pFile->iCurrentA < 0) {
    pFile->iCurrentA = 0;
    NXReportError("ERROR: unable to open attribute");
    return NXstatus::NX_ERROR;
  }

  filespace = H5Aget_space(pFile->iCurrentA);
  auto myrank = H5Sget_simple_extent_ndims(filespace);
  iRet = H5Sget_simple_extent_dims(filespace, myDim, NULL);
  if (iRet < 0) {
    NXReportError("ERROR: Cannot determine attribute dimensions");
    return NXstatus::NX_ERROR;
  }

  /* read information */
  attrt = H5Aget_type(pFile->iCurrentA);
  tclass = H5Tget_class(attrt);
  mType = hdf5ToNXType(tclass, attrt);

  /* conversion to proper ints for the platform */
  *iType = static_cast<NXnumtype>(mType);

  if (tclass == H5T_STRING) {
    myrank++;
    if (H5Tis_variable_str(attrt)) {
      memtype = H5Tcopy(H5T_C_S1);
      H5Tset_size(memtype, H5T_VARIABLE);
      H5Aread(pFile->iCurrentA, memtype, &vlStr);
      if (vlStr != NULL) {
        myDim[myrank - 1] = strlen(vlStr) + 1;
        H5Dvlen_reclaim(memtype, pFile->iCurrentA, H5P_DEFAULT, &vlStr);
      }
      H5Tclose(memtype);
    } else {
      myDim[myrank - 1] = H5Tget_size(attrt);
    }
  } else if (myrank == 0) {
    myrank = 1; /* we pretend */
    myDim[0] = 1;
  }

  for (int i = 0; i < myrank; i++) {
    dim[i] = static_cast<int>(myDim[i]);
  }
  *rank = static_cast<int>(myrank);

  return NXstatus::NX_OK;
}

/*------------------------------------------------------------------------*/
void NX5assignFunctions(pNexusFunction fHandle) {
  fHandle->nxclose = NX5close;
  fHandle->nxreopen = NX5reopen;
  fHandle->nxflush = NX5flush;
  fHandle->nxmakegroup = NX5makegroup;
  fHandle->nxopengroup = NX5opengroup;
  fHandle->nxclosegroup = NX5closegroup;
  fHandle->nxmakedata64 = NX5makedata64;
  fHandle->nxcompmakedata64 = NX5compmakedata64;
  fHandle->nxcompress = NX5compress;
  fHandle->nxopendata = NX5opendata;
  fHandle->nxclosedata = NX5closedata;
  fHandle->nxputdata = NX5putdata;
  fHandle->nxputattr = NX5putattr;
  fHandle->nxputslab64 = NX5putslab64;
  fHandle->nxgetdataID = NX5getdataID;
  fHandle->nxmakelink = NX5makelink;
  fHandle->nxmakenamedlink = NX5makenamedlink;
  fHandle->nxgetdata = NX5getdata;
  fHandle->nxgetinfo64 = NX5getinfo64;
  fHandle->nxgetnextentry = NX5getnextentry;
  fHandle->nxgetslab64 = NX5getslab64;
  fHandle->nxgetnextattr = NX5getnextattr;
  fHandle->nxgetattr = NX5getattr;
  fHandle->nxgetattrinfo = NX5getattrinfo;
  fHandle->nxgetgroupID = NX5getgroupID;
  fHandle->nxgetgroupinfo = NX5getgroupinfo;
  fHandle->nxsameID = NX5sameID;
  fHandle->nxinitgroupdir = NX5initgroupdir;
  fHandle->nxinitattrdir = NX5initattrdir;
  fHandle->nxprintlink = NX5printlink;
  fHandle->nxnativeexternallink = NX5nativeexternallink;
  fHandle->nxnativeinquirefile = NX5nativeinquirefile;
  fHandle->nxnativeisexternallink = NX5nativeisexternallink;
  fHandle->nxputattra = NX5putattra;
  fHandle->nxgetnextattra = NX5getnextattra;
  fHandle->nxgetattra = NX5getattra;
  fHandle->nxgetattrainfo = NX5getattrainfo;
}

#endif /* HDF5 */
