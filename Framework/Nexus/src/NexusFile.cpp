#include <cstring>
// REMOVE
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NexusException.h"
#include "MantidNexus/NexusFile.h"
#include "MantidNexus/hdf5_type_helper.h"
#include "MantidNexus/inverted_napi.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <H5Cpp.h>
#include <Poco/Logger.h>
#include <algorithm>
#include <array>
#include <assert.h>
#include <filesystem>
#include <hdf5.h>
#include <iostream>
#include <numeric>
#include <sstream>
#include <typeinfo>

using std::string;
using std::stringstream;
using std::vector;

#ifdef H5_VERSION_GE
#if !H5_VERSION_GE(1, 8, 0)
#error HDF5 Version must be 1.8.0 or higher
#endif
#endif

#ifdef WIN32
#define snprintf _snprintf
#define strdup _strdup
#endif

#define NXEXCEPTION(message) Exception((message), __func__, m_filename);

namespace {
/// static logger object. Use Poco directly instead of Kernel::Logger so we don't need to import from Kernel
const auto g_log = &Poco::Logger::get("NexusFile");
} // namespace

/**
 * \file NexusFile.cpp
 * The implementation of the Nexus C++ API
 */

namespace { // anonymous namespace to keep it in the file

std::string const GROUP_CLASS_SPEC("NX_class");
std::string const TARGET_ATTR_NAME("target");
std::string const SCIENTIFIC_DATA_SET("SDS");
std::string const UNKNOWN_GROUP_SPEC("NX_UNKNOWN_GROUP");
constexpr int DEFAULT_DEFLATE_LEVEL(6);

template <typename NumT> static string toString(const vector<NumT> &data) {
  stringstream result;
  result << "[";
  size_t size = data.size();
  for (size_t i = 0; i < size; i++) {
    result << data[i];
    if (i + 1 < size) {
      result << ",";
    }
  }
  result << "]";
  return result.str();
}

} // end of anonymous namespace

namespace Mantid::Nexus {

// catch for undefined types
template <typename NumT> NXnumtype getType(NumT const number) {
  stringstream msg;
  msg << "Nexus::getType() does not know type of " << typeid(number).name();
  throw Exception(msg.str(), "NXnumtype getType<NumT>");
}

// template specialisations for types we know
template <> MANTID_NEXUS_DLL NXnumtype getType(float const) { return NXnumtype::FLOAT32; }

template <> MANTID_NEXUS_DLL NXnumtype getType(double const) { return NXnumtype::FLOAT64; }

template <> MANTID_NEXUS_DLL NXnumtype getType(int8_t const) { return NXnumtype::INT8; }

template <> MANTID_NEXUS_DLL NXnumtype getType(uint8_t const) { return NXnumtype::UINT8; }

template <> MANTID_NEXUS_DLL NXnumtype getType(int16_t const) { return NXnumtype::INT16; }

template <> MANTID_NEXUS_DLL NXnumtype getType(uint16_t const) { return NXnumtype::UINT16; }

template <> MANTID_NEXUS_DLL NXnumtype getType(int32_t const) { return NXnumtype::INT32; }

template <> MANTID_NEXUS_DLL NXnumtype getType(uint32_t const) { return NXnumtype::UINT32; }

template <> MANTID_NEXUS_DLL NXnumtype getType(int64_t const) { return NXnumtype::INT64; }

template <> MANTID_NEXUS_DLL NXnumtype getType(uint64_t const) { return NXnumtype::UINT64; }

template <> MANTID_NEXUS_DLL NXnumtype getType(char const) { return NXnumtype::CHAR; }

template <> MANTID_NEXUS_DLL NXnumtype getType(string const) { return NXnumtype::CHAR; }

FileID::~FileID() {
  if (H5Iis_valid(m_fid)) {
    H5Fclose(m_fid);
    H5garbage_collect();
    m_fid = -1;
  }
}

} // namespace Mantid::Nexus

