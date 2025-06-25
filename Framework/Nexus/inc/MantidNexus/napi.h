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

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"
#include <stdint.h>

/* NeXus HDF45 */
#define NEXUS_VERSION "4.4.3" /* major.minor.patch */

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
 * \li READ read access
 * \li RDWR read write access
 * \li CREATE5 create a new HDF-5 NeXus file
 * see #NXaccess
 * Support for HDF-4 is deprecated.
 * \param handle A file handle which will be initialized upon successfull completeion of NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_init
 */
MANTID_NEXUS_DLL NXstatus NXopen(std::string const &filename, NXaccess const access_method, NXhandle &handle);

/**
 * close a NeXus file
 * \param pHandle A NeXus file handle as returned from NXopen. pHandle is invalid after this
 * call.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_init
 */
MANTID_NEXUS_DLL NXstatus NXclose(NXhandle &handle);

/**
 * flush data to disk
 * \param pHandle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXflush(NXhandle &handle);

/**
 * NeXus groups are NeXus way of structuring information into a hierarchy.
 * This function creates a group but does not open it.
 * \param handle A NeXus file handle as initialized NXopen.
 * \param name The name of the group
 * \param NXclass the class name of the group. Should start with the prefix NX
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUS_DLL NXstatus NXmakegroup(NXhandle handle, std::string const &name, std::string const &NXclass);

/**
 * Step into a group. All further access will be within the opened group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the group
 * \param NXclass the class name of the group. Should start with the prefix NX
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUS_DLL NXstatus NXopengroup(NXhandle handle, std::string const &name, std::string const &NXclass);

/**
 * Open the NeXus object with the address specified
 * \param handle A NeXus file handle as returned from NXopen.
 * \param address A unix like address string to a NeXus group or dataset. The address string
 * is a list of group names and SDS names separated with / (slash).
 * Example: /entry1/sample/name
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUS_DLL NXstatus NXopenaddress(NXhandle handle, std::string const &address);

/**
 * Opens the group in which the NeXus object with the specified address exists
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param address A unix like address string to a NeXus group or dataset. The address string
 * is a list of group names and SDS names separated with / (slash).
 * Example: /entry1/sample/name
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUS_DLL NXstatus NXopengroupaddress(NXhandle handle, std::string const &address);

/**
 * Retrieve the current address in the NeXus file
 * \param handle a NeXus file handle
 * \param address A string into which to copy the address
 * \return NX_OK or NX_ERROR
 * \ingroup c_navigation
 */
MANTID_NEXUS_DLL NXstatus NXgetaddress(NXhandle handle, std::string &address);

