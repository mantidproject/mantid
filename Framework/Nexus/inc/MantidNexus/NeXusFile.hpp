#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NeXusFile_fwd.h"
#include "MantidNexus/NexusDescriptor.h"
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
static const std::string NULL_STR("NULL");
}

/**
 * \file NeXusFile.hpp Definition of the NeXus C++ API.
 * \defgroup cpp_types C++ Types
 * \defgroup cpp_core C++ Core
 * \ingroup cpp_main
 */

namespace NeXus {

static Entry const EOD_ENTRY(NULL_STR, NULL_STR);

/**
 * The Object that allows access to the information in the file.
 * \ingroup cpp_core
 */
class MANTID_NEXUS_DLL File {
private:
  std::string m_filename;
  NXaccess m_access;
  /** The handle for the C-API. */
  std::shared_ptr<NXhandle> m_pfile_id;
  /** should be close handle on exit */
  bool m_close_handle;
  /** nexus descriptor to track the file tree
   * NOTE: in file write, the following cannot be relied upon:
   * - hasRootAttr
   * - firstEntryNameType
   */
  Mantid::Nexus::NexusDescriptor m_descriptor;

  /**
   * \return A pair of the next entry available in a listing.
   * NOTE: this is to be deleted in 6.14.  do NOT make public
   */
  Entry getNextEntry();

public:
  /**
   * \return Information about the next attribute.
   */
  AttrInfo getNextAttr();

  /**
   * Initialize the pending group search to start again.
   */
  void initGroupDir();

private:
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
  File(std::string const &filename, NXaccess const access = NXACC_READ);

  /**
   * Create a new File.
   *
   * \param filename The name of the file to open.
   * \param access How to access the file.
   */
  File(char const *filename, NXaccess const access = NXACC_READ);

  /**
   * Copy constructor
   *
   * \param f File to copy over, to complete rule of three
   */
  File(File const &f);

  /**
   * Copy constructor from pointer
   *
   * \param pf Pointer to file to copy over
   */
  File(File const *const pf);

  /**
   * Copy constructor from pointer
   *
   * \param pf Pointer to file to copy over
   */
  File(std::shared_ptr<File> pf);

  /**
   * Assignment operator, to complete the rule of three
   *
   * \param f File to assign
   */
  File &operator=(File const &f);

  /** Destructor. This does close the file. */
  ~File();

  /** Close the file before the constructor is called. */
  void close();

  /** Flush the file. */
  void flush();

  bool hasPath(std::string const &);
  bool hasGroup(std::string const &, std::string const &);
  bool hasData(std::string const &);

  /**
   * Create a new group.
   *
   * \param name The name of the group to create (i.e. "entry").
   * \param class_name The type of group to create (i.e. "NXentry").
   * \param open_group Whether or not to automatically open the group after
   * creating it.
   */
  void makeGroup(std::string const &name, std::string const &class_name, bool open_group = false);

  /**
   * Open an existing group.
   *
   * \param name The name of the group to create (i.e. "entry").
   * \param class_name The type of group to create (i.e. "NXentry").
   */
  void openGroup(std::string const &name, std::string const &class_name);

  /**
   * Open the NeXus object with the path specified.
   *
   * \param path A unix like path string to a group or field. The path
   * string is a list of group names and SDS names separated with a slash,
   * '/' (i.e. "/entry/sample/name").
   */
  void openPath(std::string const &path);

  /**
   * Open the group in which the NeXus object with the specified path exists.
   *
   * \param path A unix like path string to a group or field. The path
   * string is a list of group names and SDS names separated with a slash,
   * '/' (i.e. "/entry/sample/name").
   */
  void openGroupPath(std::string const &path);

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
   * Create a data field with the specified information.
   *
   * \param name The name of the field to create (i.e. "distance").
   * \param type The primative type of the field (i.e. "NeXus::FLOAT32").
   * \param dims The dimensions of the field.
   * \param open_data Whether or not to open the data after creating it.
   */
  void makeData(std::string const &name, NXnumtype type, DimVector const &dims, bool open_data = false);

  /**
   * Create a 1D data field with the specified information.
   *
   * \param name The name of the field to create (i.e. "distance").
   * \param type The primative type of the field (i.e. "NeXus::FLOAT32").
   * \param length The number of elements in the field.
   * \param open_data Whether or not to open the data after creating it.
   */
  void makeData(std::string const &name, NXnumtype const type, dimsize_t const length, bool open_data = false);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \param name The name of the field to create.
   * \param value The string to put into the file.
   */
  void writeData(std::string const &name, std::string const &value);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \param name The name of the field to create.
   * \param value The string to put into the file.
   */
  void writeData(std::string const &name, char const *value);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name The name of the field to create.
   * \param value The vector to put into the file.
   */
  template <typename NumT> void writeData(std::string const &name, std::vector<NumT> const &value);

  /**
   * Create a 1D data field, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name The name of the field to create.
   * \param value The value to put into the file.
   */
  template <typename NumT> void writeData(std::string const &name, NumT const &value);

  /**
   * Create a n-dimension data field, insert the data, and close the data.
   *
   * \param name The name of the field to create.
   * \param value The data to put into the file.
   * \param dims The dimensions of the data.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT>
  void writeData(std::string const &name, std::vector<NumT> const &value, DimVector const &dims);

  /** Create a 1D data field with an unlimited dimension, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   */
  template <typename NumT> void writeExtendibleData(std::string const &name, std::vector<NumT> const &value);