namespace Mantid::Nexus {
//------------------------------------------------------------------------------------------------------------------
// CONSTRUCTORS / ASSIGNMENT / DECONSTRUCTOR
//------------------------------------------------------------------------------------------------------------------

// new constructors

File::File(const char *filename, const NXaccess access)
    : m_filename(filename), m_access(access), m_address(), m_current_group_id(0), m_current_data_id(0),
      m_current_type_id(0), m_current_space_id(0), m_gid_stack{0}, m_descriptor(m_filename, m_access) {
  this->initOpenFile(m_filename, m_access);
}

void File::initOpenFile(std::string const &filename, NXaccess const am) {
  if (filename.empty()) {
    throw NXEXCEPTION("Filename specified is empty constructor");
  }
  // check HDF5 version installed
  unsigned int vers_major, vers_minor, vers_release;
  if (H5get_libversion(&vers_major, &vers_minor, &vers_release) < 0) {
    throw NXEXCEPTION("Cannot determine HDF5 library version");
  }
  if (vers_major == 1 && vers_minor < 8) {
    throw NXEXCEPTION("HDF5 library 1.8.0 or higher required");
  }
  std::string version_str =
      std::to_string(vers_major) + "." + std::to_string(vers_minor) + "." + std::to_string(vers_release);
  // turn off the automatic HDF error handling
  H5Eset_auto(H5E_DEFAULT, nullptr, nullptr);

  // create file acccess property list
  ParameterID fapl = H5Pcopy(H5Util::defaultFileAcc().getId());

  hid_t temp_fid(-1);
  if (am != NXaccess::CREATE5) {
    if (H5Fis_accessible(filename.c_str(), fapl) <= 0) {
      throw NXEXCEPTION("File is not HDF5");
    }
    temp_fid = H5Fopen(filename.c_str(), (unsigned)am, fapl);
  } else {
    temp_fid = H5Fcreate(filename.c_str(), (unsigned)am, H5P_DEFAULT, fapl);
  }

  if (temp_fid <= 0) {
    throw NXEXCEPTION("Cannot open file");
  }

  // if in creation mode, must add the following attributes
  // - file_name
  // - file_time
  // - Nexus version
  // - HDF5 version
  if (am == NXaccess::CREATE5) {
    // open the root as a group and add these attributes
    // use H5Cpp to interface with H5Util
    GroupID root_id = H5Gopen(temp_fid, "/", H5P_DEFAULT);
    H5::Group root(root_id);
    std::vector<Entry> attrs{{"NeXus_version", NEXUS_VERSION},
                             {"file_name", filename},
                             {"HDF5_Version", version_str},
                             {"file_time", Mantid::Types::Core::DateAndTime::getLocalTimeISO8601String()},
                             {GROUP_CLASS_SPEC, "NXroot"}};
    for (auto const &attr : attrs) {
      H5Util::writeStrAttribute(root, attr.first, attr.second);
    }
    root.close();
    H5Gflush(root_id);
  }
  H5Fflush(temp_fid, H5F_SCOPE_GLOBAL);

  m_gid_stack[0] = 0; // root!
  if (temp_fid <= 0) {
    stringstream msg;
    msg << "File::initOpenFile(" << filename << ", " << am << ") failed";
    throw NXEXCEPTION(msg.str());
  } else {
    m_pfile = std::make_shared<FileID>(temp_fid);
  }
}

// copy constructors

File::File(File const &f)
    : m_filename(f.m_filename), m_access(f.m_access), m_address(), m_pfile(f.m_pfile), m_current_group_id(0),
      m_current_data_id(0), m_current_type_id(0), m_current_space_id(0), m_gid_stack{0}, m_descriptor(f.m_descriptor) {
  // NOTE warning to future devs
  // if you change this method, please run the systemtest VanadiumAndFocusWithSolidAngleTest
  if (m_pfile->getId() <= 0)
    throw NXEXCEPTION("Error reopening file");
}

// deconstructor

File::~File() {
  // release all open groups and datasets
  if (m_current_data_id != 0) {
    H5Dclose(m_current_data_id);
    m_current_data_id = 0;
  }
  if (m_current_type_id != 0) {
    H5Tclose(m_current_type_id);
    m_current_type_id = 0;
  }
  if (m_current_space_id != 0) {
    H5Sclose(m_current_space_id);
    m_current_space_id = 0;
  }
  if (m_current_group_id != 0) {
    H5Gclose(m_current_group_id);
    m_current_group_id = 0;
  }
  for (hid_t &gid : m_gid_stack) {
    H5Gclose(gid);
    gid = 0;
  }
  m_gid_stack.clear();
  // NOTE warning to future devs
  // if you change this part of method, please run the systemtest VanadiumAndFocusWithSolidAngleTest
  // decrease reference counts to this file
  close();
  H5garbage_collect();
}

void File::close() {
  // NOTE warning to future devs
  // if you change this method, please run the systemtest VanadiumAndFocusWithSolidAngleTest
  if (m_pfile != nullptr) {
    // decrease reference counts to this file
    m_pfile.reset();
  }
}

void File::flush() {
  hid_t vid = getCurrentId();
  herr_t iRet = H5Fflush(vid, H5F_SCOPE_LOCAL);
  if (iRet < 0) {
    throw NXEXCEPTION("The object cannot be flushed");
  }
}

//------------------------------------------------------------------------------------------------------------------
// FILE NAVIGATION METHODS
//------------------------------------------------------------------------------------------------------------------

void File::openAddress(std::string const &address) {
  if (address.empty()) {
    throw NXEXCEPTION("Supplied empty address");
  }

  // if we are already there, do nothing
  NexusAddress absaddr(formAbsoluteAddress(address));
  if (absaddr == m_address) {
    return;
  }

  // confirm the address exists before trying to open
  if (!hasAddress(absaddr)) {
    throw NXEXCEPTION("Address " + address + " is not valid");
  }

  // if a dataset is open, close it
  if (isDataSetOpen()) {
    H5Dclose(m_current_data_id);
    H5Tclose(m_current_type_id);
    H5Sclose(m_current_space_id);
    m_current_data_id = 0;
    m_current_space_id = 0;
    m_current_type_id = 0;
  }

  // close all groups in the stack
  for (hid_t const &gid : m_gid_stack) {
    if (gid != 0) {
      H5Gclose(gid);
    }
  }
  m_gid_stack.clear();
  m_gid_stack.push_back(0);
  m_address = NexusAddress::root();

  // if we wanted to go to root, then stop here
  if (absaddr == NexusAddress::root()) {
    m_current_group_id = 0;
    return;
  }

  // open all groups in the address
  NexusAddress groupstack(absaddr.parent_path());
  NexusAddress fromroot;
  if (groupstack.isRoot()) {
    m_current_group_id = 0;
  } else {
    m_current_group_id = m_pfile->getId();
    for (auto const &name : groupstack.parts()) {
      fromroot /= name;
      if (m_descriptor.isEntry(fromroot, m_descriptor.classTypeForName(fromroot))) {
        hid_t gid = H5Gopen(m_pfile->getId(), fromroot.c_str(), H5P_DEFAULT);
        m_gid_stack.push_back(gid);
        // update stack in case of failure
        m_current_group_id = gid;
        m_address = fromroot;
      } else {
        throw NXEXCEPTION("Failed to open " + name + " while opening " + absaddr);
      }
    }
  }
  // open the last element -- either a group or a dataset
  if (hasData(absaddr)) { // is a dataset
    m_current_data_id = H5Dopen(m_pfile->getId(), absaddr.c_str(), H5P_DEFAULT);
    m_current_type_id = H5Dget_type(m_current_data_id);
    m_current_space_id = H5Dget_space(m_current_data_id);
  } else if (hasAddress(absaddr)) { // not a dataset but exists = is a group
    hid_t gid = H5Gopen(m_pfile->getId(), absaddr.c_str(), H5P_DEFAULT);
    m_gid_stack.push_back(gid);
    m_current_group_id = gid;
  } else {
    throw NXEXCEPTION("Failed to open final element of address " + absaddr);
  }
  m_address = absaddr;
}

void File::openGroupAddress(std::string const &address) {
  if (address.empty()) {
    throw NXEXCEPTION("Supplied empty address");
  }
  // get the address of the containing group and open it
  openAddress(groupAddress(formAbsoluteAddress(address)));
}

NexusAddress File::groupAddress(NexusAddress const &addr) const {
  if (hasData(addr)) {
    return addr.parent_path();
  } else if (hasAddress(addr)) {
    return addr;
  } else {
    throw NXEXCEPTION("Address " + addr + " does not exist");
  }
}

bool File::hasAddress(std::string const &name) const {
  if (name == "/") { // NexusDescriptor does not keep the root, but it does exist
    return true;
  } else {
    return m_descriptor.isEntry(formAbsoluteAddress(name));
  }
}

bool File::hasGroup(std::string const &name, std::string const &class_type) const {
  std::string const address = formAbsoluteAddress(name);
  return m_descriptor.isEntry(address, class_type);
}

bool File::hasData(std::string const &name) const {
  std::string const address = formAbsoluteAddress(name);
  return m_descriptor.isEntry(address, SCIENTIFIC_DATA_SET);
}

bool File::isDataSetOpen() const {
  if (m_current_data_id == 0) {
    return false;
  } else {
    return H5Iget_type(m_current_data_id) == H5I_DATASET;
  }
}

bool File::isDataInt() const {
  if (m_current_type_id == 0) {
    throw NXEXCEPTION("No dataset is open");
  }
  return H5Tget_class(m_current_type_id) == H5T_INTEGER;
}

NexusAddress File::formAbsoluteAddress(NexusAddress const &name) const {
  NexusAddress new_name(name);
  if (new_name.isAbsolute()) {
    new_name = name;
  } else {
    new_name = groupAddress(getAddress()) / name;
  }
  // the caller is responsible for checking that it exists
  return new_name;
}

hid_t File::getCurrentId() const {
  if (m_current_data_id != 0) {
    return m_current_data_id;
  } else if (m_current_group_id != 0) {
    return m_current_group_id;
  } else {
    return m_pfile->getId();
  }
}

std::shared_ptr<H5::H5Object> File::getCurrentObject() const {
  if (m_current_data_id != 0) {
    return std::make_shared<H5::DataSet>(m_current_data_id);
  } else if (m_current_group_id != 0) {
    return std::make_shared<H5::Group>(m_current_group_id);
  } else {
    return std::make_shared<H5::H5File>(m_pfile->getId());
  }
}

void File::registerEntry(std::string const &address, std::string const &name) {
  if (m_descriptor.isEntry(address, name)) {
    // do nothing
  } else if (address.front() != '/') {
    throw NXEXCEPTION("Address must be absolute: " + address);
  } else {
    // NOTE the caller is responsible for only registering valid address
    m_descriptor.addEntry(address, name);
  }
}

//------------------------------------------------------------------------------------------------------------------
// GROUP MAKE / OPEN / CLOSE
//------------------------------------------------------------------------------------------------------------------

void File::makeGroup(const std::string &name, const std::string &nxclass, bool open_group) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to makeGroup");
  }
  if (nxclass.empty()) {
    throw NXEXCEPTION("Supplied empty class name to makeGroup");
  }

  // get full address
  NexusAddress const absaddr(formAbsoluteAddress(name));
  // create group with H5Util by getting an H5File object from iFID
  H5::H5File h5file(m_pfile->getId());
  H5::Group grp = H5Util::createGroupNXS(h5file, absaddr, nxclass);

  // cleanup
  registerEntry(absaddr, nxclass);
  if (open_group) {
    // grp will close when it goes out of scope -- open new copy
    m_current_group_id = H5Gopen(grp.getId(), ".", H5P_DEFAULT);
    m_gid_stack.push_back(m_current_group_id);
    m_address = absaddr;
    // if we are opening a new group, close whatever dataset is already open
    if (m_current_data_id != 0) {
      closeData();
    }
  }
}

void File::openGroup(std::string const &name, std::string const &nxclass) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name");
  }
  if (nxclass.empty()) {
    throw NXEXCEPTION("Supplied empty class name");
  }

  NexusAddress const absaddr(formAbsoluteAddress(name));
  if (absaddr == getAddress()) {
    // we are already there
    return;
  }

  hid_t iVID;
  if (m_descriptor.isEntry(absaddr, nxclass)) {
    iVID = H5Gopen(m_pfile->getId(), absaddr.c_str(), H5P_DEFAULT);
  } else {
    throw NXEXCEPTION("The supplied group " + absaddr + " does not exist");
  }

  if (iVID < 0) {
    std::stringstream msg;
    msg << "Group " << absaddr.string() << " does not exist";
    throw NXEXCEPTION(msg.str());
  }

  /* maintain stack */
  m_current_group_id = iVID;
  m_gid_stack.push_back(iVID);
  m_address = absaddr;
  // if we are opening a new group, close whatever dataset is already open
  if (m_current_data_id != 0) {
    closeData();
  }
}

