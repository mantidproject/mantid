/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format

  Application Program Interface Header File

  Copyright (C) 2000-2014 NeXus International Advisory Committee

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
/** \file
 * Documentation for the NeXus-API version 4.4.3
 * 2000-2011, the NeXus International Advisory Commitee
 * \defgroup c_main C API
 * \defgroup c_types Data Types
 * \ingroup c_main
 * \defgroup c_init General Initialisation and shutdown
 * \ingroup c_main
 * \defgroup c_group Reading and Writing Groups
 * \ingroup c_main
 * \defgroup c_readwrite Reading and Writing Data
 * \ingroup c_main
 * \defgroup c_navigation General File navigation
 * \ingroup c_main
 * \defgroup c_metadata Meta data routines
 * \ingroup c_main
 * \defgroup c_linking Linking
 * \ingroup c_main
 * \defgroup c_memory Memory allocation
 * \ingroup c_main
 * \defgroup c_external External linking
 * \ingroup c_main
 */
#pragma once

#include "MantidLegacyNexus/DllConfig.h"
#include "MantidLegacyNexus/NeXusFile_fwd.h"
#include <stdint.h>

/* NeXus HDF45 */
#define NEXUS_VERSION "4.4.3" /* major.minor.patch */

/*
 * Any new NXaccess_mode options should be numbered in 2^n format
 * (8, 16, 32, etc) so that they can be bit masked and tested easily.
 *
 * To test older non bit masked options (values below 8) use e.g.
 *
 *       if ( (mode & NXACCMASK_REMOVEFLAGS) == NXACC_CREATE )
 *
 * To test new (>=8) options just use normal bit masking e.g.
 *
 *       if ( mode & NXACC_NOSTRIP )
 *
 */
#define NXACCMASK_REMOVEFLAGS (0x7) /* bit mask to remove higher flag options */

#define NX_UNLIMITED -1

#define NX_MAXRANK 32
#define NX_MAXNAMELEN 64
#define NX_MAXPATHLEN 1024

/* Map NeXus compression methods to HDF compression methods */
#define NX_CHUNK 0
#define NX_COMP_NONE 100
#define NX_COMP_LZW 200
#define NX_COMP_RLE 300
#define NX_COMP_HUF 400

/* levels for deflate - to test for these we use ((value / 100) == NX_COMP_LZW) */
#define NX_COMP_LZW_LVL0 (100 * NX_COMP_LZW + 0)
#define NX_COMP_LZW_LVL1 (100 * NX_COMP_LZW + 1)
#define NX_COMP_LZW_LVL2 (100 * NX_COMP_LZW + 2)
#define NX_COMP_LZW_LVL3 (100 * NX_COMP_LZW + 3)
#define NX_COMP_LZW_LVL4 (100 * NX_COMP_LZW + 4)
#define NX_COMP_LZW_LVL5 (100 * NX_COMP_LZW + 5)
#define NX_COMP_LZW_LVL6 (100 * NX_COMP_LZW + 6)
#define NX_COMP_LZW_LVL7 (100 * NX_COMP_LZW + 7)
#define NX_COMP_LZW_LVL8 (100 * NX_COMP_LZW + 8)
#define NX_COMP_LZW_LVL9 (100 * NX_COMP_LZW + 9)

#define NXMAXSTACK 50

// name mangling macro
#define CONCAT(__a, __b) __a##__b /* token concatenation */
#define MANGLE(__arg) CONCAT(__arg, _)

#define NXopen MANGLE(nxiopen)
#define NXclose MANGLE(nxiclose)
#define NXopengroup MANGLE(nxiopengroup)
#define NXopenpath MANGLE(nxiopenpath)
#define NXgetpath MANGLE(nxigetpath)
#define NXopengrouppath MANGLE(nxiopengrouppath)
#define NXclosegroup MANGLE(nxiclosegroup)
#define NXopendata MANGLE(nxiopendata)
#define NXclosedata MANGLE(nxiclosedata)
#define NXgetdataID MANGLE(nxigetdataid)

#define NXgetinfo MANGLE(nxigetinfo)
#define NXgetinfo64 MANGLE(nxigetinfo64)
#define NXgetnextentry MANGLE(nxigetnextentry)
#define NXgetdata MANGLE(nxigetdata)

#define NXgetnextattr MANGLE(nxigetnextattr)
#define NXgetattr MANGLE(nxigetattr)
#define NXgetnextattra MANGLE(nxigetnextattra)
#define NXgetgroupID MANGLE(nxigetgroupid)
#define NXinitgroupdir MANGLE(nxiinitgroupdir)
#define NXinitattrdir MANGLE(nxiinitattrdir)

/*
 * Standard interface
 *
 * Functions added here are not automatically exported from
 * a shared library/dll - the symbol name must also be added
 * to the file   src/nexus_symbols.txt
 *
 */

