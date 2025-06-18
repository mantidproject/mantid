#include "MantidNexus/napi_helper.h"
#include <assert.h>
#include <cstring>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <time.h>

pNexusFile5 NXI5assert(NXhandle fid) {
  pNexusFile5 pRes;

  assert(fid != NULL);
  pRes = static_cast<pNexusFile5>(fid);
  assert(pRes->iNXID == NX5SIGNATURE);
  return pRes;
}

void NXI5KillDir(pNexusFile5 self) { self->iStack5[self->iStackPtr].iCurrentIDX = 0; }

herr_t readStringAttribute(hid_t attr, char **data) {
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

herr_t readStringAttributeN(hid_t attr, char *data, int maxlen) {
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

void NXI5KillAttDir(pNexusFile5 self) { self->iCurrentIDX = 0; }

std::string buildCurrentAddress(pNexusFile5 fid) {
  hid_t current;
  if (fid->iCurrentD != 0) {
    current = fid->iCurrentD;
  } else if (fid->iCurrentG != 0) {
    current = fid->iCurrentG;
  } else {
    current = fid->iFID;
  }
  char caddr[2048];
  H5Iget_name(current, caddr, 2048);
  return std::string(caddr);
}

hid_t getAttVID(pNexusFile5 pFile) {
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

void killAttVID(const pNexusFile5 pFile, hid_t vid) {
  if (pFile->iCurrentG == 0 && pFile->iCurrentD == 0) {
    H5Gclose(vid);
  }
}

NXstatus NX5settargetattribute(pNexusFile5 pFile, NXlink *sLink) {
  hid_t dataID, aid2, aid1, attID;
  char name[] = "target";

  /*
     set the target attribute
   */
  if (sLink->linkType > 0) {
    dataID = H5Dopen(pFile->iFID, sLink->targetAddress.c_str(), H5P_DEFAULT);
  } else {
    dataID = H5Gopen(pFile->iFID, sLink->targetAddress.c_str(), H5P_DEFAULT);
  }
  if (dataID < 0) {
    NXReportError("Internal error, address to link does not exist");
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
  H5Tset_size(aid1, sLink->targetAddress.size());
  attID = H5Acreate(dataID, name, aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attID < 0) {
    return NXstatus::NX_OK;
  }
  UNUSED_ARG(H5Awrite(attID, aid1, sLink->targetAddress.c_str()));
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

int countObjectsInGroup(hid_t loc_id) {
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

NXnumtype hdf5ToNXType(H5T_class_t tclass, hid_t atype) {
  NXnumtype iPtype = NXnumtype::BAD;
  size_t size;
  H5T_sign_t sign;

  if (tclass == H5T_STRING) {
    iPtype = NXnumtype::CHAR;
  } else if (tclass == H5T_INTEGER) {
    size = H5Tget_size(atype);
    sign = H5Tget_sign(atype);
    if (size == 1) {
      if (sign == H5T_SGN_2) {
        iPtype = NXnumtype::INT8;
      } else {
        iPtype = NXnumtype::UINT8;
      }
    } else if (size == 2) {
      if (sign == H5T_SGN_2) {
        iPtype = NXnumtype::INT16;
      } else {
        iPtype = NXnumtype::UINT16;
      }
    } else if (size == 4) {
      if (sign == H5T_SGN_2) {
        iPtype = NXnumtype::INT32;
      } else {
        iPtype = NXnumtype::UINT32;
      }
    } else if (size == 8) {
      if (sign == H5T_SGN_2) {
        iPtype = NXnumtype::INT64;
      } else {
        iPtype = NXnumtype::UINT64;
      }
    }
  } else if (tclass == H5T_FLOAT) {
    size = H5Tget_size(atype);
    if (size == 4) {
      iPtype = NXnumtype::FLOAT32;
    } else if (size == 8) {
      iPtype = NXnumtype::FLOAT64;
    }
  }
  if (iPtype == NXnumtype::BAD) {
    char message[80];
    snprintf(message, 79, "ERROR: hdf5ToNXtype: invalid type (%d)", tclass);
    NXReportError(message);
  }

  return iPtype;
}

hid_t h5MemType(hid_t atype) {
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

herr_t attr_check(hid_t loc_id, const char *member_name, const H5A_info_t *unused, void *opdata) {
  UNUSED_ARG(loc_id);
  UNUSED_ARG(unused);
  UNUSED_ARG(opdata);
  char attr_name[8 + 1]; /* need to leave space for \0 as well */

  strcpy(attr_name, "NX_class");
  return strstr(member_name, attr_name) ? 1 : 0;
}

/*------------------------------------------------------------------------
  Implementation of NXopenaddress
  The below methods all help NXopenaddress move around inside an address stack
  --------------------------------------------------------------------------*/

int isDataSetOpen(NXhandle hfil) {
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
int isRoot(NXhandle hfil) {
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
char *extractNextAddress(char *address, NXname element) {
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
NXstatus gotoRoot(NXhandle hfil) {
  if (isDataSetOpen(hfil)) {
    NXstatus status = NXclosedata(hfil);
    if (status == NXstatus::NX_ERROR) {
      return status;
    }
  }
  while (!isRoot(hfil)) {
    NXclosegroup(hfil);
  }
  return NXstatus::NX_OK;
}

/*--------------------------------------------------------------------*/
int isRelative(char const *address) {
  if (address[0] == '.' && address[1] == '.')
    return 1;
  else
    return 0;
}

/*------------------------------------------------------------------*/
NXstatus moveOneDown(NXhandle hfil) {
  if (isDataSetOpen(hfil)) {
    return NXclosedata(hfil);
  } else {
    return NXclosegroup(hfil);
  }
}

/*-------------------------------------------------------------------
  returns a pointer to the remaining address string to move up
  --------------------------------------------------------------------*/
char *moveDown(NXhandle hfil, char *address, NXstatus *code) {
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
NXstatus stepOneUp(NXhandle hfil, char const *name) {
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
NXstatus stepOneGroupUp(NXhandle hfil, char const *name) {
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