void File::closeGroup() {
  if (m_current_group_id == 0) {
    // do nothing
  } else {
    // if a group is closed while a dataset is still open,
    // make sure the dataset and all its parts are closed
    if (m_current_data_id != 0) {
      closeData();
    }
    // close the current group and maintain stack
    if (H5Gclose(m_current_group_id) < 0) {
      throw NXEXCEPTION("Failed to close group at " + m_address);
    }
    m_gid_stack.pop_back();
    if (!m_gid_stack.empty()) {
      m_current_group_id = m_gid_stack.back();
    } else {
      m_current_group_id = 0;
    }
    m_address = m_address.parent_path();
  }
}

//------------------------------------------------------------------------------------------------------------------
// DATA MAKE / OPEN / CLOSE / PUT / GET
//------------------------------------------------------------------------------------------------------------------

void File::makeData(const string &name, NXnumtype const type, DimVector const &dims, bool const open_data) {
  // error check the parameters
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty label");
  }
  if (dims.empty()) {
    throw NXEXCEPTION("Supplied empty dimensions");
  }
  DimVector chunk_size(dims.size());
  for (std::size_t i = 0; i < dims.size(); i++) {
    chunk_size[i] = (dims[i] == NX_UNLIMITED || dims[i] <= 0 ? 1 : dims[i]);
  }
  this->makeCompData(name, type, dims, NXcompression::NONE, chunk_size, open_data);
}

void File::makeData(const string &name, NXnumtype const type, dimsize_t const length, bool const open_data) {
  this->makeData(name, type, DimVector({length}), open_data);
}

void File::openData(std::string const &name) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to openData");
  }

  // close any open dataset
  if (m_current_data_id != 0) {
    closeData();
  }
  m_current_data_id = 0;
  m_current_type_id = 0;
  m_current_space_id = 0;

  NexusAddress absaddr(formAbsoluteAddress(name));

  /* find the ID number and open the dataset */
  DataSetID newData = H5Dopen(m_pfile->getId(), absaddr.c_str(), H5P_DEFAULT);
  if (!newData.isValid()) {
    throw NXEXCEPTION("Dataset (" + absaddr + ") not found at this level");
  }
  /* find the ID number of datatype */
  DataTypeID newType = H5Dget_type(newData);
  if (!newType.isValid()) {
    throw NXEXCEPTION("Error opening dataset (" + absaddr + ")");
  }
  /* find the ID number of dataspace */
  DataSpaceID newSpace = H5Dget_space(newData);
  if (!newSpace.isValid()) {
    throw NXEXCEPTION("Error opening dataset (" + absaddr + ")");
  }
  // now maintain stack
  m_current_data_id = newData.release();
  m_current_type_id = newType.release();
  m_current_space_id = newSpace.release();
  m_address = absaddr;
}

void File::closeData() {
  herr_t iRet;
  if (m_current_space_id != 0) {
    iRet = H5Sclose(m_current_space_id);
    if (iRet < 0) {
      throw NXEXCEPTION("Cannot end access to dataset: failed to close dataspace");
    }
  }
  if (m_current_type_id != 0) {
    iRet = H5Tclose(m_current_type_id);
    if (iRet < 0) {
      throw NXEXCEPTION("Cannot end access to dataset: failed to close datatype");
    }
  }
  if (m_current_data_id != 0) {
    iRet = H5Dclose(m_current_data_id);
    if (iRet < 0) {
      throw NXEXCEPTION("Cannot end access to dataset: failed to close dataset");
    }
  } else {
    throw NXEXCEPTION("Cannot end access to dataset: no data open");
  }
  m_current_data_id = 0;
  m_current_space_id = 0;
  m_current_type_id = 0;
  m_address = m_address.parent_path();
}

// PUT DATA

