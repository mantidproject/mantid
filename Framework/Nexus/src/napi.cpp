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

#include <algorithm>
#include <array>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <hdf5.h>

#include "MantidNexus/napi.h"

// cppcheck-suppress-begin [unmatchedSuppression]
// cppcheck-suppress-begin [constVariablePointer, constParameterReference, unusedVariable, unreadVariable]

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

static bool canBeOpened(std::string const &filename) {
  // this is for reading, check for existence first
  FILE *fd = NULL;
  fd = fopen(filename.c_str(), "r");
  if (fd == NULL) {
    return false;
  }
  fclose(fd);

  // check that this is indeed hdf5
  if (H5Fis_hdf5(filename.c_str()) > 0) {
    return true;
  } else {
    // file type not recognized
    return false;
  }
}

/*----------------------------------------------------------------------*/
NXstatus NXopen(std::string const &filename, NXaccess const am, NXhandle &fid) {
  // allocate the object on the heap
  fid = nullptr;
  // determine if the file can be opened
  bool isHDF5 = (am == NXaccess::CREATE5 ? true : canBeOpened(filename));
  NXstatus status = NXstatus::NX_ERROR;
  if (isHDF5) {
    // construct on heap
    fid = new NexusFile5(filename, am);
    status = NXstatus::NX_OK;
  } else {
    NXReportError("ERROR: Format not readable by this NeXus library");
    status = NXstatus::NX_ERROR;
  }
  if (status != NXstatus::NX_OK && fid != nullptr) {
    delete fid;
    fid = nullptr;
  }

  return status;
}

/*-----------------------------------------------------------------------*/
/* ------------------------------------------------------------------------- */

NXstatus NXclose(NXhandle &fid) {
  if (fid == nullptr) {
    return NXstatus::NX_OK;
  }
  pNexusFile5 pFile = NXI5assert(fid);
  herr_t iRet = H5Fclose(pFile->iFID);
  if (iRet < 0) {
    NXReportError("ERROR: cannot close HDF file");
  }
  /* release memory */
  NXI5KillDir(pFile);
  delete fid;
  fid = nullptr;
  H5garbage_collect();
  return NXstatus::NX_OK;
}

/*-----------------------------------------------------------------------*/

