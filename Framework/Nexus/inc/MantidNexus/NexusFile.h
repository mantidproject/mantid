#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusAddress.h"
#include "MantidNexus/NexusDescriptor.h"
#include "MantidNexus/NexusFile_fwd.h"
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/** Nexus HDF45
 * major.minor.patch
 */
#define NEXUS_VERSION "4.4.3"

namespace {
static const std::string NULL_STR("NULL");
}

namespace H5 {
// forward declare
class H5Object;
} // namespace H5

/**
 * \file NexusFile.h Definition of the Nexus C++ API.
 * \defgroup cpp_types C++ Types
 * \defgroup cpp_core C++ Core
 * \ingroup cpp_main
 */

namespace Mantid {
namespace Nexus {

/**
 * \class FileID
 * \brief A wrapper class for managing HDF5 file handles (hid_t).
 *
 * The FileID class is designed to manage the lifecycle of HDF5 file handles (hid_t),
 * ensuring that the handle is properly closed when the FileID object is destroyed.
 * This helps prevent resource leaks and ensures proper cleanup of HDF5 resources.
 */
class MANTID_NEXUS_DLL FileID {
private:
  hid_t m_fid;
  // There is no reason to copy or assign a file ID
  FileID(FileID const &f) = delete;
  FileID(FileID const &&f) = delete;
  FileID &operator=(hid_t const) = delete;
  FileID &operator=(FileID const &) = delete;
  FileID &operator=(FileID const &&) = delete;

public:
  bool operator==(int const v) const { return static_cast<int>(m_fid) == v; }
  bool operator<=(int const v) const { return static_cast<int>(m_fid) <= v; }
  operator hid_t const &() const { return m_fid; }
  hid_t getId() const { return m_fid; }
  FileID() : m_fid(-1) {}
  FileID(hid_t const v) : m_fid(v) {}
  ~FileID();
};

using Deleter = int (*const)(hid_t);

/**
 * \class UniqueID
 * \brief A wrapper class for managing HDF5 object handles (hid_t).
 *
 * The UniqueID class is designed to manage the lifecycle of HDF5 object handles (hid_t),
 * ensuring that the handle is properly closed when the UniqueID object is destroyed.
 * This helps prevent resource leaks and ensures proper cleanup of HDF5 resources.
 */
template <Deleter const deleter> class UniqueID {
private:
  hid_t m_id;
  UniqueID &operator=(UniqueID<deleter> const &) = delete;
  UniqueID &operator=(UniqueID<deleter> const &&) = delete;
  void closeId() const {
    if (m_id >= 0) {
      deleter(m_id);
    }
  }

public:
  UniqueID(UniqueID<deleter> &uid) : m_id(uid.m_id) { uid.m_id = -1; };
  UniqueID(UniqueID<deleter> &&uid) noexcept : m_id(uid.m_id) { uid.m_id = -1; };
  /// @brief Assign a HDF5 object ID to be managed
  /// @param id : the ID to be managed
  UniqueID &operator=(hid_t const id) {
    if (id != m_id) {
      closeId();
      m_id = id;
    }
    return *this;
  };
  /// @brief Pass the HDF5 object ID from an existing UniqueID to another UniqueID
  /// @param uid : the UniqueID previously managing the ID; it will lose ownership of the ID.
  UniqueID &operator=(UniqueID<deleter> &uid) {
    if (this != &uid) {
      closeId();
      m_id = uid.m_id;
      uid.m_id = -1;
    }
    return *this;
  }
  bool operator==(int const v) const { return static_cast<int>(m_id) == v; }
  bool operator<=(int const v) const { return static_cast<int>(m_id) <= v; }
  operator hid_t const &() const { return m_id; };
  hid_t getId() const { return m_id; }
  /// @brief  Release hold on the managed ID; it will not be closed by this UniqueID
  /// @return the managed ID
  hid_t releaseId() {
    hid_t tmp = m_id;
    m_id = -1;
    return tmp;
  }
  UniqueID() : m_id(-1) {}
  UniqueID(hid_t const id) : m_id(id) {}
  ~UniqueID() { closeId(); }
};

/**
 * The Object that allows access to the information in the file.
 * \ingroup cpp_core
 */
class MANTID_NEXUS_DLL File {

