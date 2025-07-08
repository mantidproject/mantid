#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"
#include "MantidNexus/napi.h"

#include <hdf5.h>

MANTID_NEXUS_DLL pNexusFile5 NXI5assert(NXhandle fid);

MANTID_NEXUS_DLL void NXI5KillDir(pNexusFile5 self);

MANTID_NEXUS_DLL herr_t readStringAttribute(hid_t attr, char **data);

MANTID_NEXUS_DLL herr_t readStringAttributeN(hid_t attr, char *data, std::size_t maxlen);

MANTID_NEXUS_DLL void NXI5KillAttDir(pNexusFile5 self);

MANTID_NEXUS_DLL std::string buildCurrentAddress(pNexusFile5 fid);

MANTID_NEXUS_DLL hid_t getAttVID(pNexusFile5 pFile);

MANTID_NEXUS_DLL void killAttVID(const pNexusFile5 pFile, hid_t vid);

MANTID_NEXUS_DLL NXstatus NX5settargetattribute(pNexusFile5 pFile, NXlink const &sLink);

MANTID_NEXUS_DLL NXnumtype hdf5ToNXType(H5T_class_t tclass, hid_t atype);

MANTID_NEXUS_DLL hid_t nxToHDF5Type(NXnumtype datatype);

MANTID_NEXUS_DLL hid_t h5MemType(hid_t atype);

MANTID_NEXUS_DLL herr_t attr_check(hid_t loc_id, const char *member_name, const H5A_info_t *unused, void *opdata);

/*------------------------------------------------------------------------
  Implementation of NXopenaddress
  --------------------------------------------------------------------------*/
MANTID_NEXUS_DLL int isDataSetOpen(NXhandle hfil);
MANTID_NEXUS_DLL int isRoot(NXhandle hfil);
MANTID_NEXUS_DLL std::string extractNextAddress(std::string const &address, std::string &element);
MANTID_NEXUS_DLL NXstatus gotoRoot(NXhandle hfil);
MANTID_NEXUS_DLL int isRelative(std::string const &address);
MANTID_NEXUS_DLL NXstatus moveOneDown(NXhandle hfil);
MANTID_NEXUS_DLL std::string moveDown(NXhandle hfil, std::string const &address, NXstatus &code);
MANTID_NEXUS_DLL NXstatus stepOneUp(NXhandle hfil, std::string const &name);
MANTID_NEXUS_DLL NXstatus stepOneGroupUp(NXhandle hfil, std::string const &name);

/*---------------------------------------------------------------------
 * private functions used in NX5open
 */

hid_t create_file_access_plist(std::string const &filename);
herr_t set_str_attribute(hid_t parent_id, std::string const &name, std::string const &buffer);
