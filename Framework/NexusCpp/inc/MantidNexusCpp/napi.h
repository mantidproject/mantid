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
#ifndef NEXUSAPI
#define NEXUSAPI

#include "MantidNexusCpp/DllConfig.h"
#include <stdint.h>

/* NeXus HDF45 */
#define NEXUS_VERSION "4.4.3" /* major.minor.patch */

#define CONSTCHAR const char

typedef void *NXhandle; /* really a pointer to a NexusFile structure */
typedef int NXstatus;
typedef char NXname[128];

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

/** \enum NXaccess_mode
 * NeXus file access codes.
 * \li NXACC_READ read-only
 * \li NXACC_RDWR open an existing file for reading and writing.
 * \li NXACC_CREATE create a NeXus HDF-4 file
 * \li NXACC_CREATE4 create a NeXus HDF-4 file
 * \li NXACC_CREATE5 create a NeXus HDF-5 file.
 * \li NXACC_CREATEXML create a NeXus XML file.
 * \li NXACC_CHECKNAMESYNTAX Check names conform to NeXus allowed characters.
 */
typedef enum {
  NXACC_READ = 1,
  NXACC_RDWR = 2,
  NXACC_CREATE = 3,
  NXACC_CREATE4 = 4,
  NXACC_CREATE5 = 5,
  NXACC_CREATEXML = 6,
  NXACC_TABLE = 8,
  NXACC_NOSTRIP = 128,
  NXACC_CHECKNAMESYNTAX = 256
} NXaccess_mode;

/**
 * A combination of options from #NXaccess_mode
 */
typedef int NXaccess;

typedef struct {
  char *iname;
  int type;
} info_type, *pinfo;

#define NX_OK 1
#define NX_ERROR 0
#define NX_EOD -1

#define NX_UNLIMITED -1

#define NX_MAXRANK 32
#define NX_MAXNAMELEN 64
#define NX_MAXPATHLEN 1024

/**
 * \ingroup c_types
 * \def NX_FLOAT32
 * 32 bit float
 * \def NX_FLOAT64
 * 64 bit float == double
 * \def NX_INT8
 * 8 bit integer == byte
 * \def NX_UINT8
 * 8 bit unsigned integer
 * \def NX_INT16
 * 16 bit integer
 * \def NX_UINT16
 * 16 bit unsigned integer
 * \def NX_INT32
 * 32 bit integer
 * \def NX_UINT32
 * 32 bit unsigned integer
 * \def NX_CHAR
 * 8 bit character
 * \def NX_BINARY
 * lump of binary data == NX_UINT8
 */
/*--------------------------------------------------------------------------*/

/* Map NeXus to HDF types */
#define NX_FLOAT32 5
#define NX_FLOAT64 6
#define NX_INT8 20
#define NX_UINT8 21
#define NX_BOOLEAN NX_UINT
#define NX_INT16 22
#define NX_UINT16 23
#define NX_INT32 24
#define NX_UINT32 25
#define NX_INT64 26
#define NX_UINT64 27
#define NX_CHAR 4
#define NX_BINARY 21

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

typedef struct {
  long iTag;             /* HDF4 variable */
  long iRef;             /* HDF4 variable */
  char targetPath[1024]; /* path to item to link */
  int linkType;          /* HDF5: 0 for group link, 1 for SDS link */
} NXlink;

#define NXMAXSTACK 50

// name mangling macro
#define CONCAT(__a, __b) __a##__b /* token concatenation */
#define MANGLE(__arg) CONCAT(__arg, _)

