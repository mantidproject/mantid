#pragma once

#include "MantidLegacyNexus/DllConfig.h"
#include "MantidLegacyNexus/NeXusFile_fwd.h"
#include "MantidLegacyNexus/napi.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * \file NeXusFile.hpp Definition of the NeXus C++ API.
 * \defgroup cpp_types C++ Types
 * \defgroup cpp_core C++ Core
 * \ingroup cpp_main
 */

namespace Mantid::LegacyNexus {

/**
 * The available compression types. These are all ignored in xml files.
 * \li NONE no compression
 * \li LZW Lossless Lempel Ziv Welch compression (recommended)
 * \li RLE Run length encoding (only HDF-4)
 * \li HUF Huffmann encoding (only HDF-4)
 * \ingroup cpp_types
 */
enum NXcompression { CHUNK = NX_CHUNK, NONE = NX_COMP_NONE, LZW = NX_COMP_LZW, RLE = NX_COMP_RLE, HUF = NX_COMP_HUF };

/**
 * Type definition for a type-keyed multimap
 */
typedef std::multimap<std::string, std::string> TypeMap;

/**
 * This structure holds the type and dimensions of a primative field/array.
 */
struct Info {
  /** The primative type for the field. */
  NXnumtype type;
  /** The dimensions of the file. */
  std::vector<int64_t> dims;
};

/** Information about an attribute. */
struct AttrInfo {
  /** The primative type for the attribute. */
  NXnumtype type;
  /** The length of the attribute. */
  unsigned length;
  /** The name of the attribute. */
  std::string name;
  /** The dimensions of the attribute. */
  std::vector<int> dims;
};

/**
 * The Object that allows access to the information in the file.
 * \ingroup cpp_core
 */
class MANTID_LEGACYNEXUS_DLL File {
private:
  /** The handle for the C-API. */
  NXhandle m_file_id;
  /** should be close handle on exit */
  bool m_close_handle;

public:
  /**
   * \return A pair of the next entry available in a listing.
   */
  std::pair<std::string, std::string> getNextEntry();
  /**
   * \return Information about the next attribute.
   */
  AttrInfo getNextAttr();

private:
  /**
   * Initialize the pending group search to start again.
   */
  void initGroupDir();

  /**
   * Initialize the pending attribute search to start again.
   */
  void initAttrDir();

  /**
   * Function to consolidate the file opening code for the various constructors
   * \param filename The name of the file to open.
   * \param access How to access the file.
   */
  void initOpenFile(const std::string &filename, const NXaccess access = NXACC_READ);

public:
  /**
   * Create a new File.
   *
   * \param filename The name of the file to open.
   * \param access How to access the file.
   */
  File(const std::string &filename, const NXaccess access = NXACC_READ);

  /**
   * Create a new File.
   *
   * \param filename The name of the file to open.
   * \param access How to access the file.
   */
  File(const char *filename, const NXaccess access = NXACC_READ);

  /**
   * Use an existing handle returned from NXopen()
   *
   * \param handle Handle to connect to
   * \param close_handle Should the handle be closed on destruction
   */
  File(NXhandle handle, bool close_handle = false);

  /** Destructor. This does close the file. */
  ~File();

  /** Close the file before the constructor is called. */
  void close();

  /** Flush the file. */
  void flush();

  /**
   * Create a new group.
   *
   * \param name The name of the group to create (i.e. "entry").
   * \param class_name The type of group to create (i.e. "NXentry").
   * \param open_group Whether or not to automatically open the group after
   * creating it.
   */
  void makeGroup(const std::string &name, const std::string &class_name, bool open_group = false);

  /**
   * Open an existing group.
   *
   * \param name The name of the group to create (i.e. "entry").
   * \param class_name The type of group to create (i.e. "NXentry").
   */
  void openGroup(const std::string &name, const std::string &class_name);

  /**
   * Open the NeXus object with the path specified.
   *
   * \param path A unix like path string to a group or field. The path
   * string is a list of group names and SDS names separated with a slash,
   * '/' (i.e. "/entry/sample/name").
   */
  void openPath(const std::string &path);

  /**
   * Get the path into the current file
   * \return A unix like path string pointing to the current
   *         position in the file
   */
  std::string getPath();