  /** Create a 1D data field with an unlimited dimension, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   * \param chunkSize :: chunk size to use when writing
   */
  template <typename NumT>
  void writeExtendibleData(std::string const &name, std::vector<NumT> const &value, dimsize_t const chunk);

  /** Create a 1D data field with an unlimited dimension, insert the data, and close the data.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   * \param dims :: The dimensions of the data.
   * \param chunk :: chunk size to use when writing
   */
  template <typename NumT>
  void writeExtendibleData(std::string const &name, std::vector<NumT> const &value, DimVector const &dims,
                           DimSizeVector const &chunk);

  /** Updates the data written into an already-created
   * data vector. If the data was created as extendible, it will be resized.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   */
  template <typename NumT> void writeUpdatedData(std::string const &name, std::vector<NumT> const &value);

  /** Updates the data written into an already-created
   * data vector. If the data was created as extendible, it will be resized.
   *
   * \tparam NumT numeric data type of \a value
   * \param name :: The name of the field to create.
   * \param value :: The vector to put into the file.
   * \param dims :: The dimensions of the data.
   */
  template <typename NumT>
  void writeUpdatedData(std::string const &name, std::vector<NumT> const &value, DimVector const &dims);

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
  void makeCompData(std::string const &name, NXnumtype const type, DimVector const &dims, NXcompression comp,
                    DimSizeVector const &bufsize, bool open_data = false);

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
  void writeCompData(std::string const &name, std::vector<NumT> const &value, DimVector const &dims,
                     NXcompression const comp, DimSizeVector const &bufsize);

  /**
   * \param name The name of the data to open.
   */
  void openData(std::string const &name);

  /**
   * Close the currently open data.
   */
  void closeData();

  /**
   * \param data The data to put in the file.
   */
  template <typename NumT> void putData(NumT const *data);

  void putData(std::string const &data);

  /**
   * \param data The data to put in the file.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void putData(std::vector<NumT> const &data);

  /**
   * Put the supplied data as an attribute into the currently open data.
   *
   * \param info Description of the attribute to add.
   * \param data The attribute value.
   */
  template <typename NumT> void putAttr(AttrInfo const &info, NumT const *data);
  /**
   * Put the supplied data as an attribute into the currently open data.
   *
   * \param name Name of the attribute to add.
   * \param value The attribute value.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT> void putAttr(std::string const &name, NumT const &value);

  /**
   * Put a string as an attribute in the file.
   *
   * \param name Name of the attribute to add.
   * \param value The attribute value.
   */
  void putAttr(char const *name, char const *value);

  /**
   * Put a string as an attribute in the file.
   *
   * \param name Name of the attribute to add.
   * \param value The attribute value.
   */
  void putAttr(std::string const &name, std::string const &value, bool const empty_add_space = true);

  /**
   * Insert an array as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   */
  template <typename NumT> void putSlab(NumT const *data, DimSizeVector const &start, DimSizeVector const &size);

  /**
   * Insert an array as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT>
  void putSlab(std::vector<NumT> const &data, DimSizeVector const &start, DimSizeVector const &size);

  /**
   * Insert a number as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void putSlab(std::vector<NumT> const &data, dimsize_t const start, dimsize_t const size);

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
  template <typename NumT> void getData(NumT *data);

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
  template <typename NumT> void readData(std::string const &dataName, std::vector<NumT> &data);

  /** Put data into the supplied value.
   *
   * The named data object is opened, loaded, then closed.
   *
   * \param dataName :: name of the data to open.
   * \param data :: Where to put the data.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void readData(std::string const &dataName, NumT &data);

  /** Put data into the supplied string. The vector does not need to
   * be the correct size, just the correct type as it is resized to
   * the appropriate value.
   *
   * The named data object is opened, loaded, then closed.
   *
   * @param dataName :: name of the data to open.
   * @param data :: Where to put the data.
   */
  void readData(std::string const &dataName, std::string &data);

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
  Entries getEntries();

  /** Return the entries available in the current place in the file,
   * but avoids the map copy of getEntries().
   *
   * \param result The map that will be filled with the entries
   */
  void getEntries(Entries &result);

  /**
   * Get a section of data from the file.
   *
   * \param data The pointer to insert that data into.
   * \param start The offset into the file's data block to start the read
   * from.
   * \param size The size of the block to read from the file.
   */
  template <typename NumT> void getSlab(NumT *data, const DimSizeVector &start, const DimSizeVector &size);

  /** Return the string name of the top-level entry
   *
   * \return a string with the name (not abs path) of the top-level entry
   */
  std::string getTopLevelEntryName();

  /**
   * \return Information about all attributes on the data that is currently open.
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

  template <typename NumT> NumT getAttr(std::string const &name);

  /**
   * Get the value of an attribute that is a scalar number.
   *
   * \param[in] name Name of attribute to read
   * \param[out] value The read attribute value.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT> void getAttr(std::string const &name, NumT &value);

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
   * This function checks if we are in an open dataset
   * \returns true if we are currently in an open dataset else false
   */
  bool isDataSetOpen();

private:
  std::string formAbsolutePath(std::string const &);
  void registerEntry(std::string const &, std::string const &);
};

/**
 * This function returns the NXnumtype given a concrete number.
 * \tparam NumT numeric data type of \a number to check
 */
template <typename NumT> MANTID_NEXUS_DLL NXnumtype getType(NumT const number = NumT());

}; // namespace NeXus
