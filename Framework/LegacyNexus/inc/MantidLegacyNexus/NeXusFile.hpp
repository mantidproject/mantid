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

static std::pair<std::string, std::string> const EOD_ENTRY("NULL", "NULL");

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

  /** Destructor. This does close the file. */
  ~File();

  /** Close the file before the constructor is called. */
  void close();

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
   * \param name The name of the data to open.
   */
  void openData(const std::string &name);

  /**
   * Close the currently open data.
   */
  void closeData();

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
};

/**
 * This function returns the NXnumtype given a concrete number.
 * \tparam NumT numeric data type of \a number to check
 */
template <typename NumT> MANTID_LEGACYNEXUS_DLL NXnumtype getType(NumT const number = NumT());

}; // namespace Mantid::LegacyNexus