  /**
   * Close the currently open group.
   */
  void closeGroup();

  /**
   * \copydoc NeXus::File::makeData(const std::string&, NXnumtype,
   *                              const std::vector<int64_t>&, bool);
   */
  void makeData(const std::string &name, NXnumtype type, const std::vector<int> &dims, bool open_data = false);

  /**
   * Create a data field with the specified information.
   *
   * \param name The name of the field to create (i.e. "distance").
   * \param type The primative type of the field (i.e. "NeXus::FLOAT32").
   * \param dims The dimensions of the field.
   * \param open_data Whether or not to open the data after creating it.
   */
  void makeData(const std::string &name, NXnumtype type, const std::vector<int64_t> &dims, bool open_data = false);

  /**
   * Create a 1D data field with the specified information.
   *
   * \param name The name of the field to create (i.e. "distance").
   * \param type The primative type of the field (i.e. "NeXus::FLOAT32").
   * \param length The number of elements in the field.
   * \param open_data Whether or not to open the data after creating it.
   */
  template <typename NumT>
  void makeData(const std::string &name, const NXnumtype type, const NumT length, bool open_data = false);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \param name The name of the field to create.
   * \param value The string to put into the file.
   */
  void writeData(const std::string &name, const std::string &value);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \param name The name of the field to create.
   * \param value The string to put into the file.
   */
  void writeData(const std::string &name, const char *value);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name The name of the field to create.
   * \param value The vector to put into the file.
   */
  template <typename NumT> void writeData(const std::string &name, const std::vector<NumT> &value);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name The name of the field to create.
   * \param value The value to put into the file.
   */
  template <typename NumT> void writeData(const std::string &name, const NumT &value);

  /**
   * Create a n-dimension data field, insert the data, and close the data.
   *
   * \param name The name of the field to create.
   * \param value The data to put into the file.
   * \param dims The dimensions of the data.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT>
  void writeData(const std::string &name, const std::vector<NumT> &value, const std::vector<int> &dims);

  /**
   * Create a n-dimension data field, insert the data, and close the data.
   *
   * \param name The name of the field to create.
   * \param value The data to put into the file.
   * \param dims The dimensions of the data.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT>
  void writeData(const std::string &name, const std::vector<NumT> &value, const std::vector<int64_t> &dims);

  /** Create a 1D data field with an unlimited dimension, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   */
  template <typename NumT> void writeExtendibleData(const std::string &name, std::vector<NumT> &value);

  /** Create a 1D data field with an unlimited dimension, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   * \param chunkSize :: chunk size to use when writing
   */
  template <typename NumT>
  void writeExtendibleData(const std::string &name, std::vector<NumT> &value, const int64_t chunk);

  /** Create a 1D data field with an unlimited dimension, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   * \param dims :: The dimensions of the data.
   * \param chunk :: chunk size to use when writing
   */
  template <typename NumT>
  void writeExtendibleData(const std::string &name, std::vector<NumT> &value, std::vector<int64_t> &dims,
                           std::vector<int64_t> &chunk);

  /** Updates the data written into an already-created
   * data vector. If the data was created as extendible, it will be resized.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   */
  template <typename NumT> void writeUpdatedData(const std::string &name, std::vector<NumT> &value);

  /** Updates the data written into an already-created
   * data vector. If the data was created as extendible, it will be resized.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   * \param dims :: The dimensions of the data.
   */
  template <typename NumT>
  void writeUpdatedData(const std::string &name, std::vector<NumT> &value, std::vector<int64_t> &dims);

  /**
   * \copydoc makeCompData(const std::string&, const NXnumtype,
   *                       const std::vector<int64_t>&, const NXcompression,
   *                       const std::vector<int64_t>&, bool)
   */
  void makeCompData(const std::string &name, const NXnumtype type, const std::vector<int> &dims,
                    const NXcompression comp, const std::vector<int> &bufsize, bool open_data = false);

  /**
   * Create a field with compression.
   *
   * \param name The name of the data to create.
   * \param type The primitive type for the data.
   * \param dims The dimensions of the data.
   * \param comp The compression algorithm to use.
   * \param bufsize The size of the compression buffer to use.
   * \param open_data Whether or not to open the data after creating it.
   */
  void makeCompData(const std::string &name, const NXnumtype type, const std::vector<int64_t> &dims,
                    const NXcompression comp, const std::vector<int64_t> &bufsize, bool open_data = false);

