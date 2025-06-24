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

// cppcheck-suppress-begin [unmatchedSuppression, variableScope, invalidPrintfArgType_uint]
// cppcheck-suppress-begin [constParameterCallback, unreadVariable, constParameter, constParameterPointer]

#include <string>
#define H5Aiterate_vers 2

#include <algorithm>
#include <array>
#include <assert.h>
#include <cstring>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "MantidNexus/napi.h"

// this has to be after the other napi includes
#include "MantidNexus/napi5.h"
#include "MantidNexus/napi_helper.h"

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

/*---------------------------------------------------------------------

  Definition of NeXus API

---------------------------------------------------------------------*/

NXstatus NX5open(std::string const &filename, NXaccess const am, NXhandle &fid) {
  hid_t root_id;
  char pBuffer[512];
  char *time_buffer = NULL;
  char version_nr[10];
  unsigned int vers_major, vers_minor, vers_release, am1;
  hid_t fapl = -1;

  fid = nullptr;

  if (H5get_libversion(&vers_major, &vers_minor, &vers_release) < 0) {
    NXReportError("ERROR: cannot determine HDF5 library version");
    return NXstatus::NX_ERROR;
  }
  if (vers_major == 1 && vers_minor < 8) {
    NXReportError("ERROR: HDF5 library 1.8.0 or higher required");
    return NXstatus::NX_ERROR;
  }

  /* turn off the automatic HDF error handling */
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);
#ifdef USE_FTIME
  struct timeb timeb_struct;
#endif

  /* create the file access property list*/
  if ((fapl = create_file_access_plist(filename)) < 0) {
    return NXstatus::NX_ERROR;
  }

  /* start HDF5 interface */
  fid = new NexusFile5;
  if (am == NXaccess::CREATE5) {
    am1 = H5F_ACC_TRUNC;
    fid->iFID = H5Fcreate(filename.c_str(), am1, H5P_DEFAULT, fapl);
  } else {
    if (am == NXaccess::READ)
      am1 = H5F_ACC_RDONLY;
    else
      am1 = H5F_ACC_RDWR;

    fid->iFID = H5Fopen(filename.c_str(), am1, fapl);
  }

  if (fapl != -1)
    H5Pclose(fapl); /*close file access property list*/

  if (fid->iFID <= 0) {
    snprintf(pBuffer, sizeof(pBuffer) - 1, "ERROR: cannot open file: %s", filename.c_str());
    NXReportError(pBuffer);
    delete fid;
    fid = nullptr;
    return NXstatus::NX_ERROR;
  }

  /*
   * need to create global attributes
      file_name
      file_time
      NeXus_version
   * at some point for new files
   */

  if (am == NXaccess::CREATE5) {
    root_id = H5Gopen(fid->iFID, "/", H5P_DEFAULT);
    if (set_str_attribute(root_id, "NeXus_version", NEXUS_VERSION) < 0) {
      H5Gclose(root_id);
      H5Fclose(fid->iFID);
      delete fid;
      fid = nullptr;
      return NXstatus::NX_ERROR;
    }

    if (set_str_attribute(root_id, "file_name", filename) < 0) {
      H5Gclose(root_id);
      H5Fclose(fid->iFID);
      delete fid;
      fid = nullptr;
      return NXstatus::NX_ERROR;
    }

    sprintf(version_nr, "%u.%u.%u", vers_major, vers_minor, vers_release);
    if (set_str_attribute(root_id, "HDF5_Version", version_nr) < 0) {
      H5Gclose(root_id);
      H5Fclose(fid->iFID);
      delete fid;
      fid = nullptr;
      return NXstatus::NX_ERROR;
    }

    time_buffer = NXIformatNeXusTime();
    if (time_buffer != NULL) {
      if (set_str_attribute(root_id, "file_time", time_buffer) < 0) {
        H5Gclose(root_id);
        H5Fclose(fid->iFID);
        delete fid;
        fid = nullptr;
        free(time_buffer);
        return NXstatus::NX_ERROR;
      }
      free(time_buffer);
    }

    /*finally we set the NXroot NX_class attribute*/
    if (set_str_attribute(root_id, "NX_class", "NXroot") < 0) {
      H5Gclose(root_id);
      H5Fclose(fid->iFID);
      delete fid;
      fid = nullptr;
      return NXstatus::NX_ERROR;
    }

    H5Gclose(root_id);
  }

  fid->iStack5[0].iVref = 0; /* root! */
  return NXstatus::NX_OK;
}