/**
 * Open a NeXus file.
 * NXopen honours full path file names. But it also searches
 * for files in all the paths given in the NX_LOAD_PATH environment variable.
 * NX_LOAD_PATH is supposed to hold a list of path string separated by the platform
 * specific path separator. For unix this is the : , for DOS the ; . Please note
 * that crashing on an open NeXus file will result in corrupted data. Only after a NXclose
 * or a NXflush will the data file be valid.
 * \param filename The name of the file to open
 * \param access_method The file access method. This can be:
 * \li NXACC__READ read access
 * \li NXACC_RDWR read write access
 * \li NXACC_CREATE, NXACC_CREATE4 create a new HDF-4 NeXus file
 * \li NXACC_CREATE5 create a new HDF-5 NeXus file
 * \li NXACC_CREATEXML create an XML NeXus file.
 * see #NXaccess_mode
 * Support for HDF-4 is deprecated.
 * \param pHandle A file handle which will be initialized upon successfull completeion of NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_init
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXopen(Mantid::LegacyNexus::CONSTCHAR *filename,
                                                            Mantid::LegacyNexus::NXaccess access_method,
                                                            Mantid::LegacyNexus::NXhandle *pHandle);

/**
 * close a NeXus file
 * \param pHandle A NeXus file handle as returned from NXopen. pHandle is invalid after this
 * call.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_init
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXclose(Mantid::LegacyNexus::NXhandle *pHandle);

/**
 * Step into a group. All further access will be within the opened group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the group
 * \param NXclass the class name of the group. Should start with the prefix NX
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXopengroup(Mantid::LegacyNexus::NXhandle handle,
                                                                 Mantid::LegacyNexus::CONSTCHAR *name,
                                                                 Mantid::LegacyNexus::CONSTCHAR *NXclass);

/**
 * Open the NeXus object with the path specified
 * \param handle A NeXus file handle as returned from NXopen.
 * \param path A unix like path string to a NeXus group or dataset. The path string
 * is a list of group names and SDS names separated with / (slash).
 * Example: /entry1/sample/name
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXopenpath(Mantid::LegacyNexus::NXhandle handle,
                                                                Mantid::LegacyNexus::CONSTCHAR *path);

/**
 * Opens the group in which the NeXus object with the specified path exists
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param path A unix like path string to a NeXus group or dataset. The path string
 * is a list of group names and SDS names separated with / (slash).
 * Example: /entry1/sample/name
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXopengrouppath(Mantid::LegacyNexus::NXhandle handle,
                                                                     Mantid::LegacyNexus::CONSTCHAR *path);

/**
 * Retrieve the current path in the NeXus file
 * \param handle a NeXus file handle
 * \param path A buffer to copy the path too
 * \param  pathlen The maximum number of characters to copy into path
 * \return NX_OK or NX_ERROR
 * \ingroup c_navigation
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetpath(Mantid::LegacyNexus::NXhandle handle, char *path,
                                                               int pathlen);

/**
 * Closes the currently open group and steps one step down in the NeXus file
 * hierarchy.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXclosegroup(Mantid::LegacyNexus::NXhandle handle);

/**
 * Open access to a dataset. After this call it is possible to write and read data or
 * attributes to and from the dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param label The name of the dataset
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXopendata(Mantid::LegacyNexus::NXhandle handle,
                                                                Mantid::LegacyNexus::CONSTCHAR *label);

/**
 * Close access to a dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXclosedata(Mantid::LegacyNexus::NXhandle handle);

/**
 * Retrieve link data for a dataset. This link data can later on be used to link this
 * dataset into a different group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure which will be initialized with the required information
 * for linking.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetdataID(Mantid::LegacyNexus::NXhandle handle,
                                                                 Mantid::LegacyNexus::NXlink *pLink);

/**
 * Read a complete dataset from the currently open dataset into memory.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data A pointer to the memory area where to read the data, too. Data must point to a memory
 * area large enough to accomodate the data read. Otherwise your program may behave in unexpected
 * and unwelcome ways.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetdata(Mantid::LegacyNexus::NXhandle handle, void *data);

/**
 * Retrieve information about the curretly open dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param rank A pointer to an integer which will be filled with the rank of
 * the dataset.
 * \param dimension An array which will be initialized with the size of the dataset in any of its
 * dimensions. The array must have at least the size of rank.
 * \param datatype A pointer to an integer which be set to the NeXus data type code for this dataset.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_metadata
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus
NXgetinfo(Mantid::LegacyNexus::NXhandle handle, int *rank, int dimension[], Mantid::LegacyNexus::NXnumtype *datatype);

/**
 * @copydoc NXgetinfo()
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetinfo64(Mantid::LegacyNexus::NXhandle handle, int *rank,
                                                                 int64_t dimension[],
                                                                 Mantid::LegacyNexus::NXnumtype *datatype);

/**
 * Get the next entry in the currently open group. This is for retrieving infromation about the
 * content of a NeXus group. In order to search a group #NXgetnextentry is called in a loop until
 * #NXgetnextentry returns NX_EOD which indicates that there are no further items in the group.
 * Reset search using #NXinitgroupdir
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the object
 * \param nxclass The NeXus class name for a group or the string SDS for a dataset.
 * \param datatype The NeXus data type if the item is a SDS.
 * \return NX_OK on success, NX_ERROR in the case of an error, NX_EOD when there are no more items.
 * \ingroup c_navigation
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetnextentry(Mantid::LegacyNexus::NXhandle handle,
                                                                    Mantid::LegacyNexus::NXname name,
                                                                    Mantid::LegacyNexus::NXname nxclass,
                                                                    Mantid::LegacyNexus::NXnumtype *datatype);

/**
 * Read an attribute containing a single string or numerical value.
 * Use NXgetattra for attributes with dimensions. (Recommened as the general
 case.)
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the atrribute to read.
 * \param data A pointer to a memory area large enough to hold the attributes
 value.
 * \param iDataLen The length of data in bytes.
 * \param iType A pointer to an integer which will had been set to the NeXus
 data type of the attribute.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetattr(Mantid::LegacyNexus::NXhandle handle, const char *name,
                                                               void *data, int *iDataLen,
                                                               Mantid::LegacyNexus::NXnumtype *iType);

/**
 * Iterate over global, group or dataset attributes depending on the currently open group or
 * dataset. In order to search attributes multiple calls to #NXgetnextattr are performed in a loop
 * until #NXgetnextattr returns NX_EOD which indicates that there are no further attributes.
 * reset search using #NXinitattrdir
 * This allows for attributes with any dimensionality.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pName The name of the attribute
 * \param rank Rank of the attribute data.
 * \param dim Dimension array for the attribute content.
 * \param iType A pointer to an integer which be set to the NeXus data type of the attribute.
 * \return NX_OK on success, NX_ERROR in the case of an error, NX_EOD when there are no more items.
 * \ingroup c_readwrite
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetnextattra(Mantid::LegacyNexus::NXhandle handle,
                                                                    Mantid::LegacyNexus::NXname pName, int *rank,
                                                                    int dim[], Mantid::LegacyNexus::NXnumtype *iType);

/**
 * Retrieve link data for the currently open group. This link data can later on be used to link this
 * group into a different group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure which will be initialized with the required information
 * for linking.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXgetgroupID(Mantid::LegacyNexus::NXhandle handle,
                                                                  Mantid::LegacyNexus::NXlink *pLink);

/**
 * Resets a pending group search to the start again. To be called in a #NXgetnextentry loop when
 * a group search has to be restarted.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXinitgroupdir(Mantid::LegacyNexus::NXhandle handle);

/**
 * Resets a pending attribute search to the start again. To be called in a #NXgetnextattr loop when
 * an attribute search has to be restarted.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_LEGACYNEXUS_DLL Mantid::LegacyNexus::NXstatus NXinitattrdir(Mantid::LegacyNexus::NXhandle handle);

/** \typedef void (*ErrFunc)(void *data, const char *text)
 * All NeXus error reporting happens through this special function, the
 * ErrFunc. The NeXus-API allows this error reporting function to be replaced
 * through a user defined implementation. The default error function prints to stderr. User
 * defined ones may pop up dialog boxes or whatever.
 * \param data A pointer to some user defined data structure
 * \param text The text of the error message to display.
 */