#define NXopen MANGLE(nxiopen)
#define NXreopen MANGLE(nxireopen)
#define NXclose MANGLE(nxiclose)
#define NXmakegroup MANGLE(nximakegroup)
#define NXopengroup MANGLE(nxiopengroup)
#define NXopenpath MANGLE(nxiopenpath)
#define NXgetpath MANGLE(nxigetpath)
#define NXopengrouppath MANGLE(nxiopengrouppath)
#define NXclosegroup MANGLE(nxiclosegroup)
#define NXmakedata MANGLE(nximakedata)
#define NXmakedata64 MANGLE(nximakedata64)
#define NXcompmakedata MANGLE(nxicompmakedata)
#define NXcompmakedata64 MANGLE(nxicompmakedata64)
#define NXcompress MANGLE(nxicompress)
#define NXopendata MANGLE(nxiopendata)
#define NXclosedata MANGLE(nxiclosedata)
#define NXputdata MANGLE(nxiputdata)
#define NXputslab MANGLE(nxiputslab)
#define NXputslab64 MANGLE(nxiputslab64)
#define NXputattr MANGLE(nxiputattr)
#define NXputattra MANGLE(nxiputattra)
#define NXgetdataID MANGLE(nxigetdataid)
#define NXmakelink MANGLE(nximakelink)
#define NXmakenamedlink MANGLE(nximakenamedlink)
#define NXopensourcegroup MANGLE(nxiopensourcegroup)
#define NXmalloc MANGLE(nximalloc)
#define NXmalloc64 MANGLE(nximalloc64)
#define NXfree MANGLE(nxifree)
#define NXflush MANGLE(nxiflush)

#define NXgetinfo MANGLE(nxigetinfo)
#define NXgetinfo64 MANGLE(nxigetinfo64)
#define NXgetrawinfo MANGLE(nxigetrawinfo)
#define NXgetrawinfo64 MANGLE(nxigetrawinfo64)
#define NXgetnextentry MANGLE(nxigetnextentry)
#define NXgetdata MANGLE(nxigetdata)

#define NXgetslab MANGLE(nxigetslab)
#define NXgetslab64 MANGLE(nxigetslab64)
#define NXgetnextattr MANGLE(nxigetnextattr)
#define NXgetattr MANGLE(nxigetattr)
#define NXgetnextattra MANGLE(nxigetnextattra)
#define NXgetattra MANGLE(nxigetattra)
#define NXgetattrinfo MANGLE(nxigetattrinfo)
#define NXgetattrainfo MANGLE(nxigetattrainfo)
#define NXgetgroupID MANGLE(nxigetgroupid)
#define NXgetgroupinfo MANGLE(nxigetgroupinfo)
#define NXsameID MANGLE(nxisameid)
#define NXinitgroupdir MANGLE(nxiinitgroupdir)
#define NXinitattrdir MANGLE(nxiinitattrdir)
#define NXsetnumberformat MANGLE(nxisetnumberformat)
#define NXsetcache MANGLE(nxisetcache)
#define NXinquirefile MANGLE(nxiinquirefile)
#define NXisexternalgroup MANGLE(nxiisexternalgroup)
#define NXisexternaldataset MANGLE(nxiisexternaldataset)
#define NXlinkexternal MANGLE(nxilinkexternal)
#define NXlinkexternaldataset MANGLE(nxilinkexternaldataset)
#define NXgetversion MANGLE(nxigetversion)

/*
 * Standard interface
 *
 * Functions added here are not automatically exported from
 * a shared library/dll - the symbol name must also be added
 * to the file   src/nexus_symbols.txt
 *
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
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
MANTID_NEXUSCPP_DLL NXstatus NXopen(CONSTCHAR *filename, NXaccess access_method, NXhandle *pHandle);

/**
 * Opens an existing NeXus file a second time for e.g. access from another thread.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_init
 */
MANTID_NEXUSCPP_DLL NXstatus NXreopen(NXhandle pOrigHandle, NXhandle *pNewHandle);

/**
 * close a NeXus file
 * \param pHandle A NeXus file handle as returned from NXopen. pHandle is invalid after this
 * call.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_init
 */
MANTID_NEXUSCPP_DLL NXstatus NXclose(NXhandle *pHandle);