/*-----------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
/* ------------------------------------------------------------------- */
/*-----------------------------------------------------------------------*/

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
/*------------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/* ------------------------------------------------------------------- */

NXstatus NX5putattr(NXhandle fid, std::string const &name, const void *data, std::size_t const datalen,
                    NXnumtype const iType) {
  pNexusFile5 pFile;
  hid_t attr1;
  hid_t type;
  herr_t iRet = 0;
  hid_t vid, attRet;

  pFile = NXI5assert(fid);

  type = nxToHDF5Type(iType);

  // determine ID of containing HDF object
  vid = getAttVID(pFile);

  // check if the attribute exists -- if so, delete it
  attRet = H5Aopen_by_name(vid, ".", name.c_str(), H5P_DEFAULT, H5P_DEFAULT);
  if (attRet > 0) {
    H5Aclose(attRet);
    iRet = H5Adelete(vid, name.c_str());
    if (iRet < 0) {
      NXReportError("ERROR: old attribute cannot be removed! ");
      killAttVID(pFile, vid);
      return NXstatus::NX_ERROR;
    }
  }

  // prepare dataspace, datatype
  hid_t dataspace = H5Screate(H5S_SCALAR);
  hid_t datatype = H5Tcopy(type);
  if (iType == NXnumtype::CHAR) {
    H5Tset_size(datatype, datalen);
  }

  // create the attribute
  attr1 = H5Acreate(vid, name.c_str(), datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT);
  if (attr1 < 0) {
    NXReportError("ERROR: attribute cannot created! ");
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }
  if (H5Awrite(attr1, datatype, data) < 0) {
    NXReportError("ERROR: failed to store attribute ");
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }
  /* Close attribute dataspace */
  iRet += H5Tclose(datatype);
  iRet += H5Sclose(dataspace);
  /* Close attribute  */
  iRet += H5Aclose(attr1);
  killAttVID(pFile, vid);
  // TODO QUESTION always return that it is ok?
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/*--------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
/* ------------------------------------------------------------------- */
/*----------------------------------------------------------------------*/
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

NXstatus NX5getgroupinfo_recurse(NXhandle fid, std::size_t &iN, std::string &name, std::string &nxclass) {
  pNexusFile5 pFile;
  hid_t atype, attr_id, grp;

  pFile = NXI5assert(fid);
  /* check if there is a group open */
  if (pFile->iCurrentG == 0) {
    name = "root";
    nxclass = "NXroot";
    pFile->iNX = 0;
    grp = H5Gopen(pFile->iFID, "/", H5P_DEFAULT);
    H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, 0, group_info, &pFile->iNX);
    H5Gclose(grp);
    iN = pFile->iNX;
  } else {
    name = pFile->name_ref;
    attr_id = H5Aopen_by_name(pFile->iCurrentG, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
      nxclass = NX_UNKNOWN_GROUP;
    } else {
      atype = H5Tcopy(H5T_C_S1);
      char data[64];
      H5Tset_size(atype, sizeof(data));
      readStringAttributeN(attr_id, data, sizeof(data));
      nxclass = data;
      pFile->iNX = 0;
      grp = H5Gopen(pFile->iFID, pFile->name_ref.c_str(), H5P_DEFAULT);
      H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, 0, group_info, &pFile->iNX);
      H5Gclose(grp);
      iN = pFile->iNX;
      H5Aclose(attr_id);
    }
  }
  return NXstatus::NX_OK;
}