/**
 * Closes the currently open group and steps one step down in the NeXus file
 * hierarchy.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUS_DLL NXstatus NXclosegroup(NXhandle handle);

/**
 * Create a multi dimensional data array or dataset. The dataset is NOT opened.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the dataset
 * \param datatype The data type of this data set.
 * \param rank The number of dimensions this dataset is going to have
 * \param dim An array of size rank holding the size of the dataset in each dimension. The first dimension
 * can be NX_UNLIMITED. Data can be appended to such a dimension using NXputslab.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXmakedata64(NXhandle handle, std::string const &name, NXnumtype const datatype,
                                       std::size_t const rank, Mantid::Nexus::DimVector const &dims);

/**
 * Create a compressed dataset. The dataset is NOT opened. Data from this set will automatically be compressed when
 * writing and decompressed on reading.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the dataset
 * \param datatype The data type of this data set.
 * \param rank The number of dimensions this dataset is going to have
 * \param comp_typ The compression scheme to use. Possible values:
 * \li NONE no compression
 * \li LZW (recommended) despite the name this enabled zlib compression (of various levels, see above)
 * \li RLE run length encoding (only HDF-4)
 * \li HUF Huffmann encoding (only HDF-4)
 * \param dim An array of size rank holding the size of the dataset in each dimension. The first dimension
 * can be NX_UNLIMITED. Data can be appended to such a dimension using NXputslab.
 * \param bufsize The dimensions of the subset of the data which usually be writen in one go.
 * This is a parameter used by HDF for performance optimisations. If you write your data in one go, this
 * should be the same as the data dimension. If you write it in slabs, this is your preferred slab size.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXcompmakedata64(NXhandle handle, std::string const &name, NXnumtype const datatype,
                                           std::size_t const rank, Mantid::Nexus::DimVector const &dims,
                                           NXcompression const comp_typ,
                                           Mantid::Nexus::DimSizeVector const &chunk_size);

/**
 * Open access to a dataset. After this call it is possible to write and read data or
 * attributes to and from the dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the dataset
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXopendata(NXhandle handle, std::string const &name);

/**
 * Close access to a dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXclosedata(NXhandle handle);

/**
 * Write data to a datset which has previouly been opened with NXopendata.
 * This writes all the data in one go. Data should be a pointer to a memory
 * area matching the datatype and dimensions of the dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data Pointer to data to write.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXputdata(NXhandle handle, const void *data);

/**
 * Write an attribute. The kind of attribute written depends on the
 * position in the file: at root level, a global attribute is written, if
 * agroup is open but no dataset, a group attribute is written, if a dataset is
 * open, a dataset attribute is written.
 * This type of attribute can only contain a signle numerical value or a
 * single string. For multiple values use NXputattra
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the attribute.
 * \param data A pointer to the data to write for the attribute.
 * \param iDataLen The length of the data in data in bytes.
 * \param iType The NeXus data type of the attribute.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXputattr(NXhandle handle, std::string const &name, const void *data,
                                    std::size_t const iDataLen, NXnumtype const iType);

/**
 * Write  a subset of a multi dimensional dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data A pointer to a memory area holding the data to write.
 * \param start An array holding the start indices where to start the data subset.
 * \param size An array holding the size of the data subset to write in each dimension.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXputslab64(NXhandle handle, const void *data, Mantid::Nexus::DimSizeVector const &start,
                                      Mantid::Nexus::DimSizeVector const &size);

/**
 * Retrieve link data for a dataset. This link data can later on be used to link this
 * dataset into a different group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure which will be initialized with the required information
 * for linking.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_NEXUS_DLL NXstatus NXgetdataID(NXhandle handle, NXlink &pLink);

/**
 * Create a link to the group or dataset described by pLink in the currently open
 * group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure describing the object to link. This must have been initialized
 * by either a call to NXgetdataID or NXgetgroupID.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_NEXUS_DLL NXstatus NXmakelink(NXhandle handle, NXlink const &pLink);

/**
 * Read a complete dataset from the currently open dataset into memory.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data A pointer to the memory area where to read the data, too. Data must point to a memory
 * area large enough to accomodate the data read. Otherwise your program may behave in unexpected
 * and unwelcome ways.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXgetdata(NXhandle handle, void *data);

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
MANTID_NEXUS_DLL NXstatus NXgetinfo64(NXhandle handle, std::size_t &rank, Mantid::Nexus::DimVector &dimension,
                                      NXnumtype &datatype);

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
MANTID_NEXUS_DLL NXstatus NXgetnextentry(NXhandle handle, std::string &name, std::string &nxclass, NXnumtype &datatype);

/**
 * Read a subset of data from file into memory.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data A pointer to the memory data where to copy the data too. The pointer must point
 * to a memory area large enough to accomodate the size of the data read.
 * \param start An array holding the start indices where to start reading the data subset.
 * \param size An array holding the size of the data subset to read for each dimension.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXgetslab64(NXhandle handle, void *data, Mantid::Nexus::DimSizeVector const &start,
                                      Mantid::Nexus::DimSizeVector const &size);

/**
 * Read an attribute containing a single string or numerical value.
 * Use NXgetattra for attributes with dimensions. (Recommened as the general case.)
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the atrribute to read.
 * \param data A pointer to a memory area large enough to hold the attributes value.
 * \param iDataLen The length of data in bytes.
 * \param iType A pointer to an integer which will had been set to the NeXus data type of the attribute.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXgetattr(NXhandle handle, std::string const &name, void *data, std::size_t &iDataLen,
                                    NXnumtype &iType);

/**
 * Get the count of attributes in the currently open dataset, group or global attributes when at root level.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param no_items A pointer to an integer which be set to the number of attributes available.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_metadata
 */