/**
 * flush data to disk
 * \param pHandle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXflush(NXhandle *pHandle);

/**
 * NeXus groups are NeXus way of structuring information into a hierarchy.
 * This function creates a group but does not open it.
 * \param handle A NeXus file handle as initialized NXopen.
 * \param name The name of the group
 * \param NXclass the class name of the group. Should start with the prefix NX
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUSCPP_DLL NXstatus NXmakegroup(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);

/**
 * Step into a group. All further access will be within the opened group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the group
 * \param NXclass the class name of the group. Should start with the prefix NX
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUSCPP_DLL NXstatus NXopengroup(NXhandle handle, CONSTCHAR *name, CONSTCHAR *NXclass);

/**
 * Open the NeXus object with the path specified
 * \param handle A NeXus file handle as returned from NXopen.
 * \param path A unix like path string to a NeXus group or dataset. The path string
 * is a list of group names and SDS names separated with / (slash).
 * Example: /entry1/sample/name
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUSCPP_DLL NXstatus NXopenpath(NXhandle handle, CONSTCHAR *path);

/**
 * Opens the group in which the NeXus object with the specified path exists
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param path A unix like path string to a NeXus group or dataset. The path string
 * is a list of group names and SDS names separated with / (slash).
 * Example: /entry1/sample/name
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUSCPP_DLL NXstatus NXopengrouppath(NXhandle handle, CONSTCHAR *path);

/**
 * Retrieve the current path in the NeXus file
 * \param handle a NeXus file handle
 * \param path A buffer to copy the path too
 * \param  pathlen The maximum number of characters to copy into path
 * \return NX_OK or NX_ERROR
 * \ingroup c_navigation
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetpath(NXhandle handle, char *path, int pathlen);

/**
 * Closes the currently open group and steps one step down in the NeXus file
 * hierarchy.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_group
 */
MANTID_NEXUSCPP_DLL NXstatus NXclosegroup(NXhandle handle);

/**
 * Create a multi dimensional data array or dataset. The dataset is NOT opened.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param label The name of the dataset
 * \param datatype The data type of this data set.
 * \param rank The number of dimensions this dataset is going to have
 * \param dim An array of size rank holding the size of the dataset in each dimension. The first dimension
 * can be NX_UNLIMITED. Data can be appended to such a dimension using NXputslab.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXmakedata(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int dim[]);

/**
 * @copydoc NXmakedata()
 */
MANTID_NEXUSCPP_DLL NXstatus NXmakedata64(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int64_t dim[]);

/**
 * Create a compressed dataset. The dataset is NOT opened. Data from this set will automatically be compressed when
 * writing and decompressed on reading.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param label The name of the dataset
 * \param datatype The data type of this data set.
 * \param rank The number of dimensions this dataset is going to have
 * \param comp_typ The compression scheme to use. Possible values:
 * \li NX_COMP_NONE no compression
 * \li NX_COMP_LZW (recommended) despite the name this enabled zlib compression (of various levels, see above)
 * \li NX_COMP_RLE run length encoding (only HDF-4)
 * \li NX_COMP_HUF Huffmann encoding (only HDF-4)
 * \param dim An array of size rank holding the size of the dataset in each dimension. The first dimension
 * can be NX_UNLIMITED. Data can be appended to such a dimension using NXputslab.
 * \param bufsize The dimensions of the subset of the data which usually be writen in one go.
 * This is a parameter used by HDF for performance optimisations. If you write your data in one go, this
 * should be the same as the data dimension. If you write it in slabs, this is your preferred slab size.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXcompmakedata(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int dim[],
                                            int comp_typ, int bufsize[]);

/**
 * @copydoc NXcompmakedata()
 */
MANTID_NEXUSCPP_DLL NXstatus NXcompmakedata64(NXhandle handle, CONSTCHAR *label, int datatype, int rank, int64_t dim[],
                                              int comp_typ, int64_t chunk_size[]);

/**
 * Open access to a dataset. After this call it is possible to write and read data or
 * attributes to and from the dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param label The name of the dataset
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXopendata(NXhandle handle, CONSTCHAR *label);

/**
 * Close access to a dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXclosedata(NXhandle handle);

/**
 * Write data to a datset which has previouly been opened with NXopendata.
 * This writes all the data in one go. Data should be a pointer to a memory
 * area matching the datatype and dimensions of the dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data Pointer to data to write.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXputdata(NXhandle handle, const void *data);

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
MANTID_NEXUSCPP_DLL NXstatus NXputattr(NXhandle handle, CONSTCHAR *name, const void *data, int iDataLen, int iType);

/**
 * Write an attribute of any rank. The kind of attribute written depends on the
 * position in the file: at root level, a global attribute is written, if
 * agroup is open but no dataset, a group attribute is written, if a dataset is
 * open, a dataset attribute is written.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the attribute.
 * \param data A pointer to the data to write for the attribute.
 * \param rank Rank of the attribute data
 * \param dim Dimension array for the attribute data
 * \param iType The NeXus data type of the attribute.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXputattra(NXhandle handle, CONSTCHAR *name, const void *data, const int rank,
                                        const int dim[], const int iType);

/**
 * Write  a subset of a multi dimensional dataset.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data A pointer to a memory area holding the data to write.
 * \param start An array holding the start indices where to start the data subset.
 * \param size An array holding the size of the data subset to write in each dimension.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXputslab(NXhandle handle, const void *data, const int start[], const int size[]);

/**
 * @copydoc NXputdata()
 */