  /**
   * \copydoc writeCompData(const std::string & name,
   *                        const std::vector<NumT> & value,
   *                        const std::vector<int64_t> & dims, const NXcompression comp,
   *                        const std::vector<int64_t> & bufsize)
   */
  template <typename NumT>
  void writeCompData(const std::string &name, const std::vector<NumT> &value, const std::vector<int> &dims,
                     const NXcompression comp, const std::vector<int> &bufsize);

  /**
   * Create a compressed data, insert the data, and close it.
   *
   * \param name The name of the data to create.
   * \param value The vector to put into the file.
   * \param dims The dimensions of the data.
   * \param comp The compression algorithm to use.
   * \param bufsize The size of the compression buffer to use.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT>
  void writeCompData(const std::string &name, const std::vector<NumT> &value, const std::vector<int64_t> &dims,
                     const NXcompression comp, const std::vector<int64_t> &bufsize);

  /**
   * \param name The name of the data to open.
   */
  void openData(const std::string &name);

  /**
   * Close the currently open data.
   */
  void closeData();

  /**
   * \param data The data to put in the file.
   */
  void putData(const void *data);

  /**
   * \param data The data to put in the file.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void putData(const std::vector<NumT> &data);

  /**
   * Put the supplied data as an attribute into the currently open data.
   *
   * \param info Description of the attribute to add.
   * \param data The attribute value.
   */
  void putAttr(const AttrInfo &info, const void *data);

  /**
   * Put the supplied data as an attribute into the currently open data.
   *
   * \param name Name of the attribute to add.
   * \param value The attribute value.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT> void putAttr(const std::string &name, const NumT value);

  /**
   * Put a string as an attribute in the file.
   *
   * \param name Name of the attribute to add.
   * \param value The attribute value.
   */
  void putAttr(const char *name, const char *value);

  /**
   * Put a string as an attribute in the file.
   *
   * \param name Name of the attribute to add.
   * \param value The attribute value.
   */
  void putAttr(const std::string &name, const std::string &value, const bool empty_add_space = true);

  /**
   * \copydoc NeXus::File::putSlab(void* data, std::vector<int64_t>& start,
   *                                std::vector<int64_t>& size)
   */
  void putSlab(const void *data, const std::vector<int> &start, const std::vector<int> &size);

  /**
   * Insert an array as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   */
  void putSlab(const void *data, const std::vector<int64_t> &start, const std::vector<int64_t> &size);

  /**
   * \copydoc NeXus::File::putSlab(std::vector<NumT>& data, std::vector<int64_t>&,
   *                               std::vector<int64_t>&)
   */
  template <typename NumT>
  void putSlab(const std::vector<NumT> &data, const std::vector<int> &start, const std::vector<int> &size);

  /**
   * Insert an array as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT>
  void putSlab(const std::vector<NumT> &data, const std::vector<int64_t> &start, const std::vector<int64_t> &size);

  /**
   * \copydoc NeXus::File::putSlab(std::vector<NumT>&, int64_t, int64_t)
   */
  template <typename NumT> void putSlab(const std::vector<NumT> &data, int start, int size);

  /**
   * Insert a number as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void putSlab(const std::vector<NumT> &data, int64_t start, int64_t size);

  /**
   * \return The id of the data used for linking.
   */
  NXlink getDataID();

  /**
   * Create a link in the current location to the supplied id.
   *
   * \param link The object (group or data) in the file to link to.
   */
  void makeLink(NXlink &link);

  /**
   * Put the currently open data in the supplied pointer.
   *
   * \param data The pointer to copy the data to.
   */
  void getData(void *data);

  /**
   * Put data into the supplied vector. The vector does not need to
   * be the correct size, just the correct type as it is resized to
   * the appropriate value.
   *
   * \param data Where to put the data.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void getData(std::vector<NumT> &data);

  /** Get data and coerce into an int vector.
   *
   * @throw Exception if the data is actually a float or
   *    another type that cannot be coerced to an int.
   * @param data :: vector to be filled.
   */
  void getDataCoerce(std::vector<int> &data);