MANTID_NEXUS_DLL NXstatus NXgetattrinfo(NXhandle handle, std::size_t &no_items);

MANTID_NEXUS_DLL NXstatus NXgetattrainfo(NXhandle handle, std::string const &name, std::size_t &rank,
                                         Mantid::Nexus::DimVector &dim, NXnumtype &iType);

/**
 * Iterate over global, group or dataset attributes depending on the currently open group or
 * dataset. In order to search attributes multiple calls to #NXgetnextattr are performed in a loop
 * until #NXgetnextattr returns NX_EOD which indicates that there are no further attributes.
 * reset search using #NXinitattrdir
 * This allows for attributes with any dimensionality.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the attribute
 * \param rank Rank of the attribute data.
 * \param dim Dimension array for the attribute content.
 * \param iType A pointer to an integer which will be set to the NeXus data type of the attribute.
 * \return NX_OK on success, NX_ERROR in the case of an error, NX_EOD when there are no more items.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXgetnextattra(NXhandle handle, std::string &name, std::size_t &rank,
                                         Mantid::Nexus::DimVector &dims, NXnumtype &iType);

/**
 * Retrieve link data for the currently open group. This link data can later on be used to link this
 * group into a different group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure which will be initialized with the required information
 * for linking.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_NEXUS_DLL NXstatus NXgetgroupID(NXhandle handle, NXlink &pLink);

/**
 * Retrieve information about the currently open group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param no_items A pointer to an integer which will be set to the count
 *   of group elements available. This is the count of other groups and
 * data sets in this group.
 * \param name The name of the group.
 * \param nxclass The NeXus class name of the group.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_metadata
 */
MANTID_NEXUS_DLL NXstatus NXgetgroupinfo(NXhandle handle, std::size_t &no_items, std::string &name,
                                         std::string &nxclass);

/**
 * Tests if two link data structures describe the same item.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pFirstID The first link data for the test.
 * \param pSecondID The second link data structure.
 * \return NX_OK when both link data structures describe the same item, NX_ERROR else.
 * \ingroup c_linking
 */
MANTID_NEXUS_DLL NXstatus NXsameID(NXhandle handle, NXlink const &pFirstID, NXlink const &pSecondID);

/**
 * Resets a pending group search to the start again. To be called in a #NXgetnextentry loop when
 * a group search has to be restarted.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUS_DLL NXstatus NXinitgroupdir(NXhandle handle);

/**
 * Resets a pending attribute search to the start again. To be called in a #NXgetnextattr loop when
 * an attribute search has to be restarted.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUS_DLL NXstatus NXinitattrdir(NXhandle handle);

/**
 * Utility function which allocates a suitably sized memory area for the dataset characteristics specified.
 * \param data A pointer to a pointer which will be initialized with a pointer to a suitably sized memory area.
 * \param rank the rank of the data.
 * \param dims An array holding the size of the data in each dimension.
 * \param datatype The NeXus data type of the data.
 * \return NX_OK when allocation succeeds, NX_ERROR in the case of an error.
 * \ingroup c_memory
 */
MANTID_NEXUS_DLL NXstatus NXmalloc64(void *&data, std::size_t rank, Mantid::Nexus::DimVector const &dims,
                                     NXnumtype datatype);

/**
 * Utility function to release the memory for data.
 * \param data A pointer to a pointer to free.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_memory
 */
MANTID_NEXUS_DLL NXstatus NXfree(void **data);

/**
 * Dispatches the error message
 */
MANTID_NEXUS_DLL void NXReportError(const char *text);

/* extern void *NXpData; */
MANTID_NEXUS_DLL char *NXIformatNeXusTime();