MANTID_NEXUSCPP_DLL NXstatus NXputslab64(NXhandle handle, const void *data, const int64_t start[],
                                         const int64_t size[]);

/**
 * Retrieve link data for a dataset. This link data can later on be used to link this
 * dataset into a different group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure which will be initialized with the required information
 * for linking.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetdataID(NXhandle handle, NXlink *pLink);

/**
 * Create a link to the group or dataset described by pLink in the currently open
 * group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure describing the object to link. This must have been initialized
 * by either a call to NXgetdataID or NXgetgroupID.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_NEXUSCPP_DLL NXstatus NXmakelink(NXhandle handle, NXlink *pLink);

/**
 * Create a link to the group or dataset described by pLink in the currently open
 * group. But give the linked item a new name.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param newname The new name of the item in the currently open group.
 * \param pLink A link data structure describing the object to link. This must have been initialized
 * by either a call to NXgetdataID or NXgetgroupID.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_NEXUSCPP_DLL NXstatus NXmakenamedlink(NXhandle handle, CONSTCHAR *newname, NXlink *pLink);

/**
 * Open the source group of a linked group or dataset. Returns an error when the item is
 * not a linked item.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUSCPP_DLL NXstatus NXopensourcegroup(NXhandle handle);

/**
 * Read a complete dataset from the currently open dataset into memory.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param data A pointer to the memory area where to read the data, too. Data must point to a memory
 * area large enough to accomodate the data read. Otherwise your program may behave in unexpected
 * and unwelcome ways.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetdata(NXhandle handle, void *data);

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
MANTID_NEXUSCPP_DLL NXstatus NXgetinfo(NXhandle handle, int *rank, int dimension[], int *datatype);

/**
 * @copydoc NXgetinfo()
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetinfo64(NXhandle handle, int *rank, int64_t dimension[], int *datatype);

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
MANTID_NEXUSCPP_DLL NXstatus NXgetnextentry(NXhandle handle, NXname name, NXname nxclass, int *datatype);

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
MANTID_NEXUSCPP_DLL NXstatus NXgetslab(NXhandle handle, void *data, const int start[], const int size[]);

/**
 * @copydoc NXgetslab()
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetslab64(NXhandle handle, void *data, const int64_t start[], const int64_t size[]);

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
MANTID_NEXUSCPP_DLL NXstatus NXgetattr(NXhandle handle, const char *name, void *data, int *iDataLen, int *iType);

/**
 * Get the count of attributes in the currently open dataset, group or global attributes when at root level.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param no_items A pointer to an integer which be set to the number of attributes available.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_metadata
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetattrinfo(NXhandle handle, int *no_items);

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
MANTID_NEXUSCPP_DLL NXstatus NXgetnextattra(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);

/**
 * Read an arbitrarily shaped attribute.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the atrribute to read.
 * \param data A pointer to a memory area large enough to hold the attributes value.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetattra(NXhandle handle, const char *name, void *data);

/**
 * Get the information about the storage of the named attribute.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param rank Rank of the attribute data.
 * \param dim Dimension array for the attribute content.
 * \param iType A pointer to an integer which be set to the NeXus data type of the attribute.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_metadata
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetattrainfo(NXhandle handle, NXname pName, int *rank, int dim[], int *iType);

/**
 * Retrieve link data for the currently open group. This link data can later on be used to link this
 * group into a different group.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pLink A link data structure which will be initialized with the required information
 * for linking.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_linking
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetgroupID(NXhandle handle, NXlink *pLink);

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
MANTID_NEXUSCPP_DLL NXstatus NXgetgroupinfo(NXhandle handle, int *no_items, NXname name, NXname nxclass);

/**
 * Tests if two link data structures describe the same item.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param pFirstID The first link data for the test.
 * \param pSecondID The second link data structure.
 * \return NX_OK when both link data structures describe the same item, NX_ERROR else.
 * \ingroup c_linking
 */
MANTID_NEXUSCPP_DLL NXstatus NXsameID(NXhandle handle, NXlink *pFirstID, NXlink *pSecondID);

