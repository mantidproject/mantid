#include <string.h>

#include "MantidNexus/NexusException.h"
#include "MantidNexus/NexusFile.h"
#include "MantidNexus/inverted_napi.h"

/* ----------------------------------------------------------------------

   This is the inverted NAPI
   The remaining functions are simply shims to call Nexus::File
   They exist for no other reason than the tests in NapiUnutTest to run
   Those tests persist to document how string data and attributes were
   formerly loaded, in case a manual test raises a regression

   --------------------------------------------------------------------- */

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

/*--------------------------------------------------------------------
  Ah yeah, everyone! It's NeXus Time! B-)
  Chill out, relax, and let's party like it's stil 1989
  ---------------------------------------------------------------------*/
