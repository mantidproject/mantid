#include "MantidNexus/napi_helper.h"
#include <assert.h>
#include <cstring>
#include <map>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <time.h>

pNexusFile5 NXI5assert(NXhandle fid) { return fid; }

void NXI5KillDir(pNexusFile5 self) { self->iStack5.back().iCurrentIDX = 0; }

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
    *data = nullptr;
    return static_cast<herr_t>(NXstatus::NX_ERROR);
  }

  H5Tclose(atype);
  H5Sclose(space);
  if (iRet < 0)
    return static_cast<herr_t>(NXstatus::NX_ERROR);
  return static_cast<herr_t>(NXstatus::NX_OK);
}

herr_t readStringAttributeN(hid_t attr, char *data, std::size_t maxlen) {
  herr_t iRet;
  char *vdat = NULL;
  iRet = readStringAttribute(attr, &vdat);
  if (iRet >= 0) {
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
    // there is a danger of overflowing output string
    std::strncpy(data, vdat, maxlen);
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

hid_t getAttVID(pNexusFile5 fid) {
  hid_t vid;
  if (fid->iCurrentD != 0) {
    /* dataset attribute */
    vid = fid->iCurrentD;
  } else if (fid->iCurrentG != 0) {
    /* group attribute */
    vid = fid->iCurrentG;
  } else {
    /* global attribute */
    vid = H5Gopen(fid->iFID, "/", H5P_DEFAULT);
  }
  return vid;
}

void killAttVID(const pNexusFile5 pFile, hid_t vid) {
  if (pFile->iCurrentG == 0 && pFile->iCurrentD == 0) {
    H5Gclose(vid);
  }
}

NXstatus NX5settargetattribute(pNexusFile5 pFile, NXlink const &sLink) {
  hid_t dataID, aid2, aid1, attID;
  char name[] = "target";

  /*
     set the target attribute
   */
  if (sLink.linkType == NXentrytype::sds) {
    dataID = H5Dopen(pFile->iFID, sLink.targetAddress.c_str(), H5P_DEFAULT);
  } else {
    dataID = H5Gopen(pFile->iFID, sLink.targetAddress.c_str(), H5P_DEFAULT);
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
  H5Tset_size(aid1, sLink.targetAddress.size());
  attID = H5Acreate(dataID, name, aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attID < 0) {
    return NXstatus::NX_OK;
  }
  UNUSED_ARG(H5Awrite(attID, aid1, sLink.targetAddress.c_str()));
  H5Tclose(aid1);
  H5Sclose(aid2);
  H5Aclose(attID);
  if (sLink.linkType == NXentrytype::sds) {
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

hid_t nxToHDF5Type(NXnumtype datatype) {
  hid_t type;
  switch (datatype) {
  case NXnumtype::CHAR: {
    type = H5T_C_S1;
    break;
  }
  case NXnumtype::INT8: {
    type = H5T_NATIVE_CHAR;
    break;
  }
  case NXnumtype::UINT8: {
    type = H5T_NATIVE_UCHAR;
    break;
  }
  case NXnumtype::INT16: {
    type = H5T_NATIVE_SHORT;
    break;
  }
  case NXnumtype::UINT16: {
    type = H5T_NATIVE_USHORT;
    break;
  }
  case NXnumtype::INT32: {
    type = H5T_NATIVE_INT;
    break;
  }
  case NXnumtype::UINT32: {
    type = H5T_NATIVE_UINT;
    break;
  }
  case NXnumtype::INT64: {
    type = H5T_NATIVE_INT64;
    break;
  }
  case NXnumtype::UINT64: {
    type = H5T_NATIVE_UINT64;
    break;
  }
  case NXnumtype::FLOAT32: {
    type = H5T_NATIVE_FLOAT;
    break;
  }
  case NXnumtype::FLOAT64: {
    type = H5T_NATIVE_DOUBLE;
    break;
  }
  default: {
    NXReportError("ERROR: nxToHDF5Type: unknown type");
    type = -1;
  }
  }
  return type;
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
  std::string attr_name("NX_class");
  return strstr(member_name, attr_name.c_str()) ? 1 : 0;
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
  if (NXgetdataID(hfil, id) == NXstatus::NX_ERROR) {
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
  if (NXgetgroupID(hfil, id) == NXstatus::NX_ERROR) {
    return 1;
  } else {
    return 0;
  }
}

/*--------------------------------------------------------------------
  copies the next address element into element.
  returns a pointer into address beyond the extracted address
  ---------------------------------------------------------------------*/
std::string extractNextAddress(std::string const &address, std::string &element) {
  std::size_t start = 0;
  /*
     skip over leading /
   */
  if (address.starts_with('/')) {
    start++;
  }

  /*
     find next /
   */
  std::size_t next = address.find_first_of('/', start);
  std::size_t width = next - start;
  element = address.substr(start, width);
  if (next == std::string::npos) {
    return "";
  } else {
    return address.substr(next + 1, std::string::npos);
  }
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
int isRelative(std::string const &address) {
  if (address.starts_with(".."))
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
std::string moveDown(NXhandle fid, std::string const &address, NXstatus &code) {
  code = NXstatus::NX_OK;
  std::string ret(address);

  if (address.starts_with('/')) {
    code = gotoRoot(fid);
  } else {
    while (isRelative(ret)) {
      NXstatus status = moveOneDown(fid);
      if (status == NXstatus::NX_ERROR) {
        code = status;
        break;
      } else {
        ret = ret.substr(3, std::string::npos);
      }
    }
  }
  return ret;
}

/*--------------------------------------------------------------------*/
NXstatus stepOneUp(NXhandle fid, std::string const &name) {
  NXnumtype datatype;
  std::string name2, xclass;

  /*
     catch the case when we are there: i.e. no further stepping
     necessary. This can happen with address like ../
   */
  if (name.size() < 1) {
    return NXstatus::NX_OK;
  }

  NXinitgroupdir(fid);

  while (NXgetnextentry(fid, name2, xclass, datatype) != NXstatus::NX_EOD) {
    if (name2 == name) {
      if (xclass == "SDS") {
        return NXopendata(fid, name);
      } else {
        return NXopengroup(fid, name, xclass);
      }
    }
  }
  std::string warning("ERROR: NXopenaddress cannot step into " + name);
  NXReportError(warning.c_str());
  return NXstatus::NX_ERROR;
}

/*--------------------------------------------------------------------*/
NXstatus stepOneGroupUp(NXhandle fid, std::string const &name) {
  NXnumtype datatype;
  std::string name2, xclass;

  /*
     catch the case when we are there: i.e. no further stepping
     necessary. This can happen with address like ../
   */
  if (name.size() < 1) {
    return NXstatus::NX_OK;
  }

  NXinitgroupdir(fid);
  while (NXgetnextentry(fid, name2, xclass, datatype) != NXstatus::NX_EOD) {

    if (name2 == name) {
      if (xclass == "SDS") {
        return NXstatus::NX_EOD;
      } else {
        return NXopengroup(fid, name, xclass);
      }
    }
  }
  std::string warning("ERROR: NXopengroupaddress cannot step into " + name);
  NXReportError(warning.c_str());
  return NXstatus::NX_ERROR;
}

/*---------------------------------------------------------------------
 * private functions used in NX5open
 */

hid_t create_file_access_plist(std::string const &filename) {
  char pBuffer[512];
  hid_t fapl = -1;

  /* create file access property list - required in all cases*/
  if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0) {
    sprintf(pBuffer,
            "Error: failed to create file access property "
            "list for file %s",
            filename.c_str());
    NXReportError(pBuffer);
    return fapl;
  }

  /* set file close policy - need this in all cases*/
  if (H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) < 0) {
    sprintf(pBuffer,
            "Error: cannot set close policy for file "
            "%s",
            filename.c_str());
    NXReportError(pBuffer);
    return fapl;
  }

  return fapl;
}

herr_t set_str_attribute(hid_t parent_id, std::string const &name, std::string const &buffer) {
  char pBuffer[512];
  hid_t attr_id;
  hid_t space_id = H5Screate(H5S_SCALAR);
  hid_t type_id = H5Tcopy(H5T_C_S1);

  H5Tset_size(type_id, buffer.size());

  attr_id = H5Acreate(parent_id, name.c_str(), type_id, space_id, H5P_DEFAULT, H5P_DEFAULT);
  if (attr_id < 0) {
    sprintf(pBuffer, "ERROR: failed to create %s attribute", name.c_str());
    NXReportError(pBuffer);
    return -1;
  }

  if (H5Awrite(attr_id, type_id, buffer.data()) < 0) {
    sprintf(pBuffer, "ERROR: failed writting %s attribute", name.c_str());
    NXReportError(pBuffer);
    return -1;
  }

  H5Tclose(type_id);
  H5Sclose(space_id);
  H5Aclose(attr_id);

  return 0;
}