/**
 * Resets a pending group search to the start again. To be called in a #NXgetnextentry loop when
 * a group search has to be restarted.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUSCPP_DLL NXstatus NXinitgroupdir(NXhandle handle);

/**
 * Resets a pending attribute search to the start again. To be called in a #NXgetnextattr loop when
 * an attribute search has to be restarted.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_navigation
 */
MANTID_NEXUSCPP_DLL NXstatus NXinitattrdir(NXhandle handle);

/**
 * Sets the format for number printing. This call has only an effect when using the XML physical file
 * format.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param type The NeXus data type to set the format for.
 * \param format The C-language format string to use for this data type.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_readwrite
 */
MANTID_NEXUSCPP_DLL NXstatus NXsetnumberformat(NXhandle handle, int type, char *format);

/**
 * Inquire the filename of the currently open file. FilenameBufferLength of the file name
 * will be copied into the filename buffer.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param filename The buffer to hold the filename.
 * \param  filenameBufferLength The length of the filename buffer.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_metadata
 */
MANTID_NEXUSCPP_DLL NXstatus NXinquirefile(NXhandle handle, char *filename, int filenameBufferLength);

/**
 * Test if a group is actually pointing to an external file. If so, retrieve the URL of the
 * external file.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the group to test.
 * \param nxclass The class name of the group to test.
 * \param url A buffer to copy the URL too.
 * \param urlLen The length of the Url buffer. At maximum urlLen bytes will be copied to url.
 * \return NX_OK when the group is pointing to an external file, NX_ERROR else.
 * \ingroup c_external
 */
MANTID_NEXUSCPP_DLL NXstatus NXisexternalgroup(NXhandle handle, CONSTCHAR *name, CONSTCHAR *nxclass, char *url,
                                               int urlLen);

/**
 * Test if a dataset is actually pointing to an external file. If so, retrieve the URL of the
 * external file.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the dataset to test.
 * \param url A buffer to copy the URL too.
 * \param urlLen The length of the Url buffer. At maximum urlLen bytes will be copied to url.
 * \return NX_OK when the dataset is pointing to an external file, NX_ERROR else.
 * \ingroup c_external
 */
MANTID_NEXUSCPP_DLL NXstatus NXisexternaldataset(NXhandle handle, CONSTCHAR *name, char *url, int urlLen);

/**
 * Create a link to a group in an external file. This works by creating a NeXus group under the current level in
 * the hierarchy which actually points to a group in another file.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the group which points to the external file.
 * \param nxclass The class name of the group which points to the external file.
 * \param url The URL of the external file. Currently only one URL format is supported:
 * nxfile://path-tofile\#path-in-file. This consists of two parts: the first part is of course the path to the file. The
 * second part, path-in-file, is the path to the group in the external file which appears in the first file.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_external
 */
MANTID_NEXUSCPP_DLL NXstatus NXlinkexternal(NXhandle handle, CONSTCHAR *name, CONSTCHAR *nxclass, CONSTCHAR *url);

/**
 * Create a link to a dataset in an external file. This works by creating a dataset under the current level in
 * the hierarchy which actually points to a dataset in another file.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param name The name of the dataset which points to the external file.
 * \param url The URL of the external file. Currently only one URL format is supported:
 * nxfile://path-tofile\#path-in-file. This consists of two parts: the first part is of course the path to the file. The
 * second part, path-in-file, is the path to the dataset in the external file which appears in the first file.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_external
 */
MANTID_NEXUSCPP_DLL NXstatus NXlinkexternaldataset(NXhandle handle, CONSTCHAR *name, CONSTCHAR *url);

/**
 * Utility function which allocates a suitably sized memory area for the dataset characteristics specified.
 * \param data A pointer to a pointer which will be initialized with a pointer to a suitably sized memory area.
 * \param rank the rank of the data.
 * \param dimensions An array holding the size of the data in each dimension.
 * \param datatype The NeXus data type of the data.
 * \return NX_OK when allocation succeeds, NX_ERROR in the case of an error.
 * \ingroup c_memory
 */
MANTID_NEXUSCPP_DLL NXstatus NXmalloc(void **data, int rank, const int dimensions[], int datatype);

/**
 * @copydoc NXmalloc()
 */