typedef void (*ErrFunc)(void *data, const char *text);

/**
 * Suppress error reports from the NeXus-API
 */
MANTID_LEGACYNEXUS_DLL void NXMDisableErrorReporting();

/**
 * Enable error reports from the NeXus-API
 */
MANTID_LEGACYNEXUS_DLL void NXMEnableErrorReporting();

/**
 * Dispatches the error message to the error function defined by NXMSetTError
 */
MANTID_LEGACYNEXUS_DLL void NXReportError(const char *text);

/* extern void *NXpData; */
MANTID_LEGACYNEXUS_DLL char *NXIformatNeXusTime();

/**
 * Freddie Akeroyd 11/8/2009
 * Add NeXus schema support - this uses BASE.xsd as the initial file
 */
#define NEXUS_SCHEMA_VERSION "3.1"                                    /**< version of NeXus definition schema */
#define NEXUS_SCHEMA_ROOT "http://definition.nexusformat.org/schema/" /**< XML schema namespace specified by xmlns */
#define NEXUS_SCHEMA_NAMESPACE NEXUS_SCHEMA_ROOT NEXUS_SCHEMA_VERSION /**< XML schema namespace specified by xmlns */
#define NEXUS_SCHEMA_BASE "BASE"
#define NEXUS_SCHEMA_FILE NEXUS_SCHEMA_BASE ".xsd" /**< default schema file for namespace */
/** location of default schema file for namespace */
#define NEXUS_SCHEMA_URL NEXUS_SCHEMA_NAMESPACE "/" NEXUS_SCHEMA_FILE