  //------------------------------------------------------------------------------------------------------------------
  // PRIVATE MEMBERS
  //------------------------------------------------------------------------------------------------------------------
private:
  std::string m_filename;
  NXaccess m_access;
  /** The address of currently-opened element */
  NexusAddress m_address;
  /** Variables for use inside the C-API, formerly of NexusFile5
   * \li m_pfile -- shared ptr to a H5File object
   * \li m_current_group_id -- the ID for currently opened group (or 0 if none)
   * \li m_current_data_id -- the ID for currently opened dataset (or 0 if none)
   * \li m_current_type_id -- the ID of the type of the opened dataset
   * \li m_current_space_id -- the ID of the dataspace for the opened dataset
   * \li m_gid_stack -- a vector stack of opened group IDs
   */
  std::shared_ptr<FileID> m_pfile;
  hid_t m_current_group_id;
  hid_t m_current_data_id;
  hid_t m_current_type_id;
  hid_t m_current_space_id;
  std::vector<hid_t> m_gid_stack;
  /** nexus descriptor to track the file tree
   * NOTE: in file write, the following cannot be relied upon:
   * - hasRootAttr
   * - firstEntryNameType
   */
  NexusDescriptor m_descriptor;

  //------------------------------------------------------------------------------------------------------------------
  // CONSTRUCTORS / ASSIGNMENT / DECONSTRUCTOR
  //------------------------------------------------------------------------------------------------------------------
public:
  /**
   * Create a new File.
   *
   * \param filename The name of the file to open.
   * \param access How to access the file.
   */
  File(std::string const &filename, NXaccess const access = NXaccess::READ) : File(filename.c_str(), access) {}

  /**
   * Create a new File.
   *
   * \param filename The name of the file to open.
   * \param access How to access the file.
   */
  File(char const *filename, NXaccess const access = NXaccess::READ);

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
  File(File const *const pf) : File(*pf) {}

  /**
   * Copy constructor from shared pointer
   *
   * \param pf Pointer to file to copy over
   */
  File(std::shared_ptr<File> pf) : File(*pf) {}

  /**
   * Prohibit assignment, as it can lead to
   * race conditions when closing the HDF5 file handles
   */
  File &operator=(File const &f) = delete;

  /** Destructor. This does close the file. */
  ~File();

  /** Close the file before the constructor is called. */
  void close();

  /** Flush the file. */
  void flush();

private:
  /**
   * Function to consolidate the file opening code for the various constructors
   * \param filename The name of the file to open.
   * \param access How to access the file.
   */
  void initOpenFile(const std::string &filename, const NXaccess access = NXaccess::READ);

  //------------------------------------------------------------------------------------------------------------------
  // FILE NAVIGATION METHODS
  //------------------------------------------------------------------------------------------------------------------
public:
  // ADDRESS GET / OPEN

  /**
   * Open the Nexus object with the address specified.
   *
   * \param address A unix like address string to a group or field. The address
   * string is a list of group names and SDS names separated with a slash,
   * '/' (i.e. "/entry/sample/name").
   */
  void openAddress(std::string const &address);

  /**
   * Open the group in which the Nexus object with the specified address exists.
   *
   * \param address A unix like address string to a group or field. The address
   * string is a list of group names and SDS names separated with a slash,
   * '/' (i.e. "/entry/sample/name").
   */
  void openGroupAddress(std::string const &address);

  /**
   * Get the address into the current file
   * \return A unix like address string pointing to the current
   *         position in the file
   */
  std::string const &getAddress() const { return m_address.string(); };

  // CHECK ADDRESS EXISTENCE

  bool hasAddress(std::string const &) const;

  bool hasGroup(std::string const &, std::string const &) const;

  bool hasData(std::string const &) const;

  /**
   * This function checks if we are in an open dataset
   * \returns true if we are currently in an open dataset else false
   */
  bool isDataSetOpen() const;

  /** Return true if the data opened is of one of the
   * int data types, 32 bits or less.
   */
  bool isDataInt() const;

private:
  // EXPLORE FILE LEVEL ENTRIES / ATTRIBUTES

  /** get the ID of current open location
   * \returns if a datset is open, the ID of the dataset; else ID of open group
   */
  hid_t getCurrentId() const;

  /** It is sometimes necessary to interface with code using the H5Cpp library
   * This method will return the current location as an object, which can be
   * used with, for instance, H5Util or other parts of Mantid
   * \returns a shared pointer to an H5Object corresponding to current location
   */
  std::shared_ptr<H5::H5Object> getCurrentObject() const;

  // these are used for updating the NexusDescriptor
  NexusAddress groupAddress(NexusAddress const &) const;
  NexusAddress formAbsoluteAddress(NexusAddress const &) const;
  void registerEntry(std::string const &, std::string const &);

  //------------------------------------------------------------------------------------------------------------------
  // GROUP MAKE / OPEN / CLOSE
  //------------------------------------------------------------------------------------------------------------------
public:
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
   * Close the currently open group.
   */
  void closeGroup();

  //------------------------------------------------------------------------------------------------------------------
  // DATA MAKE / OPEN / PUT / GET / CLOSE
  //------------------------------------------------------------------------------------------------------------------

  /**
   * Create a data field with the specified information.
   *
   * \param name The name of the field to create (i.e. "distance").
   * \param type The primative type of the field (i.e. "Nexus::FLOAT32").
   * \param dims The dimensions of the field.
   * \param open_data Whether or not to open the data after creating it.
   */
  void makeData(std::string const &name, NXnumtype const type, DimVector const &dims, bool const open_data = false);