template <typename NumT> void File::putData(NumT const *data) {
  if (data == nullptr) {
    throw NXEXCEPTION("Data specified as null");
  }

  // get rank for proper size of dimension vectors
  int rank = H5Sget_simple_extent_ndims(m_current_space_id);
  if (rank < 0) {
    throw NXEXCEPTION("Cannot determine dataset rank");
  } else if (rank == 0) { // scalars have no extent, so cannot be unlimited
    herr_t iRet = H5Dwrite(m_current_data_id, m_current_type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (iRet < 0) {
      throw NXEXCEPTION("Failure to write data");
    }
  } else { // check for unlimited marker in any of the data
    DimVector thedims(rank, 0), maxdims(rank, 0);
    herr_t iRet = H5Sget_simple_extent_dims(m_current_space_id, thedims.data(), maxdims.data());
    if (iRet < 0) {
      throw NXEXCEPTION("Cannot determine dataset dimensions");
    }
    /* If we are using putdata on an unlimited dimension dataset, assume we want to append one single new slab */
    if (std::any_of(maxdims.cbegin(), maxdims.cend(), [](auto x) -> bool { return x == H5S_UNLIMITED; })) {
      DimVector vecStart(rank, 0), vecSize(rank, 0);
      for (int i = 0; i < rank; i++) {
        if (maxdims[i] == H5S_UNLIMITED) {
          vecStart[i] = thedims[i] + 1;
          vecSize[i] = 1;
        } else {
          vecStart[i] = 0;
          vecSize[i] = thedims[i];
        }
      }
      return putSlab(data, vecStart, vecSize);
    } else {
      iRet = H5Dwrite(m_current_data_id, m_current_type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
      if (iRet < 0) {
        throw NXEXCEPTION("Failure to write data");
      }
    }
  }
}

template <> MANTID_NEXUS_DLL void File::putData(std::string const *data) { this->putData(*data); }

void File::putData(std::string const &data) {
  if (data.empty()) {
    throw NXEXCEPTION("Supplied empty stirng to putData");
  }
  this->putData<char>(data.c_str());
}

template <typename NumT> void File::putData(const vector<NumT> &data) {
  if (data.empty()) {
    throw NXEXCEPTION("Supplied empty data to putData");
  }
  this->putData(data.data());
}

// GET DATA -- STRING / CHAR

template <> void File::getData<char>(char *data) {
  if (m_current_data_id == 0) {
    throw NXEXCEPTION("getData ERROR: no dataset open");
  }

  hsize_t dims[H5S_MAX_RANK] = {0};
  int rank = H5Sget_simple_extent_dims(m_current_space_id, dims, nullptr);
  herr_t ret = -1;
  std::size_t size = 0;
  std::vector<char> buffer;

  if (H5Tis_variable_str(m_current_type_id)) {
    char *cdata = nullptr;
    ret = H5Dread(m_current_data_id, m_current_type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &cdata);
    if (ret < 0 || cdata == nullptr)
      throw NXEXCEPTION("getData ERROR: failed to read variable length string dataset");

    size = strlen(cdata);
    buffer.assign(cdata, cdata + size + 1);
    H5free_memory(cdata);
  } else {
    hsize_t len = H5Tget_size(m_current_type_id);
    for (int i = 0; i < rank - 1; i++) {
      len *= (dims[i] > 1 ? dims[i] : 1);
    }

    buffer.resize(len + 1, '\0');
    ret = H5Dread(m_current_data_id, m_current_type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
    if (ret < 0)
      throw NXEXCEPTION("getData ERROR: failed to read string\n");

    buffer[len] = '\0';
    size = len;
  }

  // strip whitespace
  if (rank == 0 || rank == 1) {
    if (size == 1) {
      if (isspace(buffer[0])) {
        *data = '\0'; // if the only character is whitespace, return null
      } else if (buffer[0] == '\0') {
        *data = '\0'; // if the only character is null, return null
      } else {
        *data = buffer[0];
      }
      return;
    }
    // skip over any front whitespace
    char *start = buffer.data();
    while (*start && isspace(*start))
      ++start;
    // work from back until first non-whitespace char found
    int i = (int)strlen(start);
    while (--i >= 0) {
      if (!isspace(start[i])) {
        break;
      }
    }
    // add a null terminator to the end
    start[++i] = '\0';

    std::memcpy(data, start, i);
    data[i] = '\0';
  } else {
    std::memcpy(data, buffer.data(), size);
  }
}

string File::getStrData() {
  Info info = this->getInfo();
  if (info.type != NXnumtype::CHAR) {
    stringstream msg;
    msg << "Cannot use getStrData() on non-character data. Found type=" << info.type;
    throw NXEXCEPTION(msg.str());
  }
  if (info.dims.size() != 1) {
    stringstream msg;
    msg << "getStrData() only understand rank=1 data. Found rank=" << info.dims.size();
    throw NXEXCEPTION(msg.str());
  }
  std::vector<char> value(static_cast<size_t>(info.dims[0]) + 1, '\0');
  this->getData(value.data());
  std::string res(value.data(), strlen(value.data()));
  return res;
}

// GET DATA -- NUMERIC

template <typename NumT> void File::getData(NumT *data) {
  if (!data) {
    throw NXEXCEPTION("Supplied null pointer to hold data");
  }

  if (m_current_data_id == 0) {
    throw NXEXCEPTION("No dataset open");
  }

  herr_t ret = -1;
  hsize_t dims[H5S_MAX_RANK];
  H5Sget_simple_extent_dims(m_current_space_id, dims, nullptr);
  hsize_t ndims = H5Sget_simple_extent_ndims(m_current_space_id);

  if (ndims == 0) {
    DataTypeID datatype = H5Dget_type(m_current_data_id);
    DataSpaceID filespace = H5Dget_space(m_current_data_id);
    DataSpaceID memspace_id = H5Screate(H5S_SCALAR);
    H5Sselect_all(filespace);
    ret = H5Dread(m_current_data_id, datatype, memspace_id, filespace, H5P_DEFAULT, data);
  } else {
    if (H5Tget_class(m_current_type_id) == H5T_STRING) {
      this->getData<char>(reinterpret_cast<char *>(data));
      return;
    }
    hid_t memtype_id = h5MemType(m_current_type_id);
    ret = H5Dread(m_current_data_id, memtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  }

  if (ret < 0) {
    throw NXEXCEPTION("Failed to read dataset");
  }
}

template <typename NumT> void File::getData(vector<NumT> &data) {
  Info info = this->getInfo();

  if (info.type != getType<NumT>()) {
    std::stringstream msg;
    msg << "inconsistent NXnumtype file datatype=" << info.type << " supplied datatype=" << getType<NumT>();
    throw NXEXCEPTION(msg.str());
  }
  // determine the number of elements
  size_t length =
      std::accumulate(info.dims.cbegin(), info.dims.cend(), static_cast<size_t>(1),
                      [](auto const subtotal, auto const &value) { return subtotal * static_cast<size_t>(value); });

  // allocate memory to put the data into
  // need to use resize() rather than reserve() so vector length gets set
  data.resize(length);

  // fetch the data
  this->getData<NumT>(data.data());
}

//------------------------------------------------------------------------------------------------------------------
// DATA MAKE COMP / PUT/GET SLAB / COERCE
//------------------------------------------------------------------------------------------------------------------

void File::makeCompData(std::string const &name, NXnumtype const type, DimVector const &dims, NXcompression comp,
                        DimVector const &chunk, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to makeCompData");
  }
  if (dims.empty()) {
    throw NXEXCEPTION("Supplied empty dimensions to makeCompData");
  }
  if (chunk.empty()) {
    throw NXEXCEPTION("Supplied empty chunk size to makeCompData");
  }
  if (dims.size() != chunk.size()) {
    stringstream msg;
    msg << "Supplied dims rank=" << dims.size() << " must match supplied chunk rank=" << chunk.size()
        << "in makeCompData";
    throw NXEXCEPTION(msg.str());
  }

  stringstream msg;
  msg << "makeCompData(" << name << ", " << type << ", " << dims.size() << ", " << toString(dims) << ", " << comp
      << ", " << toString(chunk) << ") failed: ";

  if (m_current_group_id <= 0) {
    msg << "No group open for makedata on " << name;
    throw NXEXCEPTION(msg.str());
  }

  // check if any data is unlimited
  bool unlimited = std::any_of(dims.cbegin(), dims.cend(), [](auto x) -> bool { return x == NX_UNLIMITED || x <= 0; });

  // set the dimensions for use
  int rank = static_cast<int>(dims.size());
  DimVector mydim(dims.cbegin(), dims.cend());
  DimVector maxdims(dims.cbegin(), dims.cend());
  DimVector chunkdims(chunk.cbegin(), chunk.cend());
  // handle unlimited data
  if (unlimited) {
    for (int i = 0; i < rank; i++) {
      if (dims[i] <= 0 || dims[i] == NX_UNLIMITED) {
        mydim[i] = 1;
        maxdims[i] = H5S_UNLIMITED;
      }
    }
  }

  // create the correct datatype for this numeric type
  DataTypeID datatype = H5Tcopy(nxToHDF5Type(type));

  // create a dataspace
  DataSpaceID dataspace;
  if (type == NXnumtype::CHAR) {
    std::size_t byte_zahl(mydim.back());
    DimVector mydim1(mydim.cbegin(), mydim.cend());
    if (unlimited) {
      mydim1[0] = 1;
      maxdims[0] = H5S_UNLIMITED;
    }
    mydim1.back() = 1;
    if (mydim.back() > 1) {
      mydim.back() = maxdims.back() = 1;
    }
    if (chunkdims.back() > 1) {
      chunkdims.back() = 1;
    }
    dataspace = H5Screate_simple(rank, mydim1.data(), maxdims.data());
    H5Tset_size(datatype, byte_zahl);
  } else {
    if (unlimited) {
      dataspace = H5Screate_simple(rank, mydim.data(), maxdims.data());
    } else {
      dataspace = H5Screate_simple(rank, mydim.data(), nullptr);
    }
  }

  // set the compression parameters
  ParameterID cparms = H5Pcreate(H5P_DATASET_CREATE);
  if (comp == NXcompression::LZW) {
    herr_t ret = H5Pset_chunk(cparms, rank, chunkdims.data());
    if (ret < 0) {
      msg << "Size of chunks could not be set";
      throw NXEXCEPTION(msg.str());
    }
    H5Pset_shuffle(cparms); // mrt: improves compression
    H5Pset_deflate(cparms, DEFAULT_DEFLATE_LEVEL);
  }
  // NOTE if compression is NONE but a dimension is unlimited, then it still compresses by CHUNK.
  // this behavior is inherited from napi
  else if (comp == NXcompression::NONE) {
    if (unlimited) {
      herr_t ret = H5Pset_chunk(cparms, rank, chunkdims.data());
      if (ret < 0) {
        msg << "Size of chunks could not be set";
        throw NXEXCEPTION(msg.str());
      }
    } else {
      cparms.reset(H5Pcopy(H5P_DEFAULT));
    }
  } else if (comp == NXcompression::CHUNK) {
    herr_t ret = H5Pset_chunk(cparms, rank, chunkdims.data());
    if (ret < 0) {
      msg << "Size of chunks could not be set";
      throw NXEXCEPTION(msg.str());
    }
  } else {
    g_log->error("HDF5 doesn't support selected compression method! Dataset created without compression");
    cparms.reset(H5Pcopy(H5P_DEFAULT));
  }

  // create the dataset with the compression parameters
  NexusAddress absaddr(formAbsoluteAddress(name));
  DataSetID dataset =
      H5Dcreate(m_pfile->getId(), absaddr.c_str(), datatype, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
  if (!dataset.isValid()) {
    msg << "Creating chunked dataset failed";
    throw NXEXCEPTION(msg.str());
  }
  if (unlimited) {
    herr_t ret = H5Dset_extent(dataset, mydim.data());
    if (ret < 0) {
      msg << "Cannot create dataset " << name;
      throw NXEXCEPTION(msg.str());
    }
  }
  // cleanup
  registerEntry(absaddr, SCIENTIFIC_DATA_SET);
  if (open_data) {
    m_current_type_id = datatype.release();
    m_current_space_id = dataspace.release();
    m_current_data_id = dataset.release();
    m_address = absaddr;
  }
}

template <typename NumT> void File::putSlab(NumT const *data, DimVector const &start, DimVector const &size) {
  if (data == nullptr) {
    throw NXEXCEPTION("Data specified as null");
  }
  if (start.empty()) {
    throw NXEXCEPTION("Supplied empty start");
  }
  if (size.empty()) {
    throw NXEXCEPTION("Supplied empty size");
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "Supplied start rank=" << start.size() << " must match supplied size rank=" << size.size();
    throw NXEXCEPTION(msg.str());
  }
  stringstream msg;
  msg << "putSlab(data, " << toString(start) << ", " << toString(size) << ") failed: ";

  /* check if there is a Dataset open */
  if (!isDataSetOpen()) {
    msg << "no dataset open";
    throw NXEXCEPTION(msg.str());
  }

  // copy over start, size vectors
  DimVector myStart(start.cbegin(), start.cend());
  DimVector mySize(size.cbegin(), size.cend());
  DimVector dsize(size.size(), 0);
  std::transform(start.cbegin(), start.cend(), size.cbegin(), dsize.begin(), std::plus<dimsize_t>());
  // get rank and stored dimensions
  int rank = H5Sget_simple_extent_ndims(m_current_space_id);
  if (rank < 0) {
    msg << "cannot get rank";
    throw NXEXCEPTION(msg.str());
  }
  DimVector thedims(rank, 0), maxdims(rank, 0);
  int iRet = H5Sget_simple_extent_dims(m_current_space_id, thedims.data(), maxdims.data());
  if (iRet < 0) {
    msg << "cannot get dimensions";
    throw NXEXCEPTION(msg.str());
  }

  // for string data, use 1 in the final dimension
  if (H5Tget_class(m_current_type_id) == H5T_STRING) {
    mySize.back() = 1;
    myStart.back() = 0;
    dsize.back() = 1;
  }

  // if any dimensions are unlimited,
  if (std::any_of(maxdims.cbegin(), maxdims.cend(), [](auto x) -> bool { return x == H5S_UNLIMITED; })) {
    for (int i = 0; i < rank; i++) {
      if (dsize[i] < thedims[i]) {
        dsize[i] = thedims[i];
      }
    }
    iRet = H5Dset_extent(m_current_data_id, dsize.data());
    if (iRet < 0) {
      msg << "extend slab failed";
      throw NXEXCEPTION(msg.str());
    }

    // define slab
    DataSpaceID filespace = H5Dget_space(m_current_data_id);
    iRet = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, myStart.data(), nullptr, mySize.data(), nullptr);
    if (iRet < 0) {
      msg << "selecting slab failed";
      throw NXEXCEPTION(msg.str());
    }
    // write slab
    DataSpaceID dataspace = H5Screate_simple(rank, mySize.data(), nullptr);
    iRet = H5Dwrite(m_current_data_id, m_current_type_id, dataspace, filespace, H5P_DEFAULT, data);
    if (iRet < 0) {
      msg << "writing slab failed";
      throw NXEXCEPTION(msg.str());
    }
    /* update with new size */
    iRet = H5Sclose(m_current_space_id);
    if (iRet < 0) {
      msg << "updating size failed";
      throw NXEXCEPTION(msg.str());
    }
    m_current_space_id = filespace.release();
  } else { // no unlimited dimensions
    // define slab
    iRet = H5Sselect_hyperslab(m_current_space_id, H5S_SELECT_SET, myStart.data(), nullptr, mySize.data(), nullptr);
    // deal with HDF errors
    if (iRet < 0) {
      msg << "selecting slab failed";
      throw NXEXCEPTION(msg.str());
    }
    // write slab
    DataSpaceID dataspace = H5Screate_simple(rank, mySize.data(), nullptr);
    iRet = H5Dwrite(m_current_data_id, m_current_type_id, dataspace, m_current_space_id, H5P_DEFAULT, data);
    if (iRet < 0) {
      msg << "writing slab failed";
      throw NXEXCEPTION(msg.str());
    }
  }
  // cleanup
  if (iRet < 0) {
    msg << "closing slab failed";
    throw NXEXCEPTION(msg.str());
  }
}

template <typename NumT> void File::putSlab(vector<NumT> const &data, DimVector const &start, DimVector const &size) {
  if (data.empty()) {
    throw NXEXCEPTION("Supplied empty data to putSlab");
  }
  this->putSlab(data.data(), start, size);
}

template <typename NumT> void File::putSlab(vector<NumT> const &data, dimsize_t const start, dimsize_t const size) {
  DimVector start_v{start};
  DimVector size_v{size};
  this->putSlab(data, start_v, size_v);
}

template <typename NumT> void File::getSlab(NumT *data, DimVector const &start, DimVector const &size) {
  if (data == nullptr) {
    throw NXEXCEPTION("Supplied null pointer to getSlab");
  }
  // check if there is an Dataset open
  if (!isDataSetOpen()) {
    throw NXEXCEPTION("No dataset open");
  }
  if (start.size() == 0) {
    stringstream msg;
    msg << "Supplied empty start offset, rank = " << start.size() << " in getSlab";
    throw NXEXCEPTION(msg.str());
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "In getSlab start rank=" << start.size() << " must match size rank=" << size.size();
    throw NXEXCEPTION(msg.str());
  }

  /* map datatypes of other platforms */
  DataTypeID memtype;
  H5T_class_t tclass = H5Tget_class(m_current_type_id);
  if (tclass == H5T_STRING) {
    memtype = H5Tcopy(m_current_type_id);
  } else {
    memtype = h5MemType(m_current_type_id);
  }

  herr_t iRet = -1;
  int rank = H5Sget_simple_extent_ndims(m_current_space_id);
  if (rank < 0) {
    throw NXEXCEPTION("Failed to fetch rank for slab data");
  } else if (rank == 0) { // this is an unslabbable SCALAR
    DataSpaceID memspace = H5Screate(H5S_SCALAR);
    DataSpaceID filespace = H5Dget_space(m_current_data_id);
    H5Sselect_all(filespace);
    iRet = H5Dread(m_current_data_id, memtype, memspace, filespace, H5P_DEFAULT, data);
  } else {
    DimVector myStart(start.cbegin(), start.cend());
    DimVector mySize(size.cbegin(), size.cend());
    DimVector mStart(rank, 0);
    // this does not work for multidimensional string arrays.
    NXnumtype mtype(NXnumtype::BAD);
    if (tclass == H5T_STRING) {
      mtype = NXnumtype::CHAR;
      if (mySize[0] == 1) {
        mySize[0] = H5Tget_size(m_current_type_id);
      }
    }

    iRet = H5Sselect_hyperslab(m_current_space_id, H5S_SELECT_SET, myStart.data(), nullptr, mySize.data(), nullptr);
    if (iRet < 0) {
      throw NXEXCEPTION("Selecting slab failed");
    }

    DataSpaceID memspace = H5Screate_simple(rank, mySize.data(), nullptr);
    iRet = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, mStart.data(), nullptr, mySize.data(), nullptr);
    if (iRet < 0) {
      throw NXEXCEPTION("Selecting memspace failed");
    }
    // read slab
    if (mtype == NXnumtype::CHAR) {
      std::vector<char> tmp_data(mySize[0] + 1, '\0');
      iRet = H5Dread(m_current_data_id, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, tmp_data.data());
      char const *data1;
      data1 = tmp_data.data() + myStart[0];
      strncpy(static_cast<char *>(static_cast<void *>(data)), data1, size[0]);
    } else {
      iRet = H5Dread(m_current_data_id, memtype, memspace, m_current_space_id, H5P_DEFAULT, data);
    }
  }
  /* cleanup */
  if (iRet < 0) {
    throw NXEXCEPTION("Reading slab failed");
  }
}

void File::getDataCoerce(vector<int> &data) {
  Info info = this->getInfo();
  if (info.type == NXnumtype::INT8) {
    vector<int8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::UINT8) {
    vector<uint8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::INT16) {
    vector<int16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::UINT16) {
    vector<uint16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::INT32) {
    vector<int32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::UINT32) {
    vector<uint32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else {
    throw NXEXCEPTION("NexusFile::getDataCoerce(): Could not coerce to int.");
  }
}

void File::getDataCoerce(vector<double> &data) {
  Info info = this->getInfo();
  if (info.type == NXnumtype::INT8) {
    vector<int8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::UINT8) {
    vector<uint8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::INT16) {
    vector<int16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::UINT16) {
    vector<uint16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::INT32) {
    vector<int32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::UINT32) {
    vector<uint32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::FLOAT32) {
    vector<float> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == NXnumtype::FLOAT64) {
    this->getData(data);
  } else {
    throw NXEXCEPTION("NexusFile::getDataCoerce(): Could not coerce to double.");
  }
}

//------------------------------------------------------------------------------------------------------------------
// DATA READ / WRITE
//------------------------------------------------------------------------------------------------------------------

template <typename NumT> void File::writeData(std::string const &name, NumT const &value) {
  std::vector<NumT> v(1, value);
  this->writeData(name, v);
}

void File::writeData(std::string const &name, char const *value) { this->writeData(name, std::string(value)); }

void File::writeData(std::string const &name, std::string const &value) {
  std::string my_value(value);
  // Allow empty strings by defaulting to a space
  if (my_value.empty())
    my_value = " ";
  const DimVector dims{my_value.size()};
  this->makeData(name, NXnumtype::CHAR, dims, true);

  this->putData(my_value);

  this->closeData();
}

template <typename NumT> void File::writeData(std::string const &name, vector<NumT> const &value) {
  DimVector const dims{value.size()};
  this->writeData(name, value, dims);
}

template <typename NumT>
void File::writeData(std::string const &name, vector<NumT> const &value, DimVector const &dims) {
  this->makeData(name, getType<NumT>(), dims, true);
  this->putData(value);
  this->closeData();
}

template <typename NumT> void File::writeExtendibleData(std::string const &name, vector<NumT> const &value) {
  // Use a default chunk size of 4096 bytes. TODO: Is this optimal?
  writeExtendibleData(name, value, 4096);
}

template <typename NumT>
void File::writeExtendibleData(std::string const &name, vector<NumT> const &value, dimsize_t const chunk) {
  DimVector dims{NX_UNLIMITED};
  DimVector chunk_dims{chunk};
  // Use chunking without using compression
  this->makeCompData(name, getType<NumT>(), dims, NXcompression::NONE, chunk_dims, true);
  this->putSlab(value, dimsize_t(0), dimsize_t(value.size()));
  this->closeData();
}

template <typename NumT>
void File::writeExtendibleData(std::string const &name, vector<NumT> const &value, DimVector const &dims,
                               DimVector const &chunk) {
  // Create the data with unlimited 0th dimensions
  DimVector unlim_dims(dims);
  unlim_dims[0] = NX_UNLIMITED;
  // Use chunking without using compression
  this->makeCompData(name, getType<NumT>(), unlim_dims, NXcompression::NONE, chunk, true);
  // And put that slab of that of that given size in there
  DimVector start(dims.size(), 0);
  this->putSlab(value, start, dims);
  this->closeData();
}

template <typename NumT> void File::writeUpdatedData(std::string const &name, std::vector<NumT> const &value) {
  this->openData(name);
  this->putSlab(value, dimsize_t(0), dimsize_t(value.size()));
  this->closeData();
}

template <typename NumT>
void File::writeUpdatedData(std::string const &name, std::vector<NumT> const &value, DimVector const &dims) {
  this->openData(name);
  DimVector start(dims.size(), 0);
  this->putSlab(value, start, dims);
  this->closeData();
}

template <typename NumT>
void File::writeCompData(std::string const &name, vector<NumT> const &value, DimVector const &dims,
                         NXcompression const comp, DimVector const &bufsize) {
  this->makeCompData(name, getType<NumT>(), dims, comp, bufsize, true);
  this->putData(value);
  this->closeData();
}

/*----------------------------------------------------------------------*/

template <typename NumT> void File::readData(std::string const &dataName, std::vector<NumT> &data) {
  this->openData(dataName);
  this->getData(data);
  this->closeData();
}

template <typename NumT> void File::readData(std::string const &dataName, NumT &data) {
  std::vector<NumT> dataVector(1, 0);
  this->openData(dataName);
  this->getData(dataVector);
  data = dataVector[0];
  this->closeData();
}

void File::readData(std::string const &dataName, std::string &data) {
  this->openData(dataName);
  data = this->getStrData();
  this->closeData();
}

//------------------------------------------------------------------------------------------------------------------
// ENTRY METHODS
//------------------------------------------------------------------------------------------------------------------

Info File::getInfo() {
  Info info;

  // check if there is an open dataset
  if (!isDataSetOpen()) {
    throw NXEXCEPTION("No dataset open");
  }

  // read information
  H5T_class_t tclass = H5Tget_class(m_current_type_id);
  info.type = hdf5ToNXType(tclass, m_current_type_id);
  int rank = H5Sget_simple_extent_ndims(m_current_space_id);
  if (rank < 0) {
    throw NXEXCEPTION("Cannot get rank for current dataset");
  } else if (rank == 0) {
    rank = 1; // we pretend
    info.dims = {1};
  } else {
    info.dims.resize(rank);
    H5Sget_simple_extent_dims(m_current_space_id, info.dims.data(), nullptr);
  }
  // for string data, determine size, depending on if variable length or not
  if (tclass == H5T_STRING && info.dims.back() == 1) {
    dimsize_t length;
    if (H5Tis_variable_str(m_current_type_id)) {
      // get the needed size for variable length data
      if (H5Dvlen_get_buf_size(m_current_data_id, m_current_type_id, m_current_space_id, &length) < 0) {
        throw NXEXCEPTION("Failed to read string length for variable-length string");
      }
    } else {
      // the size of string data in bytes is stored in the string datatypes
      length = H5Tget_size(m_current_type_id);
    }
    info.dims.back() = length;
  }

  // Trim 1D CHAR arrays to the actual string length
  if ((info.type == NXnumtype::CHAR) && (rank == 1)) {
    char *buf = static_cast<char *>(malloc(static_cast<size_t>((info.dims[0] + 1) * sizeof(char))));
    if (buf == nullptr) {
      throw NXEXCEPTION("Unable to allocate memory for CHAR buffer");
    }
    memset(buf, 0, static_cast<size_t>((info.dims[0] + 1) * sizeof(char)));
    this->getData<char>(buf);
    info.dims[0] = static_cast<int64_t>(strlen(buf));
    free(buf);
  }
  return info;
}

Entries File::getEntries() const {
  Entries result;
  this->getEntries(result);
  return result;
}

void File::getEntries(Entries &result) const {
  result.clear();
  auto current = getCurrentObject();
  for (size_t i = 0; i < current->getNumObjs(); i++) {
    std::string name = current->getObjnameByIdx(i);
    std::string className;
    H5G_obj_t type = current->getObjTypeByIdx(i);
    if (type == H5G_GROUP) {
      H5::Group grp = current->openGroup(name);
      if (grp.attrExists(GROUP_CLASS_SPEC)) {
        H5::Attribute attr = grp.openAttribute(GROUP_CLASS_SPEC);
        attr.read(attr.getDataType(), className);
      } else {
        className = UNKNOWN_GROUP_SPEC;
      }
    } else if (type == H5G_DATASET) {
      className = SCIENTIFIC_DATA_SET;
    }
    if (!className.empty())
      result[name] = std::move(className);
  }
}

std::string File::getTopLevelEntryName() const {
  std::string top("");
  // check all of the NXentry's for one at top-level
  auto allEntryAddresses = m_descriptor.allAddressesOfType("NXentry");
  auto iTopAddress = std::find_if(allEntryAddresses.cbegin(), allEntryAddresses.cend(),
                                  [](auto x) { return x.find_first_of('/', 1) == std::string::npos; });
  if (iTopAddress != allEntryAddresses.cend()) {
    top = *iTopAddress;
  }
  if (top.empty()) {
    throw NXEXCEPTION("unable to find top-level entry, no valid groups");
  }
  return top;
}

//------------------------------------------------------------------------------------------------------------------
// ATTRIBUTE METHODS
//------------------------------------------------------------------------------------------------------------------

// PUT / GET ATTRIBUTES

template <typename NumT> void File::putAttr(std::string const &name, NumT const &value) {
  if (name == NULL_STR) {
    throw NXEXCEPTION("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to putAttr");
  }

  std::shared_ptr<H5::H5Object> current = getCurrentObject();
  // behavior pre-existent in napi --
  // if user tries to write an attribute that already exists, delete and overwrite
  if (current->attrExists(name)) {
    current->removeAttr(name);
  }
  try {
    H5Util::writeNumAttribute<NumT>(*current, name, value);
  } catch (H5::Exception const &e) {
    throw NXEXCEPTION(e.getDetailMsg());
  }
}

void File::putAttr(const char *name, const char *value) {
  if (name == nullptr) {
    throw NXEXCEPTION("Specified name as null to putAttr");
  }
  if (value == nullptr) {
    throw NXEXCEPTION("Specified value as null to putAttr");
  }
  this->putAttr(string(name), string(value));
}

void File::putAttr(const std::string &name, const string &value, const bool empty_add_space) {
  if (name == NULL_STR) {
    throw NXEXCEPTION("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to putAttr");
  }
  string my_value(value);
  if (my_value.empty() && empty_add_space) {
    my_value = " "; // Make a default "space" to avoid errors.
  }

  std::shared_ptr<H5::H5Object> current = getCurrentObject();
  // behavior pre-existent in napi --
  // if user tries to write an attribute that already exists, delete and overwrite
  if (current->attrExists(name)) {
    current->removeAttr(name);
  }
  try {
    H5Util::writeStrAttribute(*current, name, my_value);
  } catch (H5::Exception const &e) {
    throw NXEXCEPTION(e.getDetailMsg());
  }
}

template <typename NumT> NumT File::getAttr(std::string const &name) {
  NumT value;
  this->getAttr<NumT>(name, value);
  return value;
}

template <> MANTID_NEXUS_DLL void File::getAttr(const std::string &name, std::string &value) {
  value = this->getStrAttr(name);
}

template <typename NumT> void File::getAttr(const std::string &name, NumT &value) {
  auto current = getCurrentObject();
  try {
    value = H5Util::readNumAttributeCoerce<NumT>(*current, name);
  } catch (H5::Exception const &e) {
    throw NXEXCEPTION(e.getDetailMsg());
  }
}

string File::getStrAttr(std::string const &name) {
  std::string res("");
  auto current = getCurrentObject();
  try {
    H5Util::readStringAttribute(*current, name, res);
  } catch (H5::Exception const &e) {
    throw NXEXCEPTION(e.getDetailMsg());
  }
  return res;
}

// NAVIGATE ATTRIBUTES

std::vector<AttrInfo> File::getAttrInfos() {
  hid_t current = getCurrentId();
  // get the number of attributes
  H5O_info2_t oinfo; /* Object info */
  H5Oget_info3(current, &oinfo, H5O_INFO_NUM_ATTRS);
  std::size_t num_attr = oinfo.num_attrs;
  std::vector<AttrInfo> infos;
  infos.reserve(num_attr);
  for (std::size_t idx = 0; idx < num_attr; idx++) {
    // open the attribute -- see link below for example, implemented in H5::H5Object:getNumAttrs()
    // https://github.com/HDFGroup/hdf5/blob/51dd7758fe5d79ec61e457ff30c697ceccb32e90/c%2B%2B/src/H5Object.cpp#L192
    AttributeID attr = H5Aopen_by_idx(current, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, idx, H5P_DEFAULT, H5P_DEFAULT);
    // 1. get the attribute name
    std::size_t namelen = H5Aget_name(attr, 0, nullptr);
    char *cname = new char[namelen + 1];
    H5Aget_name(attr, namelen + 1, cname);
    cname[namelen] = '\0'; // ensure null termination
    // do not include the group class spec
    if (GROUP_CLASS_SPEC == cname) {
      continue;
    }
    // 2. get the attribute type
    DataTypeID attrtype = H5Aget_type(attr);
    H5T_class_t attrclass = H5Tget_class(attrtype);
    NXnumtype type = hdf5ToNXType(attrclass, attrtype);
    // 3. get the attribute length
    DataSpaceID attrspace = H5Aget_space(attr);
    int rank = H5Sget_simple_extent_ndims(attrspace);
    if (rank > 2 || (rank == 2 && type != NXnumtype::CHAR)) {
      throw NXEXCEPTION("ERROR iterating through attributes found array attribute not understood by this api");
    }
    DimVector dims(rank, 0);
    H5Sget_simple_extent_dims(attrspace, dims.data(), nullptr);
    std::size_t length = 1;
    if (type == NXnumtype::CHAR) {
      length = getStrAttr(cname).size();
    }
    for (int d = 0; d < rank; d++) {
      length *= dims[d];
    }
    // now add info to the vector
    infos.emplace_back(type, length, cname);
    delete[] cname;
  }
  return infos;
}

bool File::hasAttr(const std::string &name) const {
  hid_t current = getCurrentId();
  return H5Aexists(current, name.c_str()) > 0;
}

//------------------------------------------------------------------------------------------------------------------
// LINK METHODS
//------------------------------------------------------------------------------------------------------------------

NXlink File::getGroupID() {
  NXlink link;
  if (m_current_group_id == 0) {
    throw NXEXCEPTION("getGroupID failed, No current group open");
  }

  try {
    getAttr(TARGET_ATTR_NAME, link.targetAddress);
  } catch (const Mantid::Nexus::Exception &) {
    link.targetAddress = getAddress();
  }
  link.linkType = NXentrytype::group;
  return link;
}

NXlink File::getDataID() {
  NXlink link;
  /* we cannot return ID's when no datset is open */
  if (!isDataSetOpen()) {
    throw NXEXCEPTION("getDataID failed, No current dataset open");
  }

  try {
    getAttr(TARGET_ATTR_NAME, link.targetAddress);
  } catch (const Mantid::Nexus::Exception &) {
    link.targetAddress = getAddress();
  }

  link.linkType = NXentrytype::sds;
  return link;
}

void File::makeLink(NXlink const &link) {
  if (m_current_group_id == 0) { /* root level, can not link here */
    throw NXEXCEPTION("makeLink failed : cannot form link at root level");
  }

  // locate name of the element to link
  NexusAddress target(link.targetAddress);
  std::string itemName(target.stem());

  // build addressname to link from our current group and the name of the thing to link
  std::string linkTarget(groupAddress(m_address) / itemName);
  H5Lcreate_hard(m_pfile->getId(), link.targetAddress.c_str(), H5L_SAME_LOC, linkTarget.c_str(), H5P_DEFAULT,
                 H5P_DEFAULT);

  // register the entry
  registerEntry(linkTarget, m_descriptor.classTypeForName(link.targetAddress));

  // set the target attribute
  NexusAddress here(m_address);
  openAddress(linkTarget);
  putAttr(TARGET_ATTR_NAME, link.targetAddress);
  openAddress(here);
}

} // namespace Mantid::Nexus

// -------------------------- NXnumtype ----------------------------------------------------------------------------//
using namespace Mantid::Nexus;
int NXnumtype::validate_val(int const x) {
  int val = BAD;
  if ((x == INT8) || (x == UINT8) || (x == INT16) || (x == UINT16) || (x == INT32) || (x == UINT32) || (x == INT64) ||
      (x == UINT64) || (x == FLOAT32) || (x == FLOAT64) || (x == CHAR) || (x == BINARY) || (x == BAD)) {
    val = x;
  }
  return val;
}
NXnumtype::NXnumtype() : m_val(BAD) {};
NXnumtype::NXnumtype(int const val) : m_val(validate_val(val)) {};

NXnumtype &NXnumtype::operator=(int const val) {
  this->m_val = validate_val(val);
  return *this;
};

NXnumtype::operator int() const { return m_val; };

#define NXTYPE_PRINT(var) #var // stringify the variable name, for cleaner code

NXnumtype::operator std::string() const {
  // isolate each hexadigit
  unsigned short type = static_cast<unsigned short>(m_val & 0xF0);
  unsigned short size = static_cast<unsigned short>(m_val & 0x0F);
  std::string sizestr = std::to_string(size * 8u); // width in bits
  std::string ret = NXTYPE_PRINT(BAD);
  if (type == 0x00u) {
    ret = "UINT" + sizestr;
  } else if (type == 0x10u) {
    ret = "INT" + sizestr;
  } else if (type == 0x20u) {
    ret = "FLOAT" + sizestr;
  } else if (type == 0xF0u) {
    // special types
    if (m_val == CHAR) {
      ret = NXTYPE_PRINT(CHAR);
    } else if (m_val == BINARY) {
      ret = NXTYPE_PRINT(BINARY);
    }
  }
  return ret;
}

std::ostream &operator<<(std::ostream &os, const NXnumtype &value) {
  os << std::string(value);
  return os;
}

std::ostream &operator<<(std::ostream &os, const NXaccess &value) {
  std::string ret("Unknown access type");
  if (value == NXaccess::CREATE5) {
    ret = NXTYPE_PRINT(NXaccess::CREATE5);
  } else if (value == NXaccess::READ) {
    ret = NXTYPE_PRINT(NXaccess::READ);
  } else if (value == NXaccess::RDWR) {
    ret = NXTYPE_PRINT(NXaccess::RDWR);
  }
  os << ret;
  return os;
}

std::ostream &operator<<(std::ostream &os, const NXcompression &value) {
  std::string ret("Unknown compression type");
  if (value == NXcompression::NONE) {
    ret = NXTYPE_PRINT(NXcompression::NONE);
  } else if (value == NXcompression::CHUNK) {
    ret = NXTYPE_PRINT(NXcompression::CHUNK);
  } else if (value == NXcompression::LZW) {
    ret = NXTYPE_PRINT(NXcompression::LZW);
  } else if (value == NXcompression::RLE) {
    ret = NXTYPE_PRINT(NXcompression::RLE);
  } else if (value == NXcompression::HUF) {
    ret = NXTYPE_PRINT(NXcompression::HUF);
  }
  os << ret;
  return os;
}

/* ---------------------------------------------------------------- */
/* Concrete instantiations of template definitions.                 */
/* ---------------------------------------------------------------- */

// PUT / GET ATTR

template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, float const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, double const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, int8_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, uint8_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, int16_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, uint16_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, int32_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, uint32_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, int64_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, uint64_t const &value);
template MANTID_NEXUS_DLL void File::putAttr(std::string const &name, char const &value);

template MANTID_NEXUS_DLL int16_t File::getAttr(std::string const &name);
template MANTID_NEXUS_DLL uint16_t File::getAttr(std::string const &name);
template MANTID_NEXUS_DLL int32_t File::getAttr(std::string const &name);
template MANTID_NEXUS_DLL std::string File::getAttr(std::string const &name);

template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int8_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint8_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int16_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint16_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int32_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint32_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int64_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint64_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, float &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, double &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, char &value);