  /** Get data and coerce into a vector of doubles.
   *
   * @throw Exception if the data cannot be coerced to a double.
   * @param data :: vector to be filled.
   */
  void getDataCoerce(std::vector<double> &data);

  /** Return true if the data opened is of one of the
   * int data types, 32 bits or less.
   */
  bool isDataInt();

  /** Put data into the supplied vector. The vector does not need to
   * be the correct size, just the correct type as it is resized to
   * the appropriate value.
   *
   * The named data object is opened, loaded, then closed.
   *
   * @param dataName :: name of the data to open.
   * @param data :: Where to put the data.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void readData(const std::string &dataName, std::vector<NumT> &data);

  /** Put data into the supplied value.
   *
   * The named data object is opened, loaded, then closed.
   *
   * \param dataName :: name of the data to open.
   * \param data :: Where to put the data.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void readData(const std::string &dataName, NumT &data);

  /** Put data into the supplied string. The vector does not need to
   * be the correct size, just the correct type as it is resized to
   * the appropriate value.
   *
   * The named data object is opened, loaded, then closed.
   *
   * @param dataName :: name of the data to open.
   * @param data :: Where to put the data.
   */
  void readData(const std::string &dataName, std::string &data);

  /**
   * \return String data from the file.
   */
  std::string getStrData();

  /**
   * \return The Info structure that describes the currently open data.
   */
  Info getInfo();

  /**
   * Return the entries available in the current place in the file.
   */
  std::map<std::string, std::string> getEntries();

  /** Return the entries available in the current place in the file,
   * but avoids the map copy of getEntries().
   *
   * \param result The map that will be filled with the entries
   */
  void getEntries(std::map<std::string, std::string> &result);

  /**
   * \copydoc NeXus::File::getSlab(void*, const std::vector<int64_t>&,
   *                               const std::vector<int64_t>&)
   */
  void getSlab(void *data, const std::vector<int> &start, const std::vector<int> &size);

  /**
   * Get a section of data from the file.
   *
   * \param data The pointer to insert that data into.
   * \param start The offset into the file's data block to start the read
   * from.
   * \param size The size of the block to read from the file.
   */
  void getSlab(void *data, const std::vector<int64_t> &start, const std::vector<int64_t> &size);

  /**
   * \return Information about all attributes on the data that is
   * currently open.
   */
  std::vector<AttrInfo> getAttrInfos();

  /**
   *  \return true if the current point in the file has the named attribute
   *  \param name the name of the attribute to look for.
   */
  bool hasAttr(const std::string &name);

  /**
   * Get the value of the attribute specified by the AttrInfo supplied.
   *
   * \param info Designation of which attribute to read.
   * \param data The pointer to put the attribute value in.
   * \param length The length of the attribute. If this is "-1" then the
   * information in the supplied AttrInfo object will be used.
   */
  void getAttr(const AttrInfo &info, void *data, int length = -1);

  /**
   * Get the value of an attribute that is a scalar number.
   *
   * \param info Designation of which attribute to read.
   * \tparam NumT numeric data type of result
   *
   * \return The attribute value.
   */
  template <typename NumT> NumT getAttr(const AttrInfo &info);

  /**
   * Get the value of an attribute that is a scalar number.
   *
   * \param[in] name Name of attribute to read
   * \param[out] value The read attribute value.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT> void getAttr(const std::string &name, NumT &value);

  /**
   * Get the value of a string attribute.
   *
   * \param info Which attribute to read.
   *
   * \return The value of the attribute.
   */
  std::string getStrAttr(const AttrInfo &info);

  /**
   * \return The id of the group used for linking.
   */
  NXlink getGroupID();

  /**
   * This function checksi if we are in an open dataset
   * \returns true if we are currently in an open dataset else false
   */
  bool isDataSetOpen();
};

/**
 * This function returns the NXnumtype given a concrete number.
 * \tparam NumT numeric data type of \a number to check
 */
template <typename NumT> MANTID_LEGACYNEXUS_DLL NXnumtype getType(NumT const number = NumT());

}; // namespace Mantid::LegacyNexus