NXstatus NXmakegroup(NXhandle fid, std::string const &name, std::string const &nxclass) {
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
  H5Tset_size(aid1, nxclass.size());
  attr1 = H5Acreate(iVID, "NX_class", aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attr1 < 0) {
    NXReportError("ERROR: failed to store class name");
    return NXstatus::NX_ERROR;
  }
  if (H5Awrite(attr1, aid1, nxclass.c_str()) < 0) {
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

NXstatus NXopengroup(NXhandle fid, std::string const &name, std::string const &nxclass) {
  std::string pBuffer;

  pNexusFile5 pFile = NXI5assert(fid);
  if (pFile->iCurrentG == 0) {
    pBuffer = name;
  } else {
    pBuffer = pFile->name_tmp + "/" + name;
  }
  hid_t iVID = H5Gopen(pFile->iFID, pBuffer.c_str(), H5P_DEFAULT);
  if (iVID < 0) {
    std::string msg = std::string("ERROR: group ") + pFile->name_tmp + " does not exist";
    NXReportError(const_cast<char *>(msg.c_str()));
    return NXstatus::NX_ERROR;
  }
  pFile->iCurrentG = iVID;
  pFile->name_tmp = pBuffer;
  pFile->name_ref = pBuffer;

  if ((!nxclass.empty()) && (nxclass != NX_UNKNOWN_GROUP)) {
    /* check group attribute */
    herr_t iRet = H5Aiterate(pFile->iCurrentG, H5_INDEX_CRT_ORDER, H5_ITER_INC, 0, attr_check, NULL);
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
    hid_t attr1 = H5Aopen_by_name(pFile->iCurrentG, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attr1 < 0) {
      NXReportError("ERROR: opening NX_class group attribute");
      return NXstatus::NX_ERROR;
    }
    hid_t atype = H5Tcopy(H5T_C_S1);
    char data[128];
    H5Tset_size(atype, sizeof(data));
    iRet = readStringAttributeN(attr1, data, sizeof(data));
    if (data == nxclass) {
      /* test OK */
    } else {
      std::string warning("ERROR: group class is not identical: \"");
      warning += data;
      warning += "\" != \"";
      warning += nxclass;
      warning += "\"";
      NXReportError(warning.c_str());
      H5Tclose(atype);
      H5Aclose(attr1);
      return NXstatus::NX_ERROR;
    }
    H5Tclose(atype);
    H5Aclose(attr1);
  }

  /* maintain stack */
  pFile->iStack5.emplace_back(name, pFile->iCurrentG, 0);
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
    size_t i = pFile->iStack5.back().irefn.size();
    size_t ii = pFile->name_ref.size();
    if (pFile->iStack5.size() > 2) {
      ii = ii - i - 1;
    } else {
      ii = ii - i;
    }
    if (ii > 0) {
      char *uname = strdup(pFile->name_ref.c_str());
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
      pFile->name_ref = u1name;
      pFile->name_tmp = u1name;
      free(uname);
      free(u1name);
    } else {
      pFile->name_ref = "";
      pFile->name_tmp = "";
    }
    NXI5KillDir(pFile);
    pFile->iCurrentD = 0;
    pFile->iStack5.pop_back();
    if (!pFile->iStack5.empty()) {
      pFile->iCurrentG = pFile->iStack5.back().iVref;
    } else {
      pFile->iCurrentG = 0;
    }
  }
  return NXstatus::NX_OK;
}

/* --------------------------------------------------------------------- */

NXstatus NXmakedata64(NXhandle fid, std::string const &name, NXnumtype const datatype, std::size_t const rank,
                      Mantid::Nexus::DimVector const &dims) {
  NXI5assert(fid);

  Mantid::Nexus::DimSizeVector chunk_size(dims);
  for (std::size_t i = 0; i < rank; i++) {
    if (dims[i] == NX_UNLIMITED || dims[i] <= 0) {
      chunk_size[i] = 1;
    }
  }
  return NXcompmakedata64(fid, name, datatype, rank, dims, NXcompression::NONE, chunk_size);
}

/* --------------------------------------------------------------------- */

NXstatus NXcompmakedata64(NXhandle fid, std::string const &name, NXnumtype const datatype, std::size_t const rank,
                          Mantid::Nexus::DimVector const &dims, NXcompression const compress_type,
                          Mantid::Nexus::DimSizeVector const &chunk_size) {
  hid_t datatype1, dataspace, iNew;
  hid_t type, cparms = -1;
  pNexusFile5 pFile;
  char pBuffer[256];
  size_t byte_zahl = 0;
  hsize_t chunkdims[H5S_MAX_RANK];
  hsize_t mydim[H5S_MAX_RANK];
  hsize_t size[H5S_MAX_RANK];
  hsize_t maxdims[H5S_MAX_RANK];
  unsigned int compress_level;
  int unlimiteddim = 0;

  pFile = NXI5assert(fid);
  if (pFile->iCurrentG <= 0) {
    sprintf(pBuffer, "ERROR: no group open for makedata on %s", name.c_str());
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }

  if (rank == 0) {
    sprintf(pBuffer, "ERROR: invalid rank specified %s", name.c_str());
    NXReportError(pBuffer);
    return NXstatus::NX_ERROR;
  }

  type = nxToHDF5Type(datatype);

  /*
     Check dimensions for consistency. Dimension may be -1
     thus denoting an unlimited dimension.
   */
  for (std::size_t i = 0; i < rank; i++) {
    chunkdims[i] = chunk_size[i];
    mydim[i] = dims[i];
    maxdims[i] = dims[i];
    size[i] = dims[i];
    if (dims[i] <= 0) {
      mydim[i] = 1;
      maxdims[i] = H5S_UNLIMITED;
      size[i] = 1;
      unlimiteddim = 1;
    } else {
      mydim[i] = dims[i];
      maxdims[i] = dims[i];
      size[i] = dims[i];
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
    hsize_t mydim1[H5S_MAX_RANK];
    for (std::size_t i = 0; i < rank; i++) {
      mydim1[i] = mydim[i];
      if (dims[i] <= 0) {
        mydim1[0] = 1;
        maxdims[0] = H5S_UNLIMITED;
      }
    }
    mydim1[rank - 1] = 1;
    if (mydim[rank - 1] > 1) {
      maxdims[rank - 1] = size[rank - 1] = 1;
    }
    if (chunkdims[rank - 1] > 1) {
      chunkdims[rank - 1] = 1;
    }
    dataspace = H5Screate_simple(static_cast<int>(rank), mydim1, maxdims);
  } else {
    if (unlimiteddim) {
      dataspace = H5Screate_simple(static_cast<int>(rank), mydim, maxdims);
    } else {
      /* dataset creation */
      dataspace = H5Screate_simple(static_cast<int>(rank), mydim, NULL);
    }
  }
  datatype1 = H5Tcopy(type);
  if (datatype == NXnumtype::CHAR) {
    H5Tset_size(datatype1, byte_zahl);
    /*       H5Tset_strpad(H5T_STR_SPACEPAD); */
  }
  compress_level = 6;
  hid_t dID;
  if (compress_type == NXcompression::LZW) {
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    iNew = H5Pset_chunk(cparms, static_cast<int>(rank), chunkdims);
    if (iNew < 0) {
      NXReportError("ERROR: size of chunks could not be set");
      return NXstatus::NX_ERROR;
    }
    H5Pset_shuffle(cparms); // mrt: improves compression
    H5Pset_deflate(cparms, compress_level);
    dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
  } else if (compress_type == NXcompression::NONE) {
    if (unlimiteddim) {
      cparms = H5Pcreate(H5P_DATASET_CREATE);
      iNew = H5Pset_chunk(cparms, static_cast<int>(rank), chunkdims);
      if (iNew < 0) {
        NXReportError("ERROR: size of chunks could not be set");
        return NXstatus::NX_ERROR;
      }
      dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
    } else {
      dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
  } else if (compress_type == NXcompression::CHUNK) {
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    iNew = H5Pset_chunk(cparms, static_cast<int>(rank), chunkdims);
    if (iNew < 0) {
      NXReportError("ERROR: size of chunks could not be set");
      return NXstatus::NX_ERROR;
    }
    dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);

  } else {
    NXReportError("HDF5 doesn't support selected compression method! Dataset created without compression");
    dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
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
      sprintf(pBuffer, "ERROR: cannot create dataset %s", name.c_str());
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

NXstatus NXopendata(NXhandle fid, std::string const &name) {
  pNexusFile5 pFile = NXI5assert(fid);
  /* clear pending attribute directories first */
  NXI5KillAttDir(pFile);

  /* find the ID number and open the dataset */
  pFile->iCurrentD = H5Dopen(pFile->iCurrentG, name.c_str(), H5P_DEFAULT);
  if (pFile->iCurrentD < 0) {
    char pBuffer[256];
    sprintf(pBuffer, "ERROR: dataset \"%s\" not found at this level", name.c_str());
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

  return NXstatus::NX_OK;
}

/* ----------------------------------------------------------------- */

NXstatus NXclosedata(NXhandle fid) {
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

NXstatus NXputdata(NXhandle fid, const void *data) {
  pNexusFile5 pFile;
  herr_t iRet;
  std::array<hsize_t, H5S_MAX_RANK> thedims{0}, maxdims{0};

  pFile = NXI5assert(fid);
  int rank = H5Sget_simple_extent_ndims(pFile->iCurrentS);
  if (rank < 0) {
    NXReportError("ERROR: Cannot determine dataset rank");
    return NXstatus::NX_ERROR;
  }
  iRet = H5Sget_simple_extent_dims(pFile->iCurrentS, thedims.data(), maxdims.data());
  if (iRet < 0) {
    NXReportError("ERROR: Cannot determine dataset dimensions");
    return NXstatus::NX_ERROR;
  }
  bool unlimiteddim = std::any_of(maxdims.cbegin(), maxdims.cend(), [](auto x) -> bool { return x == H5S_UNLIMITED; });
  /* If we are using putdata on an unlimied dimension dataset, assume we want to append one single new slab */
  if (unlimiteddim) {
    Mantid::Nexus::DimSizeVector myStart(rank, 0), mySize(rank);
    for (int i = 0; i < rank; i++) {
      if (maxdims[i] == H5S_UNLIMITED) {
        myStart[i] = thedims[i] + 1;
        mySize[i] = 1;
      } else {
        myStart[i] = 0;
        mySize[i] = thedims[i];
      }
    }
    return NXputslab64(fid, data, myStart, mySize);
  } else {
    iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (iRet < 0) {
      NXReportError("ERROR: failure to write data");
      return NXstatus::NX_ERROR;
    }
  }
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NXputattr(NXhandle fid, std::string const &name, const void *data, std::size_t const datalen,
                   NXnumtype const iType) {
  if (datalen > 1 && iType != NXnumtype::CHAR) {
    NXReportError(
        "NXputattr: numeric arrays are not allowed as attributes - only character strings and single numbers");
    return NXstatus::NX_ERROR;
  }
  return NX5putattr(fid, name, data, datalen, iType);
}

NXstatus NXputslab64(NXhandle fid, const void *data, Mantid::Nexus::DimSizeVector const &iStart,
                     Mantid::Nexus::DimSizeVector const &iSize) {
  pNexusFile5 pFile;
  int iRet, rank;
  hsize_t myStart[H5S_MAX_RANK];
  hsize_t mySize[H5S_MAX_RANK];
  hsize_t size[H5S_MAX_RANK], thedims[H5S_MAX_RANK], maxdims[H5S_MAX_RANK];
  hid_t dataspace;
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

    hid_t filespace = H5Dget_space(pFile->iCurrentD);

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

/* ---- --------------------------------------------------------------- */

NXstatus NXgetdataID(NXhandle fid, NXlink &sRes) {
  NXnumtype type = NXnumtype::CHAR;

  pNexusFile5 pFile = NXI5assert(fid);

  /* we cannot return ID's when no datset is open */
  if (pFile->iCurrentD <= 0) {
    return NXstatus::NX_ERROR;
  }

  /*
     this means: if the item is already linked: use the target attribute else,
     the address to the current node
   */
  std::size_t datalen = 1024;
  char caddr[1024] = {0};
  if (NXgetattr(fid, "target", caddr, datalen, type) != NXstatus::NX_OK) {
    sRes.targetAddress = buildCurrentAddress(pFile);
  } else {
    sRes.targetAddress = std::string(caddr);
  }
  sRes.linkType = NXentrytype::sds;
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NXmakelink(NXhandle fid, NXlink const &sLink) {
  pNexusFile5 pFile;
  std::string linkTarget;
  // char *itemName = NULL;

  pFile = NXI5assert(fid);
  if (pFile->iCurrentG == 0) { /* root level, can not link here */
    return NXstatus::NX_ERROR;
  }

  /*
     locate name of the element to link
   */
  std::string itemName = sLink.targetAddress.substr(sLink.targetAddress.find_last_of('/') + 1);

  /*
     build addressname to link from our current group and the name
     of the thing to link
   */
  linkTarget = "/" + pFile->name_ref + "/" + itemName;

  H5Lcreate_hard(pFile->iFID, sLink.targetAddress.c_str(), H5L_SAME_LOC, linkTarget.c_str(), H5P_DEFAULT, H5P_DEFAULT);

  return NX5settargetattribute(pFile, sLink);
}

/* -------------------------------------------------------------------- */

/*----------------------------------------------------------------------*/

NXstatus NXflush(NXhandle &fid) {
  pNexusFile5 pFile = nullptr;
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

NXstatus NXmalloc64(void *&data, std::size_t rank, Mantid::Nexus::DimVector const &dims, NXnumtype datatype) {
  size_t size = 1;
  data = nullptr;
  for (size_t i = 0; i < rank; i++) {
    size *= dims[i];
  }
  switch (datatype) {
  case NXnumtype::CHAR:
  case NXnumtype::INT8:
  case NXnumtype::UINT8:
    size += 2;
    break;
  case NXnumtype::INT16:
  case NXnumtype::UINT16:
    size *= 2;
    break;
  case NXnumtype::INT32:
  case NXnumtype::UINT32:
  case NXnumtype::FLOAT32:
    size *= 4;
    break;
  case NXnumtype::INT64:
  case NXnumtype::UINT64:
  case NXnumtype::FLOAT64:
    size *= 8;
    break;
  default:
    NXReportError("ERROR: NXmalloc - unknown data type in array");
    return NXstatus::NX_ERROR;
  }
  data = calloc(size, 1);
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NXfree(void **data) {
  if (data == nullptr) {
    NXReportError("ERROR: passing NULL to NXfree");
    return NXstatus::NX_ERROR;
  }
  if (*data == nullptr) {
    NXReportError("ERROR: passing already freed pointer to NXfree");
    return NXstatus::NX_ERROR;
  }
  free(*data);
  *data = nullptr;
  return NXstatus::NX_OK;
}

/* --------------------------------------------------------------------- */

NXstatus NXgetnextentry(NXhandle fid, std::string &name, std::string &nxclass, NXnumtype &datatype) {
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
  std::size_t rank;
  Mantid::Nexus::DimVector iDim;

  NX5getinfo64(fid, rank, iDim, type); /* unstripped size if string */
  /* only strip one dimensional strings */
  if ((type == NXnumtype::CHAR) && (rank == 1)) {
    std::string pPtr(iDim[0] + 5, '\0');
    status = NX5getdata(fid, pPtr.data());
    char const *pPtr2 = nxitrim(pPtr.data());
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
    strncpy(static_cast<char *>(data), pPtr2, strlen(pPtr2)); /* not NULL terminated by default */
#if defined(__GNUC__) && !(defined(__clang__))
#pragma GCC diagnostic pop
#endif
  } else {
    status = NX5getdata(fid, data);
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetinfo64(NXhandle fid, std::size_t &rank, Mantid::Nexus::DimVector &dims, NXnumtype &iType) {
  NXstatus status;
  char *pPtr = NULL;
  rank = 0;
  status = NX5getinfo64(fid, rank, dims, iType);
  /*
     the length of a string may be trimmed....
   */
  /* only strip one dimensional strings */
  if ((iType == NXnumtype::CHAR) && (rank == 1)) {
    pPtr = static_cast<char *>(malloc(static_cast<size_t>(dims[0] + 1) * sizeof(char)));
    if (pPtr != NULL) {
      memset(pPtr, 0, static_cast<size_t>(dims[0] + 1) * sizeof(char));
      NX5getdata(fid, pPtr);
      dims[0] = static_cast<int64_t>(strlen(nxitrim(pPtr)));
      free(pPtr);
    }
  }
  return status;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetslab64(NXhandle fid, void *data, Mantid::Nexus::DimSizeVector const &iStart,
                     Mantid::Nexus::DimSizeVector const &iSize) {
  pNexusFile5 pFile;
  hsize_t mySize[H5S_MAX_RANK];
  hid_t memspace, iRet;
  H5T_class_t tclass;
  hid_t memtype_id;
  char *tmp_data = nullptr;
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
    hsize_t myStart[H5S_MAX_RANK];
    hsize_t mStart[H5S_MAX_RANK];
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
      mtype = NXnumtype::CHAR;
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
    if (mtype == NXnumtype::CHAR) {
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

NXstatus NXgetattr(NXhandle fid, std::string const &name, void *data, std::size_t &datalen, NXnumtype &iType) {
  return NX5getattr(fid, name, data, datalen, iType);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetattrainfo(NXhandle handle, std::string const &name, std::size_t &rank, Mantid::Nexus::DimVector &dims,
                        NXnumtype &iType) {
  return NX5getattrainfo(handle, name, rank, dims, iType);
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetgroupID(NXhandle fileid, NXlink &sRes) {
  pNexusFile5 pFile;
  NXnumtype type = NXnumtype::CHAR;

  pFile = NXI5assert(fileid);
  if (pFile->iCurrentG == 0) {
    return NXstatus::NX_ERROR;
  } else {
    /*
       this means: if the item is already linked: use the target attribute, else
       the address to the current node
     */
    std::size_t datalen = 1024;
    char caddr[1024] = {0};
    if (NXgetattr(fileid, "target", caddr, datalen, type) != NXstatus::NX_OK) {
      sRes.targetAddress = buildCurrentAddress(pFile);
    } else {
      sRes.targetAddress = std::string(caddr);
    }
    sRes.linkType = NXentrytype::group;
    return NXstatus::NX_OK;
  }
  /* not reached */
  return NXstatus::NX_ERROR;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

NXstatus NXsameID(NXhandle fileid, NXlink const &pFirstID, NXlink const &pSecondID) {
  NXI5assert(fileid);
  if (pFirstID.targetAddress == pSecondID.targetAddress) {
    return NXstatus::NX_OK;
  } else {
    return NXstatus::NX_ERROR;
  }
}

/*-------------------------------------------------------------------------*/

NXstatus NXinitattrdir(NXhandle fid) {
  pNexusFile5 pFile;

  pFile = NXI5assert(fid);
  NXI5KillAttDir(pFile);
  return NXstatus::NX_OK;
}

NXstatus NXinitgroupdir(NXhandle fid) {
  pNexusFile5 pFile;

  pFile = NXI5assert(fid);
  NXI5KillDir(pFile);
  return NXstatus::NX_OK;
}

/*------------------------------------------------------------------------
  Implementation of NXopenaddress
  --------------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
NXstatus NXopenaddress(NXhandle fid, std::string const &address) {
  NXstatus status;
  int run = 1;
  std::string addressElement;
  if (fid == NULL || address.empty()) {
    NXReportError("ERROR: NXopendata needs both a file handle and a address string");
    return NXstatus::NX_ERROR;
  }

  std::string pPtr(moveDown(fid, address, status));
  if (status != NXstatus::NX_OK) {
    NXReportError("ERROR: NXopendata failed to move down in hierarchy");
    return status;
  }

  while (run == 1) {
    pPtr = extractNextAddress(pPtr, addressElement);
    status = stepOneUp(fid, addressElement);
    if (status != NXstatus::NX_OK) {
      return status;
    }
    if (pPtr.empty()) {
      run = 0;
    }
  }
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------*/
NXstatus NXopengroupaddress(NXhandle fid, std::string const &address) {
  NXstatus status;
  std::string addressElement;

  if (fid == nullptr || address.empty()) {
    NXReportError("ERROR: NXopengroupaddress needs both a file handle and a address string");
    return NXstatus::NX_ERROR;
  }

  std::string pPtr(moveDown(fid, address, status));
  if (status != NXstatus::NX_OK) {
    NXReportError("ERROR: NXopengroupaddress failed to move down in hierarchy");
    return status;
  }

  do {
    pPtr = extractNextAddress(pPtr, addressElement);
    status = stepOneGroupUp(fid, addressElement);
    if (status == NXstatus::NX_ERROR) {
      std::string msg("ERROR: NXopengroupaddress cannot reach address ");
      msg += address;
      NXReportError(msg.c_str());
      return NXstatus::NX_ERROR;
    }
  } while (!pPtr.empty() && status != NXstatus::NX_EOD);
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------*/
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

NXstatus NXgetnextattra(NXhandle fid, std::string &name, std::size_t &rank, Mantid::Nexus::DimVector &dims,
                        NXnumtype &iType) {
  return NX5getnextattra(fid, name, rank, dims, iType);
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

// cppcheck-suppress-end [constVariablePointer, constParameterReference, unusedVariable, unreadVariable]
// cppcheck-suppress-end [unmatchedSuppression]
