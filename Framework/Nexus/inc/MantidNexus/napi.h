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

#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"
#include <stdint.h>

/**
 * NeXus groups are NeXus way of structuring information into a hierarchy.
 * This function creates a group but does not open it.
 * \param handle A NeXus file handle as initialized NXopen.
 * \param name The name of the group
 * \param NXclass the class name of the group. Should start with the prefix NX
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUS_DLL NXstatus NXmakegroup(NXhandle fid, std::string const &name, std::string const &nxclass);

/**
 * Step into a group. All further access will be within the opened group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the group
 * \param NXclass the class name of the group. Should start with the prefix NX
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUS_DLL NXstatus NXopengroup(NXhandle fid, std::string const &name, std::string const &nxclass);

/**
 * Open the NeXus object with the address specified
 * \param handle A NeXus file handle as returned from NXopen.
 * \param address A unix like address string to a NeXus group or dataset. The address string
 * is a list of group names and SDS names separated with / (slash).
 * Example: /entry1/sample/name
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUS_DLL NXstatus NXopenaddress(NexusFile5 &fid, std::string const &address);

/**
 * Closes the currently open group and steps one step down in the NeXus file
 * hierarchy.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUS_DLL NXstatus NXclosegroup(NXhandle fid);

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
MANTID_NEXUS_DLL NXstatus NXmakedata64(NXhandle fid, std::string const &name, NXnumtype const datatype,
                                       std::size_t const rank, Mantid::Nexus::DimVector const &dims);

/**
 * Open access to a dataset. After this call it is possible to write and read data or
 * attributes to and from the dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the dataset
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXopendata(NXhandle fid, std::string const &name);

/**
 * Close access to a dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXclosedata(NXhandle fid);

/**
 * Write data to a datset which has previouly been opened with NXopendata.
 * This writes all the data in one go. Data should be a pointer to a memory
 * area matching the datatype and dimensions of the dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data Pointer to data to write.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXputdata(NXhandle fid, char const *data);

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
MANTID_NEXUS_DLL NXstatus NXputattr(NXhandle fid, std::string const &name, char const *data, std::size_t const iDataLen,
                                    NXnumtype const iType);

/**
 * Read a complete dataset from the currently open dataset into memory.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data A pointer to the memory area where to read the data, too. Data must point to a memory
 * area large enough to accomodate the data read. Otherwise your program may behave in unexpected
 * and unwelcome ways.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUS_DLL NXstatus NXgetdata(NXhandle fid, char *data);

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
MANTID_NEXUS_DLL NXstatus NXgetinfo64(NXhandle fid, std::size_t &rank, Mantid::Nexus::DimVector &dimension,
                                      NXnumtype &datatype);

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
MANTID_NEXUS_DLL NXstatus NXgetattr(NXhandle fid, std::string const &name, char *data, std::size_t &iDataLen,
                                    NXnumtype &iType);

/**
 * Dispatches the error message
 */
MANTID_NEXUS_DLL void NXReportError(const char *text);

/* extern void *NXpData; */
MANTID_NEXUS_DLL std::string NXIformatNeXusTime();
