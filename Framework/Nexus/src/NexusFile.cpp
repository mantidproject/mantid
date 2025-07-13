#include <cstring>
// REMOVE
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NexusException.h"
#include "MantidNexus/NexusFile.h"
#include "MantidNexus/napi.h"
#include "MantidNexus/napi_helper.h"
#include <H5Cpp.h>
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

#ifdef WIN32
#define strdup _strdup
#endif

#define NXEXCEPTION(message) Exception((message), __func__, m_filename);
#define NX_UNKNOWN_GROUP ""
#define NUL '\0'

#define NAPI_CALL(status, msg)                                                                                         \
  NXstatus tmp = (status);                                                                                             \
  if (tmp != NXstatus::NX_OK) {                                                                                        \
    throw NXEXCEPTION(msg);                                                                                            \
  }

/**
 * \file NexusFile.cpp
 * The implementation of the NeXus C++ API
 */

namespace { // anonymous namespace to keep it in the file

std::string const group_class_spec("NX_class");
std::string const target_attr_name("target");
std::string const scientific_data_set("SDS");
std::string const unknown_group_spec("NX_UNKNOWN_GROUP");
constexpr int default_deflate_level(6);

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

pNexusFile5 assertNXID(std::shared_ptr<NexusFile5> const &pfid) { return pfid.get(); }

} // end of anonymous namespace

NexusFile5::NexusFile5(std::string const &filename, NXaccess const am)
    : iStack5{0}, iFID(0), iCurrentG(0), iCurrentD(0), iCurrentS(0), iCurrentT(0), groupaddr() {
  // check HDF5 version installed
  unsigned int vers_major, vers_minor, vers_release;
  if (H5get_libversion(&vers_major, &vers_minor, &vers_release) < 0) {
    throw Mantid::Nexus::Exception("Cannot determine HDF5 library version", "NexusFile5 constructor", filename);
  }
  if (vers_major == 1 && vers_minor < 8) {
    throw Mantid::Nexus::Exception("HDF5 library 1.8.0 or higher required", "NexusFile5 constructor", filename);
  }
  std::string version_str =
      std::to_string(vers_major) + "." + std::to_string(vers_minor) + "." + std::to_string(vers_release);
  // turn off the automatic HDF error handling
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);

  // create file acccess property list
  hid_t fapl = -1;
  fapl = H5Pcopy(Mantid::NeXus::H5Util::defaultFileAcc().getId());

  if (am != NXaccess::CREATE5) {
    if (H5Fis_accessible(filename.c_str(), fapl) <= 0) {
      throw Mantid::Nexus::Exception("File is not HDF5", "NexusFile5 constructor", filename);
    }
    iFID = H5Fopen(filename.c_str(), (unsigned)am, fapl);
  } else {
    iFID = H5Fcreate(filename.c_str(), (unsigned)am, H5P_DEFAULT, fapl);
  }

  if (fapl != -1) {
    H5Pclose(fapl);
  }

  if (iFID <= 0) {
    throw Mantid::Nexus::Exception("Cannot open file", "NexusFile5 constructor", filename);
  }

  // if in creation mode, must add the following attributes
  // - file_namen
  // - file_time
  // - Nexus version
  // - HDF5 version
  if (am == NXaccess::CREATE5) {
    // open the root as a group and add these attributes
    hid_t root_id = H5Gopen(iFID, "/", H5P_DEFAULT);

    std::vector<std::pair<std::string, std::string>> attrs{{"NeXus_version", NEXUS_VERSION},
                                                           {"file_name", filename},
                                                           {"HDF5_Version", version_str},
                                                           {"file_time", NXIformatNeXusTime()},
                                                           {group_class_spec, "NXroot"}};
    for (auto const &attr : attrs) {
      // cppcheck-suppress useStlAlgorithm
      if (set_str_attribute(root_id, attr.first, attr.second) < 0) {
        H5Gclose(root_id);
        H5Fclose(iFID);
        throw Mantid::Nexus::Exception("Error formatting file for NeXus", "NexusFile5 constructor", filename);
      }
    }
    H5Gflush(root_id);
    H5Gclose(root_id);
  }
  H5Fflush(iFID, H5F_SCOPE_GLOBAL);

  iStack5[0] = 0; // root!
};

NexusFile5::NexusFile5(NexusFile5 const &origHandle)
    : iStack5{0}, iFID(0), iCurrentG(0), iCurrentD(0), iCurrentS(0), iCurrentT(0), groupaddr() {
  iFID = H5Freopen(origHandle.iFID);
  if (iFID <= 0) {
    throw Mantid::Nexus::Exception("Error reopening file");
  }
  iStack5[0] = 0; // root!
};

NexusFile5 &NexusFile5::operator=(NexusFile5 const &origHandle) {
  this->iStack5 = {0};
  this->iFID = H5Freopen(origHandle.iFID);
  this->iCurrentG = this->iCurrentD = this->iCurrentS = this->iCurrentT = 0;
  this->groupaddr = Mantid::Nexus::NexusAddress::root();
  return *this;
}

NexusFile5::~NexusFile5() {
  if (iCurrentD != 0) {
    H5Dclose(iCurrentD);
  }
  for (hid_t gid : iStack5) {
    H5Gclose(gid);
  }
  H5Fclose(iFID);
  H5garbage_collect();
}

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

} // namespace Mantid::Nexus

