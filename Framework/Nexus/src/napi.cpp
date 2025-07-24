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

#include "MantidNexus/NexusException.h"
#include "MantidNexus/NexusFile.h"
#include "MantidNexus/napi.h"

// cppcheck-suppress-begin [unmatchedSuppression, variableScope]
// cppcheck-suppress-begin [constVariablePointer, constParameterReference, unusedVariable, unreadVariable]
// cppcheck-suppress-begin [nullPointerArithmeticOutOfMemory, nullPointerOutOfMemory]

// this has to be after the other napi includes
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
/*----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/* ------------------------------------------------------------------------- */
/*-----------------------------------------------------------------------*/

NXstatus NXmakegroup(NXhandle fid, std::string const &name, std::string const &nxclass) {
  try {
    fid.makeGroup(name, nxclass, false);
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/*------------------------------------------------------------------------*/

NXstatus NXopengroup(NXhandle fid, std::string const &name, std::string const &nxclass) {
  try {
    fid.openGroup(name, nxclass);
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NXclosegroup(NXhandle fid) {
  try {
    fid.closeGroup();
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* --------------------------------------------------------------------- */

NXstatus NXmakedata64(NXhandle fid, std::string const &name, NXnumtype const datatype, std::size_t const rank,
                      Mantid::Nexus::DimVector const &dims) {
  UNUSED_ARG(rank);
  try {
    fid.makeData(name, datatype, dims, false);
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* --------------------------------------------------------------------- */

NXstatus NXopendata(NXhandle fid, std::string const &name) {
  try {
    fid.openData(name);
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ----------------------------------------------------------------- */

NXstatus NXclosedata(NXhandle fid) {
  try {
    fid.closeData();
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NXputdata(NXhandle fid, char const *data) {
  try {
    fid.putData(std::string(data));
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ------------------------------------------------------------------- */

NXstatus NXputattr(NXhandle fid, std::string const &name, char const *data, std::size_t const datalen,
                   NXnumtype const iType) {
  UNUSED_ARG(datalen);
  UNUSED_ARG(iType);
  try {
    std::string value(data);
    fid.putAttr(name, value);
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/* ---- --------------------------------------------------------------- */
/* ------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*----------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* --------------------------------------------------------------------- */
/*----------------------------------------------------------------------*/
/*
**  TRIM.C - Remove leading, trailing, & excess embedded spaces
**
**  public domain by Bob Stout
*/

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
    str[++i] = '\0';
  }
  return str;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetdata(NXhandle fid, char *data) {
  try {
    fid.getData(data);
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/

NXstatus NXgetinfo64(NXhandle fid, std::size_t &rank, Mantid::Nexus::DimVector &dims, NXnumtype &iType) {
  try {
    Mantid::Nexus::Info info(fid.getInfo());
    rank = info.dims.size();
    dims = info.dims;
    iType = info.type;
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

// cppcheck-suppress constParameterCallback
NXstatus NXgetattr(NXhandle fid, std::string const &name, char *data, std::size_t &datalen, NXnumtype &iType) {
  // NOTE this is written to mimic NXgetattr with readStringAttributeN
  // readStringAttribute fetched a string value with correct handling of the length
  // readStringAttributeN took that value and copied it with `strncpy(data, vdat, maxlen)`
  // then for some reason set `data[maxlen - 1] = '\0'`, which truncated the result by one char
  // and then NXgetattr set `datalen` to the `strlen` of the truncated result
  try {
    std::string value = fid.getStrAttr(name); // get correct string value
    strncpy(data, value.c_str(), datalen);    // copy first datalen chars
    data[datalen - 1] = '\0';                 // set a null terminator
    datalen = strlen(data);                   // changevalue of datalen
    iType = NXnumtype::CHAR;
  } catch (Mantid::Nexus::Exception const &) {
    return NXstatus::NX_ERROR;
  }
  return NXstatus::NX_OK;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*------------------------------------------------------------------------
  Implementation of NXopenaddress
  --------------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
NXstatus NXopenaddress(NexusFile5 &fid, std::string const &address) {
  std::string addressElement;
  if (address.empty()) {
    NXReportError("ERROR: NXopenaddress needs both a file handle and a address string");
    return NXstatus::NX_ERROR;
  }

  // establish the new address absolutely
  Mantid::Nexus::NexusAddress absaddr(address);
  if (!absaddr.isAbsolute()) {
    absaddr = fid.groupaddr / address;
  }

  // if we are already there, do nothing
  if (absaddr == buildCurrentAddress(fid)) {
    return NXstatus::NX_OK;
  }

  // go all the way down to the root
  // if a dataset is open then close it
  if (fid.iCurrentD != 0) {
    H5Dclose(fid.iCurrentD);
    H5Sclose(fid.iCurrentS);
    H5Tclose(fid.iCurrentT);
    fid.iCurrentD = 0;
    fid.iCurrentS = 0;
    fid.iCurrentT = 0;
  }
  // now close all the groups in the stack
  for (hid_t gid : fid.iStack5) {
    if (gid != 0) {
      H5Gclose(gid);
    }
  }
  fid.iStack5.clear();
  // reset to the root condition
  fid.iStack5.push_back(0);
  fid.groupaddr = Mantid::Nexus::NexusAddress::root();

  // if we wanted to go to root, then stop here
  if (absaddr == Mantid::Nexus::NexusAddress::root()) {
    fid.iCurrentG = 0;
    return NXstatus::NX_OK;
  }

  // build new address
  Mantid::Nexus::NexusAddress up(absaddr.parent_path());
  Mantid::Nexus::NexusAddress fromroot;
  // open groups up the address
  if (up.isRoot()) {
    fid.iCurrentG = 0;
  } else {
    fid.iCurrentG = fid.iFID;
    for (auto const &name : up.parts()) {
      fromroot /= name;
      hid_t gid = H5Gopen(fid.iFID, fromroot.c_str(), H5P_DEFAULT);
      if (gid < 0) {
        return NXstatus::NX_ERROR;
      }
      fid.iStack5.push_back(gid);
      fid.iCurrentG = gid;
      fid.groupaddr = fromroot;
    }
  }
  // now open the last element -- either a group or a dataset
  H5O_info2_t op_data;
  H5Oget_info_by_name(fid.iFID, absaddr.c_str(), &op_data, H5O_INFO_BASIC, H5P_DEFAULT);
  if (op_data.type == H5O_TYPE_GROUP) {
    hid_t gid = H5Gopen(fid.iFID, absaddr.c_str(), H5P_DEFAULT);
    if (gid < 0) {
      return NXstatus::NX_ERROR;
    }
    fid.iStack5.push_back(gid);
    fid.iCurrentG = gid;
  } else if (op_data.type == H5O_TYPE_DATASET) {
    fid.iCurrentD = H5Dopen(fid.iFID, absaddr.c_str(), H5P_DEFAULT);
    fid.iCurrentT = H5Dget_type(fid.iCurrentD);
    fid.iCurrentS = H5Dget_space(fid.iCurrentD);
  } else {
    return NXstatus::NX_ERROR;
  }
  // now set the address
  if (fid.iCurrentG == 0) {
    fid.groupaddr = Mantid::Nexus::NexusAddress::root();
  } else {
    fid.groupaddr = getObjectAddress(fid.iCurrentG);
  }
  return NXstatus::NX_OK;
}

/*---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*--------------------------------------------------------------------
  format NeXus time. Code needed in every NeXus file driver
  ---------------------------------------------------------------------*/
std::string NXIformatNeXusTime() {
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
  std::string res(time_buffer);
  free(time_buffer);
  return res;
}

// cppcheck-suppress-end [nullPointerArithmeticOutOfMemory, nullPointerOutOfMemory]
// cppcheck-suppress-end [constVariablePointer, constParameterReference, unusedVariable, unreadVariable]
// cppcheck-suppress-end [unmatchedSuppression, variableScope]