// PUT / GET DATA

template MANTID_NEXUS_DLL void File::putData(const vector<int8_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<uint8_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<int16_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<uint16_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<int32_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<uint32_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<int64_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<uint64_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<float> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<double> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<char> &data);

template MANTID_NEXUS_DLL void File::putData(const float *data);
template MANTID_NEXUS_DLL void File::putData(const double *data);
template MANTID_NEXUS_DLL void File::putData(const int8_t *data);
template MANTID_NEXUS_DLL void File::putData(const uint8_t *data);
template MANTID_NEXUS_DLL void File::putData(const int16_t *data);
template MANTID_NEXUS_DLL void File::putData(const uint16_t *data);
template MANTID_NEXUS_DLL void File::putData(const int32_t *data);
template MANTID_NEXUS_DLL void File::putData(const uint32_t *data);
template MANTID_NEXUS_DLL void File::putData(const int64_t *data);
template MANTID_NEXUS_DLL void File::putData(const uint64_t *data);
template MANTID_NEXUS_DLL void File::putData(const char *data);
template MANTID_NEXUS_DLL void File::putData(const bool *data);

template MANTID_NEXUS_DLL void File::getData(float *data);
template MANTID_NEXUS_DLL void File::getData(double *data);
template MANTID_NEXUS_DLL void File::getData(int8_t *data);
template MANTID_NEXUS_DLL void File::getData(uint8_t *data);
template MANTID_NEXUS_DLL void File::getData(int16_t *data);
template MANTID_NEXUS_DLL void File::getData(uint16_t *data);
template MANTID_NEXUS_DLL void File::getData(int32_t *data);
template MANTID_NEXUS_DLL void File::getData(uint32_t *data);
template MANTID_NEXUS_DLL void File::getData(int64_t *data);
template MANTID_NEXUS_DLL void File::getData(uint64_t *data);
#ifdef _WIN32
template MANTID_NEXUS_DLL void File::getData<char>(char *data);
#endif
template MANTID_NEXUS_DLL void File::getData(bool *data);