  /**
   * Create a data field with the specified information.
   *
   * \param name The name of the field to create (i.e. "distance").
   * \param type The primative type of the field (i.e. "Nexus::FLOAT32").
   * \param length For 1D data, the length of the 1D array
   * \param open_data Whether or not to open the data after creating it.
   */
  void makeData(std::string const &name, NXnumtype const type, dimsize_t const length, bool const open_data = false);

  /**
   * \param name The name of the data to open.
   */
  void openData(std::string const &name);

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

  /**
   * \return String data from the file.
   */
  std::string getStrData();

  /**
   * Close the currently open data.
   */
  void closeData();

  //------------------------------------------------------------------------------------------------------------------
  // DATA MAKE COMP / PUT/GET SLAB / COERCE
  //------------------------------------------------------------------------------------------------------------------

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
                    DimVector const &bufsize, bool open_data = false);

  /**
   * Insert an array as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   */
  template <typename NumT> void putSlab(NumT const *data, DimVector const &start, DimVector const &size);

  /**
   * Insert an array as part of a data in the final file.
   *
   * \param data The array to put in the file.
   * \param start The starting index to insert the data.
   * \param size The size of the array to put in the file.
   * \tparam NumT numeric data type of \a data
   */
  template <typename NumT> void putSlab(std::vector<NumT> const &data, DimVector const &start, DimVector const &size);

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
   * Get a section of data from the file.
   *
   * \param data The pointer to insert that data into.
   * \param start The offset into the file's data block to start the read
   * from.
   * \param size The size of the block to read from the file.
   */
  template <typename NumT> void getSlab(NumT *data, DimVector const &start, DimVector const &size);

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

  //------------------------------------------------------------------------------------------------------------------
  // DATA READ / WRITE
  //------------------------------------------------------------------------------------------------------------------

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
                           DimVector const &chunk);

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
                     NXcompression const comp, DimVector const &bufsize);

  /*----------------------------------------------------------------------*/

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

  //------------------------------------------------------------------------------------------------------------------
  // ENTRY METHODS
  //------------------------------------------------------------------------------------------------------------------

  /**
   * \return The Info structure that describes the currently open data.
   */
  Info getInfo();

  /**
   * Return the entries available in the current place in the file.
   */
  Entries getEntries() const;

  /** Return the entries available in the current place in the file,
   * but avoids the map copy of getEntries().
   *
   * \param result The map that will be filled with the entries
   */
  void getEntries(Entries &result) const;

  /** Return the string name of the top-level entry
   *
   * \return a string with the name (not abs address) of the top-level entry
   */
  std::string getTopLevelEntryName() const;

  //------------------------------------------------------------------------------------------------------------------
  // ATTRIBUTE METHODS
  //------------------------------------------------------------------------------------------------------------------

  // PUT / GET ATTRIBUTES

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
   * Get the value of an attribute that is a scalar number.
   *
   * \param name Name of attribute to read.
   * \tparam NumT numeric data type of result
   *
   * \return The attribute value.
   */
  template <typename NumT> NumT getAttr(std::string const &name);

  /**
   * Get the value of an attribute that is a scalar number.
   * Only use this method if you do not care about precisely matching the data type on disk
   *
   * \param[in] name Name of attribute to read
   * \param[out] value The read attribute value.
   * \tparam NumT numeric data type of \a value
   */
  template <typename NumT> void getAttr(std::string const &name, NumT &value);

  /**
   * Get the value of a string attribute.
   *
   * \param name Name of attribute to read.
   *
   * \return The value of the attribute.
   */
  std::string getStrAttr(std::string const &name);

  // NAVIGATE ATTRIBUTES

  /**
   * \return Information about all attributes on the data that is currently open.
   */
  std::vector<AttrInfo> getAttrInfos();

  /**
   *  \return true if the current point in the file has the named attribute
   *  \param name the name of the attribute to look for.
   */
  bool hasAttr(const std::string &name) const;

  //------------------------------------------------------------------------------------------------------------------
  // LINK METHODS
  //------------------------------------------------------------------------------------------------------------------

  /**
   * \return The id of the group used for linking.
   */
  NXlink getGroupID();

  /**
   * \return The id of the data used for linking.
   */
  NXlink getDataID();

  /**
   * Create a link in the current location to the supplied id.
   *
   * \param link The object (group or data) in the file to link to.
   */
  void makeLink(NXlink const &link);
};

/**
 * This function returns the NXnumtype given a concrete number.
 * \tparam NumT numeric data type of \a number to check
 */
template <typename NumT> MANTID_NEXUS_DLL NXnumtype getType(NumT const number = NumT());

} // namespace Nexus
} // namespace Mantid
