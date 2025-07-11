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

// cppcheck-suppress-begin unmatchedSuppression
// cppcheck-suppress-begin [variableScope, invalidPrintfArgType_uint, constParameterReference]
// cppcheck-suppress-begin [constParameterCallback, unreadVariable, constParameter, constParameterPointer]
// cppcheck-suppress-begin [shadowArgument]

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
/*----------------------------------------------------------------------------*/
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
  idx = pFile->iStack5.back().iCurrentIDX;
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
    pFile->iStack5.back().iCurrentIDX++;
    if (op_data.iname != NULL) {
      name = op_data.iname;
      free(op_data.iname);
    } else {
      pFile->iStack5.back().iCurrentIDX = 0;
      return NXstatus::NX_EOD;
    }
    if (op_data.type == H5O_TYPE_GROUP) {
      /*
         open group and find class name attribute
       */
      std::string ph_name("");
      for (stackEntry const &entry : pFile->iStack5) {
        ph_name += entry.irefn + "/";
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
      pFile->iStack5.back().iCurrentIDX = 0;
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
      hsize_t len = strlen(cdata);
      cdata[len] = '\0';
      memcpy(data, cdata, (len + 1) * sizeof(char));
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

// cppcheck-suppress-end [constParameterCallback, unreadVariable, constParameter, constParameterPointer, shadowArgument]
// cppcheck-suppress-end [variableScope, invalidPrintfArgType_uint, constParameterReference]
// cppcheck-suppress-end unmatchedSuppression