template MANTID_NEXUS_DLL void File::getData(vector<float> &data);
template MANTID_NEXUS_DLL void File::getData(vector<double> &data);
template MANTID_NEXUS_DLL void File::getData(vector<int8_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<uint8_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<int16_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<uint16_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<int32_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<uint32_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<int64_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<uint64_t> &data);
template MANTID_NEXUS_DLL void File::getData(vector<char> &data);

// READ / WRITE DATA -- BASIC

template MANTID_NEXUS_DLL void File::writeData(std::string const &name, float const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, double const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, int8_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, uint8_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, int16_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, uint16_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, int32_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, uint32_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, int64_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, uint64_t const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, char const &value);

template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<float> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<double> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int8_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint8_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int16_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint16_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int32_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint32_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int64_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint64_t> const &value);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<char> const &value);

template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<float> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<double> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int8_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint8_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int16_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint16_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int32_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint32_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int64_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint64_t> const &value,
                                               DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<char> const &value,
                                               DimVector const &dims);

template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<float> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<double> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int8_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint8_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int16_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint16_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int32_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint32_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int64_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint64_t> &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<char> &data);

template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, float &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, double &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int8_t &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint8_t &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int16_t &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint16_t &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int32_t &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint32_t &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int64_t &data);
template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint64_t &data);