/*----------------------------------------------------------------------------*/
NXstatus NX5getgroupinfo(NXhandle fid, std::size_t &iN, std::string &name, std::string &nxclass) {
  pNexusFile5 pFile;
  hid_t atype, attr_id, gid;

  pFile = NXI5assert(fid);
  /* check if there is a group open */
  if (pFile->iCurrentG == 0) {
    name = "root";
    nxclass = "NXroot";
    gid = H5Gopen(pFile->iFID, "/", H5P_DEFAULT);
    iN = countObjectsInGroup(gid);
    H5Gclose(gid);
  } else {
    name = pFile->name_ref;
    attr_id = H5Aopen_by_name(pFile->iCurrentG, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attr_id < 0) {
      nxclass = NX_UNKNOWN_GROUP;
    } else {
      atype = H5Tcopy(H5T_C_S1);
      char data[64];
      H5Tset_size(atype, sizeof(data));
      readStringAttributeN(attr_id, data, sizeof(data));
      nxclass = data;
      H5Aclose(attr_id);
    }
    pFile->iNX = 0;
    iN = countObjectsInGroup(pFile->iCurrentG);
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getnextentry(NXhandle fid, std::string &name, std::string &nxclass, NXnumtype &datatype) {
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
  if (pFile->name_ref.empty()) {
    /* root group */
    pFile->name_ref = "/";
  }
  grp = H5Gopen(pFile->iFID, pFile->name_ref.c_str(), H5P_DEFAULT);
  // index can be wrong here
  iRet = H5Literate(grp, H5_INDEX_NAME, H5_ITER_INC, &idx, nxgroup_info, &op_data);
  H5Gclose(grp);
  nxclass = NX_UNKNOWN_GROUP;

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
    grp = H5Gopen(pFile->iFID, pFile->name_ref.c_str(), H5P_DEFAULT);
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
      name = op_data.iname;
      free(op_data.iname);
    } else {
      pFile->iStack5[pFile->iStackPtr].iCurrentIDX = 0;
      return NXstatus::NX_EOD;
    }
    if (op_data.type == H5O_TYPE_GROUP) {
      /*
         open group and find class name attribute
       */
      std::string ph_name("");
      for (std::size_t i = 1; i < (pFile->iStackPtr + 1); i++) {
        ph_name += pFile->iStack5[i].irefn + "/";
      }
      ph_name += name;
      grp = H5Gopen(pFile->iFID, ph_name.c_str(), H5P_DEFAULT);
      if (grp < 0) {
        size_t const Nbuff(2048);
        char pBuffer[Nbuff];
        snprintf(pBuffer, Nbuff, "ERROR: group %s does not exist", ph_name.c_str());
        NXReportError(pBuffer);
        return NXstatus::NX_ERROR;
      }
      attr1 = H5Aopen_by_name(grp, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
      if (attr1 < 0) {
        nxclass = NX_UNKNOWN_GROUP;
      } else {
        type = H5T_C_S1;
        atype = H5Tcopy(type);
        char data[128];
        H5Tset_size(atype, sizeof(data));
        if (readStringAttributeN(attr1, data, sizeof(data)) < 0) {
          NXReportError("ERROR: reading attribute");
          return NXstatus::NX_ERROR;
        }
        nxclass = data;
        H5Tclose(atype);
        H5Aclose(attr1);
      }
      H5Gclose(grp);
    } else if (op_data.type == H5O_TYPE_DATASET) {
      /*
         open dataset and find type
       */
      if (pFile->iCurrentG == 0) {
        grp = H5Dopen(pFile->iFID, name.c_str(), H5P_DEFAULT);
      } else {
        grp = H5Dopen(pFile->iCurrentG, name.c_str(), H5P_DEFAULT);
      }
      type = H5Dget_type(grp);
      atype = H5Tcopy(type);
      tclass = H5Tget_class(atype);
      NXnumtype iPtype = hdf5ToNXType(tclass, atype);
      datatype = iPtype;
      nxclass = "SDS";
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

NXstatus NX5getchardata(NXhandle fid, void *data) {
  pNexusFile5 pFile = NXI5assert(fid);
  NXstatus status = NXstatus::NX_ERROR;

  if (H5Tis_variable_str(pFile->iCurrentT)) {
    char *cdata = nullptr;
    herr_t ret = H5Dread(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &cdata);
    if (ret < 0 || cdata == nullptr) {
      status = NXstatus::NX_ERROR;
    } else {
      memcpy(data, cdata, strlen(cdata) * sizeof(char));
      H5free_memory(cdata); // NOTE must free cdata within hdf5
      status = NXstatus::NX_OK;
    }
  } else {
    hsize_t dims[NX_MAXRANK] = {0};
    hsize_t len = H5Tget_size(pFile->iCurrentT);
    // for a 2D char array, handle block size
    int rank = H5Sget_simple_extent_dims(pFile->iCurrentS, dims, NULL);
    for (int i = 0; i < rank - 1; i++) {
      len *= (dims[i] > 1 ? dims[i] : 1); // min of dims[0], 1
    }
    // reserve space in memory
    char *cdata = new char[len + 1];
    memset(cdata, 0, (len + 1) * sizeof(char));
    herr_t ret = H5Dread(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, cdata);
    if (ret < 0) {
      status = NXstatus::NX_ERROR;
    } else {
      cdata[len] = '\0'; // ensure null termination
      /* NOTE len is truly the correct length to use here.
       * It is not necessary to use (len + 1); null termination is already handled,
       * and even-more-handling it causes errors downstream.
       * It is not preferable to use strlen(cdata), as the cdata may have \0 between char arrays. */
      memcpy(data, cdata, len * sizeof(char));
      status = NXstatus::NX_OK;
    }
    delete[] cdata;
  }
  return status;
}

NXstatus NX5getdata(NXhandle fid, void *data) {
  pNexusFile5 pFile = NXI5assert(fid);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    NXReportError("ERROR: no dataset open");
    return NXstatus::NX_ERROR;
  }

  NXstatus status;
  if (H5Tget_class(pFile->iCurrentT) == H5T_STRING) {
    status = NX5getchardata(fid, data);
  } else {
    int ret;
    hsize_t ndims, dims[H5S_MAX_RANK];
    ndims = static_cast<hsize_t>(H5Sget_simple_extent_dims(pFile->iCurrentS, dims, NULL));
    if (ndims == 0) { /* SCALAR dataset */
      hid_t datatype = H5Dget_type(pFile->iCurrentD);
      hid_t filespace = H5Dget_space(pFile->iCurrentD);
      hid_t memtype_id = H5Screate(H5S_SCALAR);
      H5Sselect_all(filespace);
      ret = H5Dread(pFile->iCurrentD, datatype, memtype_id, filespace, H5P_DEFAULT, data);
      // cleanup
      H5Sclose(memtype_id);
      H5Sclose(filespace);
      H5Tclose(datatype);
    } else {
      /* map datatypes of other plateforms */
      hid_t memtype_id = h5MemType(pFile->iCurrentT);
      ret = H5Dread(pFile->iCurrentD, memtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    }
    if (ret < 0) {
      NXReportError("ERROR: failed to transfer dataset");
      status = NXstatus::NX_ERROR;
    } else {
      status = NXstatus::NX_OK;
    }
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getinfo64(NXhandle fid, std::size_t &rank, Mantid::Nexus::DimVector &dims, NXnumtype &iType) {
  pNexusFile5 pFile;
  std::size_t iRank;
  NXnumtype mType;
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
  }
  /* conversion to proper ints for the platform */
  iType = mType;
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
  rank = iRank;
  dims.resize(rank);
  for (std::size_t i = 0; i < rank; i++) {
    dims[i] = myDim[i];
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/* Operator function. */

herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *unused, void *opdata) {
  UNUSED_ARG(loc_id);
  UNUSED_ARG(unused);
  *(static_cast<char **>(opdata)) = strdup(name);

  return 1;
}

NXstatus NX5getnextattr(NXhandle fileid, std::string &name, std::size_t &iLength, NXnumtype &iType) {
  std::size_t rank;
  NXstatus status;
  Mantid::Nexus::DimVector mydim;

  status = NX5getnextattra(fileid, name, rank, mydim, iType);

  if (status != NXstatus::NX_OK)
    return status;

  if (rank == 0 || (rank == 1 && mydim[0] == 1)) {
    iLength = 1;
    return NXstatus::NX_OK;
  }

  if (rank == 1 && iType == NXnumtype::CHAR) {
    iLength = mydim[0];
    return NXstatus::NX_OK;
  }

  NXReportError("ERROR iterating through attributes found array attribute not understood by this api");
  return NXstatus::NX_ERROR;
}

/*-------------------------------------------------------------------------*/

// cppcheck-suppress constParameterCallback
NXstatus NX5getattr(NXhandle fid, std::string const &name, void *data, std::size_t &datalen, NXnumtype &iType) {
  pNexusFile5 pFile;
  hid_t vid, iNew;
  hsize_t dims[H5S_MAX_RANK], totalsize;
  herr_t iRet;
  hid_t type, filespace;
  char pBuffer[256];

  pFile = NXI5assert(fid);

  type = nxToHDF5Type(iType);

  vid = getAttVID(pFile);
  iNew = H5Aopen_by_name(vid, ".", name.c_str(), H5P_DEFAULT, H5P_DEFAULT);
  if (iNew < 0) {
    sprintf(pBuffer, "ERROR: attribute \"%s\" not found", name.c_str());
    killAttVID(pFile, vid);
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }
  pFile->iCurrentA = iNew;

  // get the dataspace and proper dimensions
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
    iRet = readStringAttributeN(pFile->iCurrentA, static_cast<char *>(data), datalen);
    datalen = strlen(static_cast<char *>(data));
  } else {
    iRet = H5Aread(pFile->iCurrentA, type, data);
    datalen = 1;
  }

  if (iRet < 0) {
    sprintf(pBuffer, "ERROR: could not read attribute data for \"%s\"", name.c_str());
    NXReportError(pBuffer);
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }

  H5Aclose(pFile->iCurrentA);

  killAttVID(pFile, vid);
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NX5getattrinfo(NXhandle fid, std::size_t &iN) {
  pNexusFile5 pFile;
  int idx;
  hid_t vid;
  H5O_info1_t oinfo;

  pFile = NXI5assert(fid);
  idx = 0;
  iN = idx;

  vid = getAttVID(pFile);

  // TODO use new version of method rather than v1
  H5Oget_info1(vid, &oinfo);
  idx = static_cast<int>(oinfo.num_attrs);
  if (idx > 0) {
    if (pFile->iCurrentG > 0 && pFile->iCurrentD == 0) {
      iN = idx - 1;
    } else {
      iN = idx;
    }
  } else {
    iN = 0;
  }
  killAttVID(pFile, vid);
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

/* ------------------------------------------------------------------- */

/*-------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
NXstatus NX5getnextattra(NXhandle fid, std::string &name, std::size_t &rank, Mantid::Nexus::DimVector &dim,
                         NXnumtype &iType) {
  pNexusFile5 pFile;
  herr_t iRet;
  char *iname = NULL;
  hid_t vid;
  H5O_info1_t oinfo;

  pFile = NXI5assert(fid);

  vid = getAttVID(pFile);

  name = "\0";
  hsize_t idx = pFile->iCurrentIDX;
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
  pFile->iCurrentIDX++;
  if (iname != NULL) {
    if (strcmp(iname, "NX_class") == 0 && pFile->iCurrentG != 0 && pFile->iCurrentD == 0) {
      /*
         skip NXclass attribute which is internal
       */
      free(iname);
      iname = NULL;
      killAttVID(pFile, vid);
      return NX5getnextattra(fid, name, rank, dim, iType);
    }
    name = iname;
    free(iname);
    iname = NULL;
  } else {
    NXReportError("ERROR: encountered nameless attribute");
    killAttVID(pFile, vid);
    return NXstatus::NX_ERROR;
  }

  killAttVID(pFile, vid);
  return NX5getattrainfo(fid, name, rank, dim, iType);
}
/*------------------------------------------------------------------------*/
NXstatus NX5getattrainfo(NXhandle handle, std::string const &name, std::size_t &rank, Mantid::Nexus::DimVector &dim,
                         NXnumtype &iType) {
  pNexusFile5 pFile;
  int iRet;
  NXnumtype mType;
  hid_t vid;
  hid_t filespace, attrt, memtype;
  hsize_t myDim[H5S_MAX_RANK];
  H5T_class_t tclass;
  char *vlStr = NULL;

  pFile = NXI5assert(handle);

  vid = getAttVID(pFile);
  pFile->iCurrentA = H5Aopen_by_name(vid, ".", name.c_str(), H5P_DEFAULT, H5P_DEFAULT);
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
  iType = mType;

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

  rank = static_cast<std::size_t>(myrank);
  dim.resize(rank);
  for (std::size_t i = 0; i < rank; i++) {
    dim[i] = myDim[i];
  }

  return NXstatus::NX_OK;
}

// cppcheck-suppress-end [constParameterCallback, unreadVariable, constParameter, constParameterPointer]
// cppcheck-suppress-end [unmatchedSuppression, variableScope, invalidPrintfArgType_uint]
