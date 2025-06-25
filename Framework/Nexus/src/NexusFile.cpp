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

#define NAPI_CALL(status, msg)                                                                                         \
  NXstatus tmp = (status);                                                                                             \
  if (tmp != NXstatus::NX_OK) {                                                                                        \
    throw NXEXCEPTION(msg);                                                                                            \
  }

#define DEBUG_LOG()                                                                                                    \
  printf("%s:%d %s\n", __FILE__, __LINE__, __func__);                                                                  \
  fflush(stdout);

#define FILE_EXISTS(filename)                                                                                          \
  if (!std::filesystem::exists(filename)) {                                                                            \
    throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__));                                  \
  } else {                                                                                                             \
    DEBUG_LOG();                                                                                                       \
  }
/**
 * \file NexusFile.cpp
 * The implementation of the NeXus C++ API
 */

namespace { // anonymous namespace to keep it in the file

std::string const scientific_data_set("SDS");

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

NexusFile5::NexusFile5(std::string const &filename, NXaccess const am)
    : iStack5{{"", 0, 0}}, iAtt5{"", 0, 0}, iFID(0), iCurrentG(0), iCurrentD(0), iCurrentS(0), iCurrentT(0),
      iCurrentA(0), iNX(0), iStackPtr(0), name_ref(""), name_tmp("") {
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
  fapl = create_file_access_plist(filename);

  if (am != NXACC_CREATE5) {
    if (H5Fis_accessible(filename.c_str(), fapl) <= 0) {
      throw Mantid::Nexus::Exception("File is not HDF5", "NexusFile5 constructor", filename);
    }
    iFID = H5Fopen(filename.c_str(), am, fapl);
  } else {
    iFID = H5Fcreate(filename.c_str(), am, H5P_DEFAULT, fapl);
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
  if (am == NXACC_CREATE5) {
    // open the root as a group and add these attributes
    hid_t root_id = H5Gopen(iFID, "/", H5P_DEFAULT);

    std::vector<std::pair<std::string, std::string>> attrs{{"NeXus_version", NEXUS_VERSION},
                                                           {"file_name", filename},
                                                           {"HDF5_Version", version_str},
                                                           {"file_time", NXIformatNeXusTime()},
                                                           {"NX_class", "NXroot"}};
    for (auto attr : attrs) {
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

  iStack5[0].iVref = 0; // root!
};

NexusFile5::NexusFile5(NexusFile5 const &origHandle)
    : iStack5{{"", 0, 0}}, iAtt5{"", 0, 0}, iFID(0), iCurrentG(0), iCurrentD(0), iCurrentS(0), iCurrentT(0),
      iCurrentA(0), iNX(0), iStackPtr(0), name_ref(""), name_tmp("") {
  iFID = H5Freopen(origHandle.iFID);
  if (iFID <= 0) {
    throw Mantid::Nexus::Exception("Error reopening file");
  }
  iStack5[0].iVref = 0; // root!
};

NexusFile5 &NexusFile5::operator=(NexusFile5 const &origHandle) {
  this->iFID = H5Freopen(origHandle.iFID);
  return *this;
}

NexusFile5::~NexusFile5() {
  if (iCurrentD != 0) {
    H5Dclose(iCurrentD);
  }
  for (auto entry : iStack5) {
    H5Gclose(entry.iVref);
  }
  H5Fclose(iFID);
  H5garbage_collect();
}

namespace Mantid::Nexus {

// catch for undefined types
template <typename NumT> NXnumtype getType(NumT const number) {
  stringstream msg;
  msg << "NeXus::getType() does not know type of " << typeid(number).name();
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
    : m_filename(filename), m_access(access), m_close_handle(true), m_descriptor(m_filename, m_access) {
  this->initOpenFile(m_filename, m_access);
}

File::File(const char *filename, const NXaccess access)
    : m_filename(filename), m_access(access), m_close_handle(true), m_descriptor(m_filename, m_access) {
  this->initOpenFile(m_filename, m_access);
}

void File::initOpenFile(const string &filename, const NXaccess access) {
  if (filename.empty()) {
    throw NXEXCEPTION("Filename specified is empty constructor");
  }
  NexusFile5 tmp(filename, access);
  if (tmp.iFID <= 0) {
    stringstream msg;
    msg << "NXopen(" << filename << ", " << access << ") failed";
    throw NXEXCEPTION(msg.str());
  } else {
    m_pfile_id = std::make_shared<NexusFile5>(tmp);
  }
}

// copy constructors

File::File(File const &f)
    : m_filename(f.m_filename), m_access(f.m_access), m_close_handle(false), m_pfile_id(f.m_pfile_id),
      m_descriptor(f.m_descriptor) {}

File::File(File const *const pf)
    : m_filename(pf->m_filename), m_access(pf->m_access), m_close_handle(false), m_pfile_id(pf->m_pfile_id),
      m_descriptor(pf->m_descriptor) {}

File::File(std::shared_ptr<File> pf)
    : m_filename(pf->m_filename), m_access(pf->m_access), m_close_handle(false), m_pfile_id(pf->m_pfile_id),
      m_descriptor(pf->m_descriptor) {}

File &File::operator=(File const &f) {
  if (this == &f) {
  } else {
    this->m_filename = f.m_filename;
    this->m_access = f.m_access;
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
    for (auto entry : m_pfile_id->iStack5) {
      H5Gclose(entry.iVref);
    }
    H5Fclose(m_pfile_id->iFID);
    NXI5KillDir(m_pfile_id.get());
    H5garbage_collect();
    m_pfile_id = nullptr;
  }
  m_pfile_id.reset();
}

void File::flush() {
  herr_t iRet;
  if (m_pfile_id->iCurrentD != 0) {
    iRet = H5Fflush(m_pfile_id->iCurrentD, H5F_SCOPE_LOCAL);
  } else if (m_pfile_id->iCurrentG != 0) {
    iRet = H5Fflush(m_pfile_id->iCurrentG, H5F_SCOPE_LOCAL);
  } else {
    iRet = H5Fflush(m_pfile_id->iFID, H5F_SCOPE_LOCAL);
  }
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
  NAPI_CALL(NXopenaddress(m_pfile_id.get(), address), "NXopenaddress(" + address + ") failed");
}

void File::openGroupAddress(std::string const &address) {
  if (address.empty()) {
    throw NXEXCEPTION("Supplied empty address");
  }
  NAPI_CALL(NXopengroupaddress(m_pfile_id.get(), address), "NXopengroupaddress(" + address + ") failed");
}

std::string File::getAddress() {
  std::string address;
  NAPI_CALL(NXgetaddress(m_pfile_id.get(), address), "NXgetaddress() failed");
  // openAddress expects "/" to open root
  // for consitency, this should return "/" at the root
  if (address == "") {
    address = "/";
  }
  return address;
}

bool File::hasAddress(std::string const &name) {
  if (name == "/") { // NexusDescriptor does not keep the root, but it does exist
    return true;
  }
  std::string const address = formAbsoluteAddress(name);
  return m_descriptor.isEntry(address);
}

bool File::hasGroup(std::string const &name, std::string const &class_type) {
  std::string const address = formAbsoluteAddress(name);
  return m_descriptor.isEntry(address, class_type);
}

bool File::hasData(std::string const &name) {
  std::string const address = formAbsoluteAddress(name);
  return m_descriptor.isEntry(address, scientific_data_set);
}

bool File::isDataSetOpen() {
  NXlink id;
  if (NXgetdataID(m_pfile_id.get(), id) == NXstatus::NX_ERROR) {
    return false;
  } else {
    return true;
  }
}

bool File::isDataInt() {
  Info info = this->getInfo();
  switch (info.type) {
  case NXnumtype::INT8:
  case NXnumtype::UINT8:
  case NXnumtype::INT16:
  case NXnumtype::UINT16:
  case NXnumtype::INT32:
  case NXnumtype::UINT32:
    return true;
  default:
    return false;
  }
}

std::string File::formAbsoluteAddress(std::string const &name) {
  std::string new_name;
  if (name.front() == '/') {
    new_name = name;
  } else {
    std::string to_root = this->getAddress();
    if (to_root == "/") {
      to_root = "";
    }
    new_name = to_root + "/" + name;
  }
  // the caller is responsible for checking that it exists
  return new_name;
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

void File::makeGroup(const std::string &name, const std::string &class_name, bool open_group) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to makeGroup");
  }
  if (class_name.empty()) {
    throw NXEXCEPTION("Supplied empty class name to makeGroup");
  }

  pNexusFile5 pFile;
  hid_t iVID;
  hid_t attr1, aid1, aid2;
  std::string pBuffer;

  pFile = assertNXID(m_pfile_id);
  /* create and configure the group */
  if (pFile->iCurrentG == 0) {
    pBuffer = "/" + std::string(name);
  } else {
    pBuffer = "/" + std::string(pFile->name_ref) + "/" + std::string(name);
  }
  iVID = H5Gcreate(pFile->iFID, pBuffer.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (iVID < 0) {
    throw NXEXCEPTION("Could not create Group");
  }
  aid2 = H5Screate(H5S_SCALAR);
  aid1 = H5Tcopy(H5T_C_S1);
  H5Tset_size(aid1, class_name.length());
  attr1 = H5Acreate(iVID, "NX_class", aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attr1 < 0) {
    throw NXEXCEPTION("Failed to store class name");
  }
  if (H5Awrite(attr1, aid1, const_cast<char *>(static_cast<const char *>(class_name.c_str()))) < 0) {
    throw NXEXCEPTION("Failed to store class name");
  }
  /* close group */
  hid_t iRet = H5Sclose(aid2);
  iRet += H5Tclose(aid1);
  iRet += H5Aclose(attr1);
  iRet += H5Gclose(iVID);
  UNUSED_ARG(iRet);

  registerEntry(formAbsoluteAddress(name), class_name);
  if (open_group) {
    this->openGroup(name, class_name);
  }
}

void File::openGroup(std::string const &name, std::string const &class_name) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name");
  }
  if (class_name.empty()) {
    throw NXEXCEPTION("Supplied empty class name");
  }
  pNexusFile5 pFile;
  hid_t attr1, iVID;
  herr_t iRet;
  std::string pBuffer;

  pFile = assertNXID(m_pfile_id);
  if (pFile->iCurrentG == 0) {
    pBuffer = name;
  } else {
    pBuffer = pFile->name_tmp + "/" + name;
  }
  iVID = H5Gopen(pFile->iFID, pBuffer.c_str(), H5P_DEFAULT);
  if (iVID < 0) {
    std::stringstream msg;
    msg << "Group " << pFile->name_tmp << " does not exist";
    throw NXEXCEPTION(msg.str());
  }
  pFile->iCurrentG = iVID;
  pFile->name_tmp = pBuffer;
  pFile->name_ref = pBuffer;

  if ((!class_name.empty()) && (strcmp(class_name.c_str(), NX_UNKNOWN_GROUP) != 0)) {
    /* check group attribute */
    iRet = H5Aiterate(pFile->iCurrentG, H5_INDEX_CRT_ORDER, H5_ITER_INC, 0, attr_check, NULL);
    if (iRet < 0) {
      throw NXEXCEPTION("Error iterating through attribute list");
    } else if (iRet == 1) {
      /* group attribute was found */
    } else {
      /* no group attribute available */
      throw NXEXCEPTION("No group attribute available");
    }
    /* check contents of group attribute */
    attr1 = H5Aopen_by_name(pFile->iCurrentG, ".", "NX_class", H5P_DEFAULT, H5P_DEFAULT);
    if (attr1 < 0) {
      throw NXEXCEPTION("Error opening NX_class group attribute");
    }
    const H5::Group group(pFile->iCurrentG);
    std::string data;
    Mantid::NeXus::H5Util::readStringAttribute(group, "NX_class", data);
    if (data == class_name) {
      /* test OK */
    } else {
      H5Aclose(attr1);
      std::stringstream msg;
      msg << "Group class is not identical: '" << data << "' != '" << class_name << "'";
      throw NXEXCEPTION(msg.str());
    }
    H5Aclose(attr1);
  }

  /* maintain stack */
  pFile->iStack5.emplace_back(name, pFile->iCurrentG, 0);
  pFile->iStackPtr++;
  pFile->iCurrentIDX = 0;
  pFile->iCurrentD = 0;
  NXI5KillDir(pFile);
}

void File::closeGroup() {
  pNexusFile5 pFile;

  pFile = assertNXID(m_pfile_id);
  /* first catch the trivial case: we are at root and cannot get
     deeper into a negative directory hierarchy (anti-directory)
   */
  if (pFile->iCurrentG == 0) {
    NXI5KillDir(pFile);
  } else {
    /* close the current group and decrement name_ref */
    H5Gclose(pFile->iCurrentG);
    size_t i = pFile->iStack5[pFile->iStackPtr].irefn.size();
    size_t ii = pFile->name_ref.size();
    if (pFile->iStackPtr > 1) {
      ii = ii - i - 1;
    } else {
      ii = ii - i;
    }
    if (ii > 0) {
      char const *uname = strdup(pFile->name_ref.c_str());
      char *u1name = NULL;
      u1name = static_cast<char *>(malloc((ii + 1) * sizeof(char)));
      memset(u1name, 0, ii);
      for (i = 0; i < ii; i++) {
        *(u1name + i) = *(uname + i);
      }
      *(u1name + i) = '\0';
      /*
         strncpy(u1name, uname, ii);
       */
      pFile->name_ref = u1name;
      pFile->name_tmp = u1name;
    } else {
      pFile->name_ref = "";
      pFile->name_tmp = "";
    }
    NXI5KillDir(pFile);
    pFile->iCurrentD = 0;
    pFile->iStackPtr--;
    if (pFile->iStackPtr > 0) {
      pFile->iCurrentG = pFile->iStack5[pFile->iStackPtr].iVref;
    } else {
      pFile->iCurrentG = 0;
    }
    pFile->iStack5.pop_back();
  }
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
  pNexusFile5 pFile;

  pFile = assertNXID(m_pfile_id);
  /* clear pending attribute directories first */
  NXI5KillAttDir(pFile);

  /* find the ID number and open the dataset */
  pFile->iCurrentD = H5Dopen(pFile->iCurrentG, name.c_str(), H5P_DEFAULT);
  if (pFile->iCurrentD < 0) {
    throw NXEXCEPTION("dataset (" + name + ") not found at this level");
  }
  /* find the ID number of datatype */
  pFile->iCurrentT = H5Dget_type(pFile->iCurrentD);
  if (pFile->iCurrentT < 0) {
    pFile->iCurrentT = 0;
    throw NXEXCEPTION("error opening dataset (" + name + ")");
  }
  /* find the ID number of dataspace */
  pFile->iCurrentS = H5Dget_space(pFile->iCurrentD);
  if (pFile->iCurrentS < 0) {
    pFile->iCurrentS = 0;
    throw NXEXCEPTION("HDF error opening dataset (" + name + ")");
  }
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

template <typename NumT> void File::getData(NumT *data) {
  if (data == NULL) {
    throw NXEXCEPTION("Supplied null pointer to write data to");
  }
  NAPI_CALL(NXgetdata(m_pfile_id.get(), data), "NXgetdata failed");
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
  char *value = new char[static_cast<size_t>(info.dims[0]) + 1]; // probably do not need +1, but being safe
  try {
    this->getData(value);
  } catch (const Exception &) {
    delete[] value;
    throw; // rethrow the original exception
  }
  std::string res(value, static_cast<size_t>(info.dims[0]));
  delete[] value;
  return res;
}

void File::closeData() {
  pNexusFile5 pFile;
  herr_t iRet;

  pFile = assertNXID(m_pfile_id);
  iRet = H5Sclose(pFile->iCurrentS);
  iRet += H5Tclose(pFile->iCurrentT);
  iRet += H5Dclose(pFile->iCurrentD);
  if (iRet < 0) {
    throw NXEXCEPTION("Cannot end access to dataset");
  }
  pFile->iCurrentD = 0;
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
  int i_type = static_cast<int>(type);

  hid_t datatype1, dataspace, iNew;
  hid_t dtype, cparms = -1;
  pNexusFile5 pFile;
  size_t byte_zahl = 0;
  hsize_t chunkdims[H5S_MAX_RANK];
  hsize_t mydim[H5S_MAX_RANK], mydim1[H5S_MAX_RANK];
  hsize_t dsize[H5S_MAX_RANK];
  hsize_t maxdims[H5S_MAX_RANK];
  unsigned int compress_level;
  bool unlimiteddim = false;
  int rank = static_cast<int>(dims.size());
  stringstream msg;
  msg << "NXcompmakedata64(" << name << ", " << i_type << ", " << dims.size() << ", " << toString(dims) << ", " << comp
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
  compress_level = 6;
  hid_t dID;
  if (comp == NXcompression::LZW) {
    cparms = H5Pcreate(H5P_DATASET_CREATE);
    iNew = H5Pset_chunk(cparms, rank, chunkdims);
    if (iNew < 0) {
      msg << "Size of chunks could not be set";
      throw NXEXCEPTION(msg.str());
    }
    H5Pset_shuffle(cparms); // mrt: improves compression
    H5Pset_deflate(cparms, compress_level);
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
  registerEntry(formAbsoluteAddress(name), scientific_data_set);
  if (open_data) {
    this->openData(name);
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
  hsize_t myStart[H5S_MAX_RANK];
  hsize_t mySize[H5S_MAX_RANK];
  hsize_t mStart[H5S_MAX_RANK];
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
      tmp_data = static_cast<char *>(malloc((size_t)mySize[0]));
      memset(tmp_data, 0, sizeof(mySize[0]));
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
  if (!dataVector.empty())
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
  std::size_t rank;
  NAPI_CALL(NXgetinfo64(m_pfile_id.get(), rank, info.dims, info.type), "NXgetinfo failed");
  return info;
}

void File::initGroupDir() { NAPI_CALL(NXinitgroupdir(m_pfile_id.get()), "NXinitgroupdir failed"); }

Entry File::getNextEntry() {
  // set up temporary variables to get the information
  std::string name, class_name;
  NXnumtype datatype;

  NXstatus status = NXgetnextentry(m_pfile_id.get(), name, class_name, datatype);
  if (status == NXstatus::NX_OK) {
    string str_name(name);
    string str_class(class_name);
    return Entry(str_name, str_class);
  } else if (status == NXstatus::NX_EOD) {
    return EOD_ENTRY;
  } else {
    throw NXEXCEPTION("NXgetnextentry failed");
  }
}

Entries File::getEntries() {
  Entries result;
  this->getEntries(result);
  return result;
}

void File::getEntries(Entries &result) {
  result.clear();
  this->initGroupDir();
  Entry temp;
  while (true) {
    temp = this->getNextEntry();
    if (temp == EOD_ENTRY) {
      break;
    } else {
      result.insert(temp);
    }
  }
}

std::string File::getTopLevelEntryName() {
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

template <typename NumT> void File::putAttr(const AttrInfo &info, NumT const *data) {
  if (info.name == NULL_STR) {
    throw NXEXCEPTION("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (info.name.empty()) {
    throw NXEXCEPTION("Supplied empty name to putAttr");
  }
  NAPI_CALL(NXputattr(m_pfile_id.get(), info.name, data, info.length, info.type),
            "NXputattr(" + info.name + ", data, " + std::to_string(info.length) + ", " + (string)info.type +
                ") failed");
}

template <typename NumT> void File::putAttr(std::string const &name, NumT const &value) {
  AttrInfo info;
  info.name = name;
  info.length = 1;
  info.type = getType<NumT>();
  this->putAttr(info, &value);
}

void File::putAttr(const char *name, const char *value) {
  if (name == NULL) {
    throw NXEXCEPTION("Specified name as null to putAttr");
  }
  if (value == NULL) {
    throw NXEXCEPTION("Specified value as null to putAttr");
  }
  string s_name(name);
  string s_value(value);
  this->putAttr(s_name, s_value);
}

void File::putAttr(const std::string &name, const string &value, const bool empty_add_space) {
  string my_value(value);
  if (my_value.empty() && empty_add_space)
    my_value = " "; // Make a default "space" to avoid errors.
  AttrInfo info;
  info.name = name;
  info.length = static_cast<unsigned int>(my_value.size());
  info.type = NXnumtype::CHAR;
  this->putAttr(info, my_value.data());
}

void File::getAttr(const AttrInfo &info, void *data, std::size_t length) {
  NXnumtype type = info.type;
  if (length == 0) {
    length = info.length;
  }
  NAPI_CALL(NXgetattr(m_pfile_id.get(), info.name, data, length, type), "NXgetattr(" + info.name + ") failed");
  if (type != info.type) {
    stringstream msg;
    msg << "NXgetattr(" << info.name << ") changed type [" << info.type << "->" << type << "]";
    throw NXEXCEPTION(msg.str());
  }
  // char attributes are always NULL terminated and so may change length
  if (length != info.length && type != NXnumtype::CHAR) {
    stringstream msg;
    msg << "NXgetattr(" << info.name << ") change length [" << info.length << "->" << length << "]";
    throw NXEXCEPTION(msg.str());
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
  NXnumtype type = getType<NumT>();
  std::size_t length = 0;
  NAPI_CALL(NXgetattr(m_pfile_id.get(), name, &value, length, type), "NXgetattr(" + name + ") failed");
}

string File::getStrAttr(std::string const &name) {
  // get info for this string attribute
  std::size_t rank;
  DimVector dims(NX_MAXRANK);
  NXnumtype datatype = NXnumtype::CHAR;
  NAPI_CALL(NXgetattrainfo(m_pfile_id.get(), name, rank, dims, datatype), "NXgetinfo failed");
  AttrInfo info{datatype, static_cast<size_t>(dims[0]), name};

  // do checks
  if (info.type != NXnumtype::CHAR) {
    stringstream msg;
    msg << "getStrAttr only works with strings (type=" << NXnumtype::CHAR << ") found type=" << info.type;
    throw NXEXCEPTION(msg.str());
  }
  char *value = new char[info.length + 1];
  try {
    this->getAttr(info, value, static_cast<int>(info.length) + 1);
  } catch (const Exception &) {
    // Avoid memory leak
    delete[] value;
    throw; // rethrow original exception
  }
  std::string res(value);
  delete[] value;

  return res;
}

// NAVIGATE ATTRIBUTES

void File::initAttrDir() { NAPI_CALL(NXinitattrdir(m_pfile_id.get()), "NXinitattrdir failed"); }

AttrInfo File::getNextAttr() {
  // string & name, int & length, NXnumtype type) {
  std::string name;
  NXnumtype type;

  std::size_t rank;
  DimVector dim(NX_MAXRANK);
  NXstatus status = NXgetnextattra(m_pfile_id.get(), name, rank, dim, type);
  if (status == NXstatus::NX_OK) {
    AttrInfo info;
    info.type = type;
    info.name = string(name);

    // scalar value
    if (rank == 0 || (rank == 1 && dim[0] == 1)) {
      info.length = 1;
      return info;
    }

    // char (=string) or number array (1 dim)
    if (rank == 1) {
      info.length = static_cast<unsigned int>(dim[0]);
      return info;
    }

    // string array (2 dim char array)
    if (rank == 2 && type == NXnumtype::CHAR) {
      info.length = 1;
      for (std::size_t d = 0; d < rank; ++d) {
        info.length *= static_cast<std::size_t>(dim[d]);
      }
      return info;
    }

    // TODO - AttrInfo cannot handle more complex ranks/dimensions, we need to throw an error
    std::cerr << "ERROR iterating through attributes found array attribute not understood by this api" << std::endl;
    throw NXEXCEPTION("getNextAttr failed");

  } else if (status == NXstatus::NX_EOD) {
    AttrInfo info;
    info.name = NULL_STR;
    info.length = 0;
    info.type = NXnumtype::BINARY; // junk value that shouldn't be checked for
    return info;
  } else {
    throw NXEXCEPTION("NXgetnextattra failed");
  }
}

vector<AttrInfo> File::getAttrInfos() {
  vector<AttrInfo> infos;
  this->initAttrDir();
  AttrInfo temp;
  while (true) {
    temp = this->getNextAttr();
    if (temp.name == NULL_STR) {
      break;
    }
    infos.push_back(temp);
  }
  return infos;
}

bool File::hasAttr(const std::string &name) {
  this->initAttrDir();
  AttrInfo temp;
  while (true) {
    temp = this->getNextAttr();
    if (temp.name == NULL_STR) {
      break;
    }
    if (temp.name == name)
      return true;
  }
  return false;
}

//------------------------------------------------------------------------------------------------------------------
// LINK METHODS
//------------------------------------------------------------------------------------------------------------------

NXlink File::getGroupID() {
  NXlink link;
  pNexusFile5 pFile;

  pFile = assertNXID(m_pfile_id);
  if (pFile->iCurrentG == 0) {
    throw NXEXCEPTION("getGroupID failed, No current group open");
  }

  try {
    getAttr("target", link.targetAddress);
  } catch (const Mantid::Nexus::Exception &) {
    link.targetAddress = buildCurrentAddress(pFile);
  }
  link.linkType = NXentrytype::group;
  return link;
}

NXlink File::getDataID() {
  NXlink link;
  pNexusFile5 pFile;

  pFile = assertNXID(m_pfile_id);

  /* we cannot return ID's when no datset is open */
  if (pFile->iCurrentD <= 0) {
    throw NXEXCEPTION("getDataID failed, No current dataset open");
  }

  try {
    getAttr("target", link.targetAddress);
  } catch (const Mantid::Nexus::Exception &) {
    link.targetAddress = buildCurrentAddress(pFile);
  }

  link.linkType = NXentrytype::sds;
  return link;
}

void File::makeLink(NXlink const &link) {
  pNexusFile5 pFile;
  std::string linkTarget;

  pFile = assertNXID(m_pfile_id);
  if (pFile->iCurrentG == 0) { /* root level, can not link here */
    throw NXEXCEPTION("makeLink failed");
  }

  /*
     locate name of the element to link
   */
  const char *itemName = strrchr(link.targetAddress.data(), '/');
  if (itemName == NULL) {
    throw NXEXCEPTION("makeLink failed bad link structure");
  }
  itemName++;

  /*
     build addressname to link from our current group and the name
     of the thing to link
   */
  linkTarget = "/" + pFile->name_ref + "/" + itemName;
  H5Lcreate_hard(pFile->iFID, link.targetAddress.c_str(), H5L_SAME_LOC, linkTarget.c_str(), H5P_DEFAULT, H5P_DEFAULT);

  hid_t dataID, aid2, aid1, attID;
  char name[] = "target";

  /*
     set the target attribute
   */
  if (link.linkType == NXentrytype::sds) {
    dataID = H5Dopen(pFile->iFID, link.targetAddress.c_str(), H5P_DEFAULT);
  } else {
    dataID = H5Gopen(pFile->iFID, link.targetAddress.c_str(), H5P_DEFAULT);
  }
  if (dataID < 0) {
    throw NXEXCEPTION("Internal error, address to link does not exist");
  }
  hid_t status = H5Aopen_by_name(dataID, ".", name, H5P_DEFAULT, H5P_DEFAULT);
  if (status > 0) {
    H5Aclose(status);
    status = H5Adelete(dataID, name);
    if (status < 0) {
      return;
    }
  }
  aid2 = H5Screate(H5S_SCALAR);
  aid1 = H5Tcopy(H5T_C_S1);
  H5Tset_size(aid1, link.targetAddress.size());
  attID = H5Acreate(dataID, name, aid1, aid2, H5P_DEFAULT, H5P_DEFAULT);
  if (attID < 0) {
    return;
  }
  UNUSED_ARG(H5Awrite(attID, aid1, link.targetAddress.c_str()));
  H5Tclose(aid1);
  H5Sclose(aid2);
  H5Aclose(attID);
  if (link.linkType == NXentrytype::sds) {
    H5Dclose(dataID);
  } else {
    H5Gclose(dataID);
  }
}

} // namespace Mantid::Nexus

// -------------------------- NXnumtype ----------------------------------------------------------------------------//
using namespace Mantid::Nexus;
int NXnumtype::validate_val(int const x) {
  int val = BAD;
  // NOTE for user-readability it is better to check all of these cases
  // cppcheck doesn't like this, as some of these have the same value, making checks redundant
  // cppcheck-suppress-begin knownConditionTrueFalse
  if ((x == FLOAT32) || (x == FLOAT64) || (x == INT8) || (x == UINT8) || (x == BOOLEAN) || (x == INT16) ||
      (x == UINT16) || (x == INT32) || (x == UINT32) || (x == INT64) || (x == UINT64) || (x == CHAR) || (x == BINARY) ||
      (x == BAD)) {
    val = x;
  }
  // cppcheck-suppress-end knownConditionTrueFalse
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
  // NOTE for user-readability it is better to check all of these cases
  // cppcheck doesn't like this, as some of these have the same value, making checks redundant
  // cppcheck-suppress-begin knownConditionTrueFalse
  std::string ret = NXTYPE_PRINT(BAD);
  if (m_val == FLOAT32) {
    ret = NXTYPE_PRINT(FLOAT32);
  } else if (m_val == FLOAT64) {
    ret = NXTYPE_PRINT(FLOAT64);
  } else if (m_val == INT8) {
    ret = NXTYPE_PRINT(INT8);
  } else if (m_val == UINT8) {
    ret = NXTYPE_PRINT(UINT8);
  } else if (m_val == BOOLEAN) {
    ret = NXTYPE_PRINT(BOOLEAN);
  } else if (m_val == INT16) {
    ret = NXTYPE_PRINT(INT16);
  } else if (m_val == UINT16) {
    ret = NXTYPE_PRINT(UINT16);
  } else if (m_val == INT32) {
    ret = NXTYPE_PRINT(INT32);
  } else if (m_val == UINT32) {
    ret = NXTYPE_PRINT(UINT32);
  } else if (m_val == INT64) {
    ret = NXTYPE_PRINT(INT64);
  } else if (m_val == UINT64) {
    ret = NXTYPE_PRINT(UINT64);
  } else if (m_val == CHAR) {
    ret = NXTYPE_PRINT(CHAR);
  } else if (m_val == BINARY) {
    ret = NXTYPE_PRINT(BINARY);
  }
  // cppcheck-suppress-end knownConditionTrueFalse
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