// READ / WRITE DATA -- EXTENDIBLE

template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<float> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<double> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int8_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint8_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int16_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint16_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int32_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint32_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int64_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint64_t> const &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<char> const &value);

template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<float> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<double> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int8_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint8_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int16_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint16_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int32_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint32_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int64_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint64_t> const &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<char> const &value,
                                                         dimsize_t const chunk);

template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<float> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<double> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int8_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint8_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int16_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint16_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int32_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint32_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int64_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint64_t> const &value,
                                                         DimVector const &dims, DimVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<char> const &value,
                                                         DimVector const &dims, DimVector const &chunk);

template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<float> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<double> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int8_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint8_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int16_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint16_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int32_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint32_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int64_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint64_t> const &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<char> const &value);

template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<float> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<double> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int8_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint8_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int16_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint16_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int32_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint32_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int64_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint64_t> const &value,
                                                      DimVector const &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<char> const &value,
                                                      DimVector const &dims);

template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<float> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<double> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int8_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint8_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int16_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint16_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int32_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint32_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int64_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint64_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimVector const &bufsize);

// READ / WRITE DATA -- SLAB / EXTENDIBLE

template MANTID_NEXUS_DLL void File::getSlab(float *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(double *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int8_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint8_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int16_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint16_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int32_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint32_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int64_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint64_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(char *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::getSlab(bool *data, const DimVector &start, const DimVector &size);

template MANTID_NEXUS_DLL void File::putSlab(const float *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const double *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int8_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint8_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int16_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint16_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int32_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint32_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int64_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint64_t *data, const DimVector &start, const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const char *data, const DimVector &start, const DimVector &size);

template MANTID_NEXUS_DLL void File::putSlab(const std::vector<float> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<double> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int8_t> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint8_t> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int16_t> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint16_t> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int32_t> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint32_t> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int64_t> &data, const DimVector &start,
                                             const DimVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint64_t> &data, const DimVector &start,
                                             const DimVector &size);

template MANTID_NEXUS_DLL void File::putSlab(const std::vector<float> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<double> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int8_t> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint8_t> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int16_t> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint16_t> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int32_t> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint32_t> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int64_t> &data, const dimsize_t start,
                                             const dimsize_t size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint64_t> &data, const dimsize_t start,
                                             const dimsize_t size);