namespace Mantid::Nexus {

//------------------------------------------------------------------------------------------------------------------
// CONSTRUCTORS / ASSIGNMENT / DECONSTRUCTOR
//------------------------------------------------------------------------------------------------------------------

// new constructors

File::File(const string &filename, const NXaccess access)
    : m_filename(filename), m_access(access), m_address(), m_close_handle(true), m_descriptor(m_filename, m_access) {
  this->initOpenFile(m_filename, m_access);
}

File::File(const char *filename, const NXaccess access)
    : m_filename(filename), m_access(access), m_address(), m_close_handle(true), m_descriptor(m_filename, m_access) {
  this->initOpenFile(m_filename, m_access);
}

void File::initOpenFile(const string &filename, const NXaccess access) {
  if (filename.empty()) {
    throw NXEXCEPTION("Filename specified is empty constructor");
  }
  NexusFile5 tmp(filename, access);
  if (tmp.iFID <= 0) {
    stringstream msg;
    msg << "File::initOpenFile(" << filename << ", " << access << ") failed";
    throw NXEXCEPTION(msg.str());
  } else {
    m_pfile_id = std::make_shared<NexusFile5>(tmp);
  }
}

// copy constructors

File::File(File const &f)
    : m_filename(f.m_filename), m_access(f.m_access), m_address(f.m_address), m_close_handle(false),
      m_pfile_id(f.m_pfile_id), m_descriptor(f.m_descriptor) {}

File::File(File const *const pf)
    : m_filename(pf->m_filename), m_access(pf->m_access), m_address(pf->m_address), m_close_handle(false),
      m_pfile_id(pf->m_pfile_id), m_descriptor(pf->m_descriptor) {}

File::File(std::shared_ptr<File> pf)
    : m_filename(pf->m_filename), m_access(pf->m_access), m_address(pf->m_address), m_close_handle(false),
      m_pfile_id(pf->m_pfile_id), m_descriptor(pf->m_descriptor) {}

File &File::operator=(File const &f) {
  if (this == &f) {
  } else {
    this->m_filename = f.m_filename;
    this->m_access = f.m_access;
    this->m_address = f.m_address;
    this->m_pfile_id = f.m_pfile_id;
    this->m_close_handle = f.m_close_handle;
    this->m_descriptor = f.m_descriptor;
  }
  return *this;
}

// deconstructor

File::~File() {
  if (m_close_handle) {
    close();
  }
  m_pfile_id.reset();
}

void File::close() {
  if (m_pfile_id != nullptr) {
    /* release memory */
    for (hid_t gid : m_pfile_id->iStack5) {
      H5Gclose(gid);
    }
    H5Fclose(m_pfile_id->iFID);
    H5garbage_collect();
    m_pfile_id = nullptr;
  }
  m_pfile_id.reset();
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
  NexusAddress absaddr(formAbsoluteAddress(address));
  if (address.empty()) {
    throw NXEXCEPTION("Supplied empty address");
  }
  NXstatus ret = NXopenaddress(m_pfile_id.get(), absaddr);
  m_address = getObjectAddress(getCurrentId());
  NAPI_CALL(ret, "NXopenaddress(" + address + ") failed");
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

std::string File::getAddress() const { return m_address; }

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
  return m_descriptor.isEntry(address, scientific_data_set);
}

bool File::isDataSetOpen() const {
  if (m_pfile_id->iCurrentD == 0) {
    return false;
  } else {
    return H5Iget_type(m_pfile_id->iCurrentD) == H5I_DATASET;
  }
}

bool File::isDataInt() const {
  if (m_pfile_id->iCurrentD == 0) {
    throw NXEXCEPTION("No dataset is open");
  }
  hid_t datatype = H5Dget_type(m_pfile_id->iCurrentD);
  H5T_class_t dataclass = H5Tget_class(datatype);
  H5Tclose(datatype);
  return dataclass == H5T_INTEGER;
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
  if (m_pfile_id->iCurrentD != 0) {
    return m_pfile_id->iCurrentD;
  } else if (m_pfile_id->iCurrentG != 0) {
    return m_pfile_id->iCurrentG;
  } else {
    return m_pfile_id->iFID;
  }
}

std::shared_ptr<H5::H5Object> File::getCurrentObject() const {
  if (m_pfile_id->iCurrentD != 0) {
    return std::make_shared<H5::DataSet>(m_pfile_id->iCurrentD);
  } else if (m_pfile_id->iCurrentG != 0) {
    return std::make_shared<H5::Group>(m_pfile_id->iCurrentG);
  } else {
    return std::make_shared<H5::H5File>(m_pfile_id->iFID);
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
  H5::H5File h5file(m_pfile_id->iFID);
  Mantid::NeXus::H5Util::createGroupNXS(h5file, absaddr, nxclass);

  // cleanup
  registerEntry(absaddr, nxclass);
  if (open_group) {
    this->openGroup(name, nxclass);
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
    iVID = H5Gopen(m_pfile_id->iFID, absaddr.c_str(), H5P_DEFAULT);
  } else {
    throw NXEXCEPTION("The supplied group " + absaddr + " does not exist");
  }

  if (iVID < 0) {
    std::stringstream msg;
    msg << "Group " << absaddr.string() << " does not exist";
    throw NXEXCEPTION(msg.str());
  }

  /* maintain stack */
  m_pfile_id->iCurrentG = iVID;
  m_pfile_id->iStack5.push_back(iVID);
  m_pfile_id->groupaddr = absaddr;
  m_address = absaddr;
  // if we are opening a new group, close whatever dataset is already open
  if (m_pfile_id->iCurrentD != 0) {
    closeData();
  }
}

void File::closeGroup() {
  pNexusFile5 pFile = assertNXID(m_pfile_id);
  if (pFile->iCurrentG == 0) {
    // do nothing
  } else {
    // if a group is closed while a dataset is still open,
    // make sure the dataset and all its parts are closed
    if (pFile->iCurrentD != 0) {
      closeData();
    }
    // close the current group and maintain stack
    if (H5Gclose(pFile->iCurrentG) < 0) {
      throw NXEXCEPTION("Failed to close group at " + pFile->groupaddr);
    }
    pFile->groupaddr = pFile->groupaddr.parent_path();
    pFile->iStack5.pop_back();
    if (!pFile->iStack5.empty()) {
      pFile->iCurrentG = pFile->iStack5.back();
    } else {
      pFile->iCurrentG = 0;
    }
  }
  m_address = m_address.parent_path();
}

//------------------------------------------------------------------------------------------------------------------
// DATA MAKE / OPEN / PUT / GET / CLOSE
//------------------------------------------------------------------------------------------------------------------

void File::makeData(const string &name, NXnumtype const type, DimVector const &dims, bool const open_data) {
  // error check the parameters
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty label");
  }
  if (dims.empty()) {
    throw NXEXCEPTION("Supplied empty dimensions");
  }
  DimSizeVector chunk_size(dims.size());
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

  pNexusFile5 pFile = assertNXID(m_pfile_id);
  // close any open dataset
  if (pFile->iCurrentD != 0) {
    closeData();
  }

  NexusAddress absaddr(formAbsoluteAddress(name));

  /* find the ID number and open the dataset */
  hid_t newData, newType, newSpace;
  newData = H5Dopen(pFile->iFID, absaddr.c_str(), H5P_DEFAULT);
  if (newData < 0) {
    pFile->iCurrentD = 0;
    pFile->iCurrentT = 0;
    pFile->iCurrentS = 0;
    throw NXEXCEPTION("dataset (" + absaddr + ") not found at this level");
  }
  /* find the ID number of datatype */
  newType = H5Dget_type(newData);
  if (pFile->iCurrentT < 0) {
    H5Dclose(newData);
    pFile->iCurrentD = 0;
    pFile->iCurrentT = 0;
    pFile->iCurrentS = 0;
    throw NXEXCEPTION("error opening dataset (" + absaddr + ")");
  }
  /* find the ID number of dataspace */
  newSpace = H5Dget_space(newData);
  if (pFile->iCurrentS < 0) {
    H5Dclose(newData);
    H5Tclose(newType);
    pFile->iCurrentD = 0;
    pFile->iCurrentT = 0;
    pFile->iCurrentS = 0;
    throw NXEXCEPTION("HDF error opening dataset (" + absaddr + ")");
  }
  // now maintain stack
  pFile->iCurrentD = newData;
  pFile->iCurrentT = newType;
  pFile->iCurrentS = newSpace;
  m_address = absaddr;
}

template <typename NumT> void File::putData(NumT const *data) {
  if (data == NULL) {
    throw NXEXCEPTION("Data specified as null");
  }
  pNexusFile5 pFile;
  herr_t iRet;
  std::array<hsize_t, H5S_MAX_RANK> thedims{0}, maxdims{0};
  int rank;

  pFile = assertNXID(m_pfile_id);
  rank = H5Sget_simple_extent_ndims(pFile->iCurrentS);
  if (rank < 0) {
    throw NXEXCEPTION("Cannot determine dataset rank");
  }
  iRet = H5Sget_simple_extent_dims(pFile->iCurrentS, thedims.data(), maxdims.data());
  if (iRet < 0) {
    throw NXEXCEPTION("Cannot determine dataset dimensions");
  }
  bool unlimiteddim = std::any_of(maxdims.cbegin(), maxdims.cend(), [](auto x) -> bool { return x == H5S_UNLIMITED; });
  /* If we are using putdata on an unlimied dimension dataset, assume we want to append one single new slab */
  if (unlimiteddim) {
    std::array<int64_t, H5S_MAX_RANK> myStart{0}, mySize{0};
    for (std::size_t i = 0; i < myStart.size(); i++) {
      if (maxdims[i] == H5S_UNLIMITED) {
        myStart[i] = static_cast<int64_t>(thedims[i] + 1);
        mySize[i] = 1;
      } else {
        myStart[i] = 0;
        mySize[i] = static_cast<int64_t>(thedims[i]);
      }
    }
    DimSizeVector vecStart(myStart.begin(), myStart.end());
    DimSizeVector vecSize(mySize.begin(), mySize.end());

    return putSlab(data, vecStart, vecSize);
  } else {
    iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (iRet < 0) {
      throw NXEXCEPTION("failure to write data");
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

template <> void File::getData<char>(char *data) {
  pNexusFile5 pFile = assertNXID(m_pfile_id);

  if (pFile->iCurrentD == 0) {
    throw NXEXCEPTION("getData ERROR: no dataset open");
  }

  hsize_t dims[H5S_MAX_RANK] = {0};
  int rank = H5Sget_simple_extent_dims(pFile->iCurrentS, dims, nullptr);
  herr_t ret = -1;
  std::size_t size = 0;
  std::vector<char> buffer;

  if (H5Tis_variable_str(pFile->iCurrentT)) {
    char *cdata = nullptr;
    ret = H5Dread(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &cdata);
    if (ret < 0 || cdata == nullptr)
      throw NXEXCEPTION("getData ERROR: failed to read variable length string dataset");

    size = strlen(cdata);
    buffer.assign(cdata, cdata + size + 1);
    H5free_memory(cdata);
  } else {
    hsize_t len = H5Tget_size(pFile->iCurrentT);
    for (int i = 0; i < rank - 1; i++) {
      len *= (dims[i] > 1 ? dims[i] : 1);
    }

    buffer.resize(len + 1, '\0');
    ret = H5Dread(pFile->iCurrentD, pFile->iCurrentT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
    if (ret < 0)
      throw NXEXCEPTION("getData ERROR: failed to read string\n");

    buffer[len] = '\0';
    size = len;
  }

  if (rank == 0 || rank == 1) {
    char *start = buffer.data();
    while (*start && isspace(*start))
      ++start;

    int i = (int)strlen(start);
    while (--i >= 0) {
      if (!isspace(start[i])) {
        break;
      }
    }
    start[++i] = '\0';

    std::memcpy(data, start, i);
    data[i] = '\0';
  } else {
    std::memcpy(data, buffer.data(), size);
  }
}

template <typename NumT> void File::getData(NumT *data) {
  if (!data) {
    throw NXEXCEPTION("Supplied null pointer to write data to");
  }

  pNexusFile5 pFile = assertNXID(m_pfile_id);
  if (pFile->iCurrentD == 0) {
    throw NXEXCEPTION("getData ERROR: no dataset open");
  }

  herr_t ret = -1;
  hsize_t dims[H5S_MAX_RANK];
  H5Sget_simple_extent_dims(pFile->iCurrentS, dims, nullptr);
  hsize_t ndims = H5Sget_simple_extent_ndims(pFile->iCurrentS);

  if (ndims == 0) {
    hid_t datatype = H5Dget_type(pFile->iCurrentD);
    hid_t filespace = H5Dget_space(pFile->iCurrentD);
    hid_t memtype_id = H5Screate(H5S_SCALAR);
    H5Sselect_all(filespace);
    ret = H5Dread(pFile->iCurrentD, datatype, memtype_id, filespace, H5P_DEFAULT, data);
    H5Sclose(memtype_id);
    H5Sclose(filespace);
    H5Tclose(datatype);
  } else {
    if (H5Tget_class(pFile->iCurrentT) == H5T_STRING) {
      this->getData<char>(reinterpret_cast<char *>(data));
      return;
    }
    hid_t memtype_id = h5MemType(pFile->iCurrentT);
    ret = H5Dread(pFile->iCurrentD, memtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  }

  if (ret < 0) {
    throw NXEXCEPTION("getData ERROR: failed to transfer dataset");
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

void File::closeData() {
  herr_t iRet = 0;
  if (m_pfile_id->iCurrentS != 0) {
    iRet = H5Sclose(m_pfile_id->iCurrentS);
    if (iRet < 0) {
      throw NXEXCEPTION("Cannot end access to dataset: failed to close dataspace");
    }
  }
  if (m_pfile_id->iCurrentT != 0) {
    iRet = H5Tclose(m_pfile_id->iCurrentT);
    if (iRet < 0) {
      throw NXEXCEPTION("Cannot end access to dataset: failed to close datatype");
    }
  }
  if (m_pfile_id->iCurrentD != 0) {
    iRet = H5Dclose(m_pfile_id->iCurrentD);
    if (iRet < 0) {
      throw NXEXCEPTION("Cannot end access to dataset: failed to close dataset");
    }
  } else {
    throw NXEXCEPTION("Cannot end access to dataset: no data open");
  }
  m_pfile_id->iCurrentD = 0;
  m_pfile_id->iCurrentS = 0;
  m_pfile_id->iCurrentT = 0;
  m_address = m_address.parent_path();
}

//------------------------------------------------------------------------------------------------------------------
// DATA MAKE COMP / PUT/GET SLAB / COERCE
//------------------------------------------------------------------------------------------------------------------

void File::makeCompData(std::string const &name, NXnumtype const type, DimVector const &dims, NXcompression comp,
                        DimSizeVector const &chunk, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to makeCompData");
  }
  if (dims.empty()) {
    throw NXEXCEPTION("Supplied empty dimensions to makeCompData");
  }
  if (chunk.empty()) {
    throw NXEXCEPTION("Supplied empty bufsize to makeCompData");
  }
  if (dims.size() != chunk.size()) {
    stringstream msg;
    msg << "Supplied dims rank=" << dims.size() << " must match supplied bufsize rank=" << chunk.size()
        << "in makeCompData";
    throw NXEXCEPTION(msg.str());
  }

  // do the work

  hid_t datatype1, dataspace, iNew;
  hid_t dtype, cparms = -1;
  pNexusFile5 pFile;
  size_t byte_zahl = 0;
  hsize_t chunkdims[H5S_MAX_RANK];
  hsize_t mydim[H5S_MAX_RANK], mydim1[H5S_MAX_RANK];
  hsize_t dsize[H5S_MAX_RANK];
  hsize_t maxdims[H5S_MAX_RANK];
  bool unlimiteddim = false;
  int rank = static_cast<int>(dims.size());
  stringstream msg;
  msg << "compMakeData(" << name << ", " << type << ", " << dims.size() << ", " << toString(dims) << ", " << comp
      << ", " << toString(chunk) << ") failed: ";

  pFile = assertNXID(m_pfile_id);
  if (pFile->iCurrentG <= 0) {
    msg << "No group open for makedata on " << name;
    throw NXEXCEPTION(msg.str());
  }

  if (rank <= 0) {
    msg << "Invalid rank specified " << name;
    throw NXEXCEPTION(msg.str());
  }

  dtype = nxToHDF5Type(type);

  /*
     Check dimensions for consistency. Dimension may be -1
     thus denoting an unlimited dimension.
   */
  for (int i = 0; i < rank; i++) {
    chunkdims[i] = chunk[i];
    mydim[i] = dims[i];
    maxdims[i] = dims[i];
    dsize[i] = dims[i];
    if (dims[i] <= 0) {
      mydim[i] = 1;
      maxdims[i] = H5S_UNLIMITED;
      dsize[i] = 1;
      unlimiteddim = true;
    } else {
      mydim[i] = dims[i];
      maxdims[i] = dims[i];
      dsize[i] = dims[i];
    }
  }

  if (type == NXnumtype::CHAR) {
    /*
     *  This assumes string lenght is in the last dimensions and
     *  the logic must be the same as used in NX5getslab and NX5getinfo
     *
     *  search for tests on H5T_STRING
     */
    byte_zahl = (size_t)mydim[rank - 1];
    for (int i = 0; i < rank; i++) {
      mydim1[i] = mydim[i];
      if (dims[i] <= 0) {
        mydim1[0] = 1;
        maxdims[0] = H5S_UNLIMITED;
      }
    }
    mydim1[rank - 1] = 1;
    if (mydim[rank - 1] > 1) {
      maxdims[rank - 1] = dsize[rank - 1] = 1;
    }
    if (chunkdims[rank - 1] > 1) {
      chunkdims[rank - 1] = 1;
    }
    dataspace = H5Screate_simple(rank, mydim1, maxdims);
  } else {
    if (unlimiteddim) {
      dataspace = H5Screate_simple(rank, mydim, maxdims);
    } else {
      /* dataset creation */
      dataspace = H5Screate_simple(rank, mydim, NULL);
    }
  }
  datatype1 = H5Tcopy(dtype);
  if (type == NXnumtype::CHAR) {
    H5Tset_size(datatype1, byte_zahl);
    /*       H5Tset_strpad(H5T_STR_SPACEPAD); */
  }
  hid_t dID;
  if (comp == NXcompression::LZW) {
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    iNew = H5Pset_chunk(cparms, rank, chunkdims);
    if (iNew < 0) {
      msg << "Size of chunks could not be set";
      throw NXEXCEPTION(msg.str());
    }
    H5Pset_shuffle(cparms); // mrt: improves compression
    H5Pset_deflate(cparms, default_deflate_level);
    dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
  } else if (comp == NXcompression::NONE) {
    if (unlimiteddim) {
      cparms = H5Pcreate(H5P_DATASET_CREATE);
      iNew = H5Pset_chunk(cparms, rank, chunkdims);
      if (iNew < 0) {
        msg << "Size of chunks could not be set";
        throw NXEXCEPTION(msg.str());
      }
      dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
    } else {
      dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
  } else if (comp == NXcompression::CHUNK) {
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    iNew = H5Pset_chunk(cparms, rank, chunkdims);
    if (iNew < 0) {
      msg << "Size of chunks could not be set";
      throw NXEXCEPTION(msg.str());
    }
    dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);

  } else {
    NXReportError("HDF5 doesn't support selected compression method! Dataset created without compression");
    dID = H5Dcreate(pFile->iCurrentG, name.c_str(), datatype1, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  }
  if (dID < 0) {
    msg << "Creating chunked dataset failed";
    throw NXEXCEPTION(msg.str());
  } else {
    pFile->iCurrentD = dID;
  }
  if (unlimiteddim) {
    iNew = H5Dset_extent(pFile->iCurrentD, dsize);
    if (iNew < 0) {
      msg << "Cannot create dataset " << name;
      throw NXEXCEPTION(msg.str());
    }
  }
  herr_t iRet;
  if (cparms != -1) {
    H5Pclose(cparms);
  }
  iRet = H5Sclose(dataspace);
  iRet += H5Tclose(datatype1);
  iRet += H5Dclose(pFile->iCurrentD);
  pFile->iCurrentD = 0;
  if (iRet < 0) {
    msg << "HDF cannot close dataset";
    throw NXEXCEPTION(msg.str());
  }
  NexusAddress absaddr(formAbsoluteAddress(name));
  registerEntry(absaddr, scientific_data_set);
  if (open_data) {
    this->openData(absaddr);
  }
}

template <typename NumT> void File::putSlab(NumT const *data, DimSizeVector const &start, DimSizeVector const &size) {
  if (data == NULL) {
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
  if (start.size() > NX_MAXRANK) {
    stringstream msg;
    msg << "The supplied rank exceeds the max allowed rank " << start.size() << " > " << NX_MAXRANK;
    throw NXEXCEPTION(msg.str());
  }

  pNexusFile5 pFile;
  int iRet, rank;
  hsize_t myStart[H5S_MAX_RANK];
  hsize_t mySize[H5S_MAX_RANK];
  hsize_t dsize[H5S_MAX_RANK], thedims[H5S_MAX_RANK], maxdims[H5S_MAX_RANK];
  hid_t filespace, dataspace;
  bool unlimiteddim = false;
  stringstream msg;
  msg << "putSlab(data, " << toString(start) << ", " << toString(size) << ") failed: ";

  pFile = assertNXID(m_pfile_id);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    msg << "no dataset open";
    throw NXEXCEPTION(msg.str());
  }
  rank = H5Sget_simple_extent_ndims(pFile->iCurrentS);
  if (rank < 0) {
    msg << "cannot get rank";
    throw NXEXCEPTION(msg.str());
  }
  iRet = H5Sget_simple_extent_dims(pFile->iCurrentS, thedims, maxdims);
  if (iRet < 0) {
    msg << "cannot get dimensions";
    throw NXEXCEPTION(msg.str());
  }

  for (int i = 0; i < rank; i++) {
    myStart[i] = static_cast<hsize_t>(start[i]);
    mySize[i] = static_cast<hsize_t>(size[i]);
    dsize[i] = static_cast<hsize_t>(start[i] + size[i]);
    if (maxdims[i] == H5S_UNLIMITED) {
      unlimiteddim = true;
    }
  }
  if (H5Tget_class(pFile->iCurrentT) == H5T_STRING) {
    mySize[rank - 1] = 1;
    myStart[rank - 1] = 0;
    dsize[rank - 1] = 1;
  }
  dataspace = H5Screate_simple(rank, mySize, NULL);
  if (unlimiteddim) {
    for (int i = 0; i < rank; i++) {
      if (dsize[i] < thedims[i]) {
        dsize[i] = thedims[i];
      }
    }
    iRet = H5Dset_extent(pFile->iCurrentD, dsize);
    if (iRet < 0) {
      msg << "extend slab failed";
      throw NXEXCEPTION(msg.str());
    }

    filespace = H5Dget_space(pFile->iCurrentD);

    /* define slab */
    iRet = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, myStart, NULL, mySize, NULL);
    /* deal with HDF errors */
    if (iRet < 0) {
      msg << "selecting slab failed";
      throw NXEXCEPTION(msg.str());
    }
    /* write slab */
    iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, dataspace, filespace, H5P_DEFAULT, data);
    if (iRet < 0) {
      msg << "writing slab failed";
      throw NXEXCEPTION(msg.str());
    }
    /* update with new size */
    iRet = H5Sclose(pFile->iCurrentS);
    if (iRet < 0) {
      msg << "updating size failed";
      throw NXEXCEPTION(msg.str());
    }
    pFile->iCurrentS = filespace;
  } else {
    /* define slab */
    iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, myStart, NULL, mySize, NULL);
    /* deal with HDF errors */
    if (iRet < 0) {
      msg << "selecting slab failed";
      throw NXEXCEPTION(msg.str());
    }
    /* write slab */
    iRet = H5Dwrite(pFile->iCurrentD, pFile->iCurrentT, dataspace, pFile->iCurrentS, H5P_DEFAULT, data);
    if (iRet < 0) {
      msg << "writing slab failed";
      throw NXEXCEPTION(msg.str());
    }
  }
  /* deal with HDF errors */
  iRet = H5Sclose(dataspace);
  if (iRet < 0) {
    msg << "closing slab failed";
    throw NXEXCEPTION(msg.str());
  }
}

template <typename NumT>
void File::putSlab(vector<NumT> const &data, DimSizeVector const &start, DimSizeVector const &size) {
  if (data.empty()) {
    throw NXEXCEPTION("Supplied empty data to putSlab");
  }
  this->putSlab(data.data(), start, size);
}

template <typename NumT> void File::putSlab(vector<NumT> const &data, dimsize_t const start, dimsize_t const size) {
  DimSizeVector start_v{start};
  DimSizeVector size_v{size};
  this->putSlab(data, start_v, size_v);
}

template <typename NumT> void File::getSlab(NumT *data, DimSizeVector const &start, DimSizeVector const &size) {
  if (data == NULL) {
    throw NXEXCEPTION("Supplied null pointer to getSlab");
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

  pNexusFile5 pFile;
  hsize_t myStart[H5S_MAX_RANK] = {0};
  hsize_t mySize[H5S_MAX_RANK] = {0};
  hsize_t mStart[H5S_MAX_RANK] = {0};
  hid_t memspace, iRet;
  H5T_class_t tclass;
  hid_t memtype_id;
  char *tmp_data = NULL;
  int iRank;

  pFile = assertNXID(m_pfile_id);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    throw NXEXCEPTION("No dataset open");
  }
  tclass = H5Tget_class(pFile->iCurrentT);
  /* map datatypes of other platforms */
  if (tclass == H5T_STRING) {
    memtype_id = pFile->iCurrentT;
  } else {
    memtype_id = h5MemType(pFile->iCurrentT);
  }

  iRank = H5Sget_simple_extent_ndims(pFile->iCurrentS);

  if (iRank == 0) {
    /* this is an unslabbale SCALAR */
    hid_t filespace = H5Dget_space(pFile->iCurrentD);
    memspace = H5Screate(H5S_SCALAR);
    H5Sselect_all(filespace);
    iRet = H5Dread(pFile->iCurrentD, memtype_id, memspace, filespace, H5P_DEFAULT, data);
    H5Sclose(filespace);
  } else {

    for (int i = 0; i < iRank; i++) {
      myStart[i] = static_cast<hsize_t>(start[i]);
      mySize[i] = static_cast<hsize_t>(size[i]);
      mStart[i] = static_cast<hsize_t>(0);
    }

    /*
     * this does not work for multidimensional string arrays.
     */
    int mtype = 0;
    if (tclass == H5T_STRING) {
      mtype = NXnumtype::CHAR;
      if (mySize[0] == 1) {
        mySize[0] = H5Tget_size(pFile->iCurrentT);
      }
      if (mySize[0] > 0) {
        tmp_data = static_cast<char *>(calloc(mySize[0], sizeof(char)));
      } else {
        tmp_data = static_cast<char *>(calloc(1, sizeof(char)));
      }
      iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, mStart, NULL, mySize, NULL);
    } else {
      iRet = H5Sselect_hyperslab(pFile->iCurrentS, H5S_SELECT_SET, myStart, NULL, mySize, NULL);
    }
    /* define slab */
    /* deal with HDF errors */
    if (iRet < 0) {
      throw NXEXCEPTION("Selecting slab failed");
    }

    memspace = H5Screate_simple(iRank, mySize, NULL);
    iRet = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, mStart, NULL, mySize, NULL);
    if (iRet < 0) {
      throw NXEXCEPTION("Selecting memspace failed");
    }
    /* read slab */
    if (mtype == NXnumtype::CHAR) {
      iRet = H5Dread(pFile->iCurrentD, memtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, tmp_data);
      char const *data1;
      data1 = tmp_data + myStart[0];
      strncpy(static_cast<char *>(static_cast<void *>(data)), data1, static_cast<size_t>(size[0]));
      free(tmp_data);
    } else {
      iRet = H5Dread(pFile->iCurrentD, memtype_id, memspace, pFile->iCurrentS, H5P_DEFAULT, data);
    }
  }
  /* cleanup */
  H5Sclose(memspace);
  H5Tclose(tclass);

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
  const DimVector dims{static_cast<dimsize_t>(my_value.size())};
  this->makeData(name, NXnumtype::CHAR, dims, true);

  this->putData(my_value);

  this->closeData();
}

template <typename NumT> void File::writeData(std::string const &name, vector<NumT> const &value) {
  DimVector const dims{static_cast<dimsize_t>(value.size())};
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
  DimSizeVector chunk_dims{chunk};
  // Use chunking without using compression
  this->makeCompData(name, getType<NumT>(), dims, NXcompression::NONE, chunk_dims, true);
  this->putSlab(value, dimsize_t(0), dimsize_t(value.size()));
  this->closeData();
}

template <typename NumT>
void File::writeExtendibleData(std::string const &name, vector<NumT> const &value, DimVector const &dims,
                               DimSizeVector const &chunk) {
  // Create the data with unlimited 0th dimensions
  DimVector unlim_dims(dims);
  unlim_dims[0] = NX_UNLIMITED;
  // Use chunking without using compression
  this->makeCompData(name, getType<NumT>(), unlim_dims, NXcompression::NONE, chunk, true);
  // And put that slab of that of that given size in there
  DimSizeVector start(dims.size(), 0);
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
  DimSizeVector start(dims.size(), 0);
  this->putSlab(value, start, dims);
  this->closeData();
}

template <typename NumT>
void File::writeCompData(std::string const &name, vector<NumT> const &value, DimVector const &dims,
                         NXcompression const comp, DimSizeVector const &bufsize) {
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
  std::vector<NumT> dataVector;
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

  pNexusFile5 pFile;
  std::size_t iRank;
  NXnumtype mType;
  hsize_t myDim[H5S_MAX_RANK];
  H5T_class_t tclass;
  hid_t memType;
  char *vlData = NULL;

  pFile = assertNXID(m_pfile_id);
  /* check if there is an Dataset open */
  if (pFile->iCurrentD == 0) {
    throw NXEXCEPTION("getInfo Error: no dataset open");
  }

  /* read information */
  tclass = H5Tget_class(pFile->iCurrentT);
  mType = hdf5ToNXType(tclass, pFile->iCurrentT);
  iRank = H5Sget_simple_extent_dims(pFile->iCurrentS, myDim, NULL);
  if (iRank == 0) {
    iRank = 1; /* we pretend */
    myDim[0] = 1;
  }
  /* conversion to proper ints for the platform */
  info.type = mType;
  if (tclass == H5T_STRING && myDim[iRank - 1] == 1) {
    if (H5Tis_variable_str(pFile->iCurrentT)) {
      /* this will not work for arrays of strings */
      memType = H5Tcopy(H5T_C_S1);
      H5Tset_size(memType, H5T_VARIABLE);
      H5Dread(pFile->iCurrentD, memType, H5S_ALL, H5S_ALL, H5P_DEFAULT, &vlData);
      if (vlData != NULL) {
        myDim[iRank - 1] = strlen(vlData) + 1;
        H5Dvlen_reclaim(memType, pFile->iCurrentS, H5P_DEFAULT, &vlData);
      }
      H5Tclose(memType);
    } else {
      myDim[iRank - 1] = H5Tget_size(pFile->iCurrentT);
    }
  }

  info.dims.resize(iRank);
  for (std::size_t i = 0; i < iRank; i++) {
    info.dims[i] = myDim[i];
  }

  // Trim 1D CHAR arrays to the actual string length
  if ((info.type == NXnumtype::CHAR) && (iRank == 1)) {
    char *buf = static_cast<char *>(malloc(static_cast<size_t>((info.dims[0] + 1) * sizeof(char))));
    memset(buf, 0, static_cast<size_t>((info.dims[0] + 1) * sizeof(char)));
    this->getData<char>(buf);
    info.dims[0] = static_cast<int64_t>(strlen(buf));
    free(buf);
  }
  return info;
}

namespace {
herr_t gr_iterate_cb(hid_t loc_id, const char *name, const H5L_info2_t *info, void *op_data) {
  UNUSED_ARG(info);
  Entries *entryData = static_cast<Entries *>(op_data);
  std::string nxclass;

  H5O_info_t obj_info;
  H5Oget_info_by_name(loc_id, name, &obj_info, H5O_INFO_ALL, H5P_DEFAULT);
  if (obj_info.type == H5O_TYPE_GROUP) {
    hid_t grp = H5Gopen(loc_id, name, H5P_DEFAULT);
    if (grp >= 0) {
      H5::Group group(grp);
      try {
        Mantid::NeXus::H5Util::readStringAttribute(group, group_class_spec, nxclass);
      } catch (...) {
        nxclass = unknown_group_spec;
      }
      H5Gclose(grp);
    }
  } else if (obj_info.type == H5O_TYPE_DATASET) {
    nxclass = scientific_data_set;
  } else {
    nxclass = "unknown";
  }

  (*entryData)[name] = nxclass;
  return 0;
}
} // namespace

Entries File::getEntries() const {
  Entries result;
  this->getEntries(result);
  return result;
}

void File::getEntries(Entries &result) const {
  result.clear();

  int iRet = H5Literate_by_name(m_pfile_id->iFID, m_pfile_id->groupaddr.c_str(), H5_INDEX_NAME, H5_ITER_NATIVE, nullptr,
                                gr_iterate_cb, &result, H5P_DEFAULT);
  if (iRet < 0) {
    throw NXEXCEPTION("H5Literate failed on group: " + m_pfile_id->groupaddr);
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
    Mantid::NeXus::H5Util::writeNumAttribute<NumT>(*current, name, value);
  } catch (H5::Exception const &e) {
    throw NXEXCEPTION(e.getDetailMsg());
  }
}

void File::putAttr(const char *name, const char *value) {
  if (name == NULL) {
    throw NXEXCEPTION("Specified name as null to putAttr");
  }
  if (value == NULL) {
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
    Mantid::NeXus::H5Util::writeStrAttribute(*current, name, my_value);
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
    value = Mantid::NeXus::H5Util::readNumAttributeCoerce<NumT>(*current, name);
  } catch (H5::Exception const &e) {
    throw NXEXCEPTION(e.getDetailMsg());
  }
}

string File::getStrAttr(std::string const &name) {
  // NOTE ensure the H5Cpp objects created here are properly destroyed when exiting scope
  std::string res("");
  auto current = getCurrentObject();
  try {
    Mantid::NeXus::H5Util::readStringAttribute(*current, name, res);
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
    hid_t attr = H5Aopen_by_idx(current, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, idx, H5P_DEFAULT, H5P_DEFAULT);
    // 1. get the attribute name
    std::size_t namelen = H5Aget_name(attr, 0, NULL);
    char *cname = new char[namelen + 1];
    H5Aget_name(attr, namelen + 1, cname);
    cname[namelen] = '\0'; // ensure null termination
    // do not include the group class spec
    if (group_class_spec == cname) {
      continue;
    }
    // 2. get the attribute type
    hid_t attrtype = H5Aget_type(attr);
    H5T_class_t attrclass = H5Tget_class(attrtype);
    NXnumtype type = hdf5ToNXType(attrclass, attrtype);
    // 3. get the attribute length
    hid_t attrspace = H5Aget_space(attr);
    int rank = H5Sget_simple_extent_ndims(attrspace);
    if (rank > 2 || (rank == 2 && type != NXnumtype::CHAR)) {
      throw NXEXCEPTION("ERROR iterating through attributes found array attribute not understood by this api");
    }
    hsize_t *dims = new hsize_t[rank];
    H5Sget_simple_extent_dims(attrspace, dims, NULL);
    std::size_t length = 1;
    if (type == NXnumtype::CHAR) {
      length = getStrAttr(cname).size();
    }
    for (int d = 0; d < rank; d++) {
      length *= dims[d];
    }
    delete[] dims;
    // now add info to the vector
    infos.emplace_back(type, length, cname);
    delete[] cname;
    H5Sclose(attrspace);
    H5Tclose(attrtype);
    H5Aclose(attr);
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
  if (m_pfile_id->iCurrentG == 0) {
    throw NXEXCEPTION("getGroupID failed, No current group open");
  }

  try {
    getAttr(target_attr_name, link.targetAddress);
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
    getAttr(target_attr_name, link.targetAddress);
  } catch (const Mantid::Nexus::Exception &) {
    link.targetAddress = getAddress();
  }

  link.linkType = NXentrytype::sds;
  return link;
}

void File::makeLink(NXlink const &link) {
  if (m_pfile_id->iCurrentG == 0) { /* root level, can not link here */
    throw NXEXCEPTION("makeLink failed");
  }

  // locate name of the element to link
  NexusAddress target(link.targetAddress);
  std::string itemName(target.stem());

  /*
     build addressname to link from our current group and the name
     of the thing to link
   */
  std::string linkTarget(m_pfile_id->groupaddr / itemName);
  H5Lcreate_hard(m_pfile_id->iFID, link.targetAddress.c_str(), H5L_SAME_LOC, linkTarget.c_str(), H5P_DEFAULT,
                 H5P_DEFAULT);

  // register the entry
  registerEntry(linkTarget, m_descriptor.classTypeForName(link.targetAddress));

  // set the target attribute
  NexusAddress here(m_address);
  openAddress(linkTarget);
  putAttr(target_attr_name, link.targetAddress);
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
template MANTID_NEXUS_DLL void File::getData(char *data);
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
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<double> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int8_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint8_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int16_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint16_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int32_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint32_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int64_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint64_t> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<char> const &value,
                                                         DimVector const &dims, DimSizeVector const &chunk);

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
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<double> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int8_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint8_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int16_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint16_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int32_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint32_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int64_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint64_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);

// READ / WRITE DATA -- SLAB / EXTENDIBLE

template MANTID_NEXUS_DLL void File::getSlab(float *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(double *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int8_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint8_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int16_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint16_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int32_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint32_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(int64_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(uint64_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(char *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::getSlab(bool *data, const DimSizeVector &start, const DimSizeVector &size);

template MANTID_NEXUS_DLL void File::putSlab(const float *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const double *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int8_t *data, const DimSizeVector &start, const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint8_t *data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int16_t *data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint16_t *data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int32_t *data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint32_t *data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const int64_t *data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const uint64_t *data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const char *data, const DimSizeVector &start, const DimSizeVector &size);

template MANTID_NEXUS_DLL void File::putSlab(const std::vector<float> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<double> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int8_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint8_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int16_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint16_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int32_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint32_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int64_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);
template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint64_t> &data, const DimSizeVector &start,
                                             const DimSizeVector &size);

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