MANTID_NEXUSCPP_DLL NXstatus NXmalloc64(void **data, int rank, const int64_t dimensions[], int datatype);

/**
 * Utility function to return NeXus version
 * \return pointer to string in static storage. Version in
 * same format as NEXUS_VERSION string in napi.h i.e. "major.minor.patch"
 * \ingroup c_metadata
 */
MANTID_NEXUSCPP_DLL const char *NXgetversion();

/**
 * Utility function to release the memory for data.
 * \param data A pointer to a pointer to free.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_memory
 */
MANTID_NEXUSCPP_DLL NXstatus NXfree(void **data);

MANTID_NEXUSCPP_DLL NXstatus NXIprintlink(NXhandle fid, NXlink *link);

/**
 * Retrieve information about the currently open dataset. In contrast to the main function below,
 * this function does not try to find out about the size of strings properly.
 * \param handle A NeXus file handle as initialized by NXopen.
 * \param rank A pointer to an integer which will be filled with the rank of
 * the dataset.
 * \param dimension An array which will be initialized with the size of the dataset in any of its
 * dimensions. The array must have at least the size of rank.
 * \param datatype A pointer to an integer which be set to the NeXus data type code for this dataset.
 * \return NX_OK on success, NX_ERROR in the case of an error.
 * \ingroup c_metadata
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetrawinfo(NXhandle handle, int *rank, int dimension[], int *datatype);

/**
 * @copydoc NXgetrawinfo
 */
MANTID_NEXUSCPP_DLL NXstatus NXgetrawinfo64(NXhandle handle, int *rank, int64_t dimension[], int *datatype);

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
 * Set a global error function.
 * Not threadsafe.
 * \param pData A pointer to a user defined data structure which be passed to
 * the error display function.
 * \param newErr The new error display function.
 */
MANTID_NEXUSCPP_DLL void NXMSetError(void *pData, ErrFunc newErr);

/**
 * Set an error function for the current thread.
 * When used this overrides anything set in NXMSetError (for the current thread).
 * Use this method in threaded applications.
 * \param pData A pointer to a user defined data structure which be passed to
 * the error display function.
 * \param newErr The new error display function.
 */
MANTID_NEXUSCPP_DLL void NXMSetTError(void *pData, ErrFunc newErr);

/**
 * Retrieve the current error display function
 * \return The current error display function.
 */
MANTID_NEXUSCPP_DLL ErrFunc NXMGetError();

/**
 * Suppress error reports from the NeXus-API
 */
MANTID_NEXUSCPP_DLL void NXMDisableErrorReporting();

/**
 * Enable error reports from the NeXus-API
 */
MANTID_NEXUSCPP_DLL void NXMEnableErrorReporting();

/**
 * Dispatches the error message to the error function defined by NXMSetTError
 */
MANTID_NEXUSCPP_DLL void NXReportError(const char *text);

/**
 * Do not use, first parameter should be set by NXMSetTError
 */
MANTID_NEXUSCPP_DLL void NXIReportError(void *pData, const char *text);
/* extern void *NXpData; */
MANTID_NEXUSCPP_DLL char *NXIformatNeXusTime();

/**
 * A function for setting the default cache size for HDF-5
 * \ingroup c_init
 */
MANTID_NEXUSCPP_DLL NXstatus NXsetcache(long newVal);

#ifdef __cplusplus
};
#endif /* __cplusplus */

/**
 * Freddie Akeroyd 11/8/2009
 * Add NeXus schema support - this uses BASE.xsd as the initial file
 */
#define NEXUS_SCHEMA_VERSION "3.1"                                    /**< version of NeXus definition schema */
#define NEXUS_SCHEMA_ROOT "http://definition.nexusformat.org/schema/" /**< XML schema namespace specified by xmlns */
#define NEXUS_SCHEMA_NAMESPACE NEXUS_SCHEMA_ROOT NEXUS_SCHEMA_VERSION /**< XML schema namespace specified by xmlns */
#define NEXUS_SCHEMA_BASE "BASE"
#define NEXUS_SCHEMA_FILE NEXUS_SCHEMA_BASE ".xsd" /**< default schema file for namespace */
#define NEXUS_SCHEMA_URL                                                                                               \
  NEXUS_SCHEMA_NAMESPACE "/" NEXUS_SCHEMA_FILE /**< location of default schema file for namespace */

#endif /*NEXUSAPI*/
