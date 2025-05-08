#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NeXusException.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <numeric>
#include <sstream>
#include <typeinfo>

#define NXEXCEPTION(message) Exception(message, __func__, m_filename);

using namespace NeXus;
using namespace Mantid::NeXus;
using std::string;
using std::stringstream;
using std::vector;

#define NXEXCEPTION(message) Exception((message), __func__, m_filename);

#define NAPI_CALL(status, msg)                                                                                         \
  NXstatus tmp = (status);                                                                                             \
  if (tmp != NXstatus::NX_OK) {                                                                                        \
    throw NXEXCEPTION(msg);                                                                                            \
  }
typedef std::array<hsize_t, NX_MAXRANK> DimArray;

/**
 * \file NeXusFile.cpp
 * The implementation of the NeXus C++ API
 */

namespace { // anonymous namespace to keep it in the file

constexpr std::string group_class_spec("NX_class");
constexpr std::string target_attr_name("target");
constexpr std::string scientific_data_set("SDS");
constexpr int default_deflate_level(6);

template <typename NumT> static std::string toString(vector<NumT> const &data) {
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

template <typename T> static DimArray toDimArray(vector<T> const &small_v) {
  DimArray ret;
  size_t i = 0;
  // fill in all values from vector
  for (; i < small_v.size(); i++) {
    ret.at(i) = static_cast<hsize_t>(small_v[i]);
  }
  // pad rest with zero
  for (; i < ret.size(); i++) {
    ret.at(i) = 0;
  }
  return ret;
}

static std::map<int, void const *> const nxToHDF5Map{
    std::pair<int, void const *>(NXnumtype::CHAR, &H5::PredType::NATIVE_CHAR),
    std::pair<int, void const *>(NXnumtype::INT8, &H5::PredType::NATIVE_SCHAR),
    std::pair<int, void const *>(NXnumtype::UINT8, &H5::PredType::NATIVE_UCHAR),
    std::pair<int, void const *>(NXnumtype::INT16, &H5::PredType::NATIVE_INT16),
    std::pair<int, void const *>(NXnumtype::UINT16, &H5::PredType::NATIVE_UINT16),
    std::pair<int, void const *>(NXnumtype::INT32, &H5::PredType::NATIVE_INT32),
    std::pair<int, void const *>(NXnumtype::UINT32, &H5::PredType::NATIVE_UINT32),
    std::pair<int, void const *>(NXnumtype::INT64, &H5::PredType::NATIVE_INT64),
    std::pair<int, void const *>(NXnumtype::UINT64, &H5::PredType::NATIVE_UINT64),
    std::pair<int, void const *>(NXnumtype::FLOAT32, &H5::PredType::NATIVE_FLOAT),
    std::pair<int, void const *>(NXnumtype::FLOAT64, &H5::PredType::NATIVE_DOUBLE),
    std::pair<int, void const *>(NXnumtype::BOOLEAN, &H5::PredType::NATIVE_HBOOL)};

static H5::DataType nxToHDF5Type(NXnumtype const &datatype) {
  H5::DataType type;
  if (nxToHDF5Map.contains(datatype)) {
    type = *static_cast<H5::PredType const *>(nxToHDF5Map.at(datatype));
  } else {
    type = H5::PredType::NATIVE_HERR;
  }
  return type;
}

/*-------------------------------------------------------------------------
 * Function: hdf5ToNXType
 *
 * Purpose:	Convert a HDF5 class to a NeXus type;  it handles the following HDF5 classes
 *  H5T_STRING
 *  H5T_INTEGER
 *  H5T_FLOAT
 *
 * Return: the NeXus type
 *
 *-------------------------------------------------------------------------
 */
static NXnumtype hdf5ToNXType(H5::DataType const &dt) {
  NXnumtype iPtype = NXnumtype::BAD;

  if (dt == H5::PredType::NATIVE_CHAR) {
    iPtype = NXnumtype::CHAR;
  } else {
    // if the type is one of the predefined datatypes used by us, just get it
    auto findit = std::find_if(nxToHDF5Map.begin(), nxToHDF5Map.end(), [dt](auto const &x) -> bool {
      return *static_cast<H5::DataType const *>(x.second) == dt;
    });

    if (findit != nxToHDF5Map.end()) {
      iPtype = findit->first;
    } else {
      // if it's not a usual type, try to deduce the type from the datatype object
      auto tclass = dt.getClass();
      if (tclass == H5T_STRING) {
        iPtype = NXnumtype::CHAR;
      } else if (tclass == H5T_INTEGER) {
        size_t size = dt.getSize();
        H5T_sign_t sign = H5T_SGN_2;
        // NOTE H5Cpp only defines the getSign method for IntType objects
        // Can cause segfault if used on any other object.  Therefore we
        // have to default to assuming signed int and only go to unsigned
        // if we are able to deduce otherwise
        H5::IntType const *it = dynamic_cast<H5::IntType const *>(&dt);
        if (it != nullptr) {
          sign = it->getSign();
        }
        // signed integers
        if (sign == H5T_SGN_2) { // NOTE: the "2" means 2's-complement
          if (size == 1) {
            iPtype = NXnumtype::INT8;
          } else if (size == 2) {
            iPtype = NXnumtype::INT16;
          } else if (size == 4) {
            iPtype = NXnumtype::INT32;
          } else if (size == 8) {
            iPtype = NXnumtype::INT64;
          }
        }
        // unsigned integers
        else {
          if (size == 1) {
            iPtype = NXnumtype::UINT8;
          } else if (size == 2) {
            iPtype = NXnumtype::UINT16;
          } else if (size == 4) {
            iPtype = NXnumtype::UINT32;
          } else if (size == 8) {
            iPtype = NXnumtype::UINT64;
          }
        }
      } else if (tclass == H5T_FLOAT) {
        size_t size = dt.getSize();
        if (size == 4) {
          iPtype = NXnumtype::FLOAT32;
        } else if (size == 8) {
          iPtype = NXnumtype::FLOAT64;
        }
      }
    }
    if (iPtype == NXnumtype::BAD) {
      std::stringstream msg;
      msg << "cannot convert type=" << dt.getId();
      throw Exception(msg.str(), "hdf5ToNXtype");
    }
  }
  return iPtype;
}

void recursivePopulateEntries(H5::Group const &grp, Entries &entries) {
  // get the path and class of this group
  std::string const groupNameStr = grp.getObjName();
  std::string nxClass = "";
  if (groupNameStr != "/") {
    if (grp.attrExists(group_class_spec)) {
      H5::Attribute const attr = grp.openAttribute(group_class_spec);
      attr.read(attr.getDataType(), nxClass);
    }
  }

  if (!nxClass.empty()) {
    entries[groupNameStr] = nxClass;
  }

  // recursively grab all entries within this group
  for (hsize_t i = 0; i < grp.getNumObjs(); i++) {
    H5G_obj_t type = grp.getObjTypeByIdx(i);
    H5std_string memberName = grp.getObjnameByIdx(i);

    if (type == H5G_GROUP) {
      recursivePopulateEntries(grp.openGroup(memberName), entries);
    } else if (type == H5G_DATASET) {
      std::string absoluteEntryName = groupNameStr + "/" + memberName;
      entries[absoluteEntryName] = scientific_data_set;
    }
  }
}

} // end of anonymous namespace

namespace NeXus {

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

template <> MANTID_NEXUS_DLL NXnumtype getType(bool const) { return NXnumtype::BOOLEAN; }

} // namespace NeXus

namespace NeXus {

File::File(std::string const &filename, NXaccess const access)
    : H5File(filename, access, H5Util::defaultFileAcc()), m_filename(filename), m_access(access), m_path("/"),
      m_current(nullptr), m_close_handle(true), m_descriptor(filename) {};

File::File(char const *filename, NXaccess const access)
    : H5File(filename, access, H5Util::defaultFileAcc()), m_filename(filename), m_access(access), m_path("/"),
      m_current(nullptr), m_close_handle(true), m_descriptor(std::string(filename)) {};

// copy constructors

File::File(File const &f)
    : H5File(f), m_filename(f.m_filename), m_access(f.m_access), m_path(f.m_path), m_current(f.m_current),
      m_close_handle(false), m_descriptor(f.m_descriptor) {}

File::File(File const *const pf)
    : H5File(*pf), m_filename(pf->m_filename), m_access(pf->m_access), m_path(pf->m_path), m_current(pf->m_current),
      m_close_handle(false), m_descriptor(pf->m_descriptor) {}

File::File(std::shared_ptr<File> pf)
    : H5File(*pf), m_filename(pf->m_filename), m_access(pf->m_access), m_path(pf->m_path), m_current(pf->m_current),
      m_close_handle(false), m_descriptor(pf->m_descriptor) {}

File &File::operator=(File const &f) {
  if (this == &f) {
  } else {
    this->m_filename = f.m_filename;
    this->m_access = f.m_access;
    this->m_path = f.m_path;
    this->m_current = f.m_current;
    this->m_close_handle = f.m_close_handle;
    this->m_descriptor = f.m_descriptor;
  }
  return *this;
}

// deconstructor

// deconstructor

File::~File() {
  if (this->m_close_handle) {
    H5::H5File::close();
  }
}

void File::close() { H5::H5File::close(); }

void File::flush(H5F_scope_t scope) const { H5::H5File::flush(scope); }

H5::H5File File::getRoot() {
  // return std::dynamic_pointer_cast<H5::Group>(std::make_shared<::NeXus::File>(this));
  // H5::H5File *cpy = dynamic_cast<H5::H5File *>(this);
  // return *cpy;
  return *this;
}

namespace {
// methods for better exception messages
template <typename T> std::string H5TypeAsString() { return "unknown"; }
template <> std::string H5TypeAsString<H5::H5Object>() { return "H5Object"; }
template <> std::string H5TypeAsString<H5::Group>() { return "Group"; }
template <> std::string H5TypeAsString<H5::DataSet>() { return "DataSet"; }
template <> std::string H5TypeAsString<H5::H5File>() { return "H5File"; }
} // namespace

/** Return a pointer corresponding to current location in file stack,
 * cast to pointer of indicated type.
 */
template <typename T> std::shared_ptr<T> File::getCurrentLocationAs() {
  std::shared_ptr<T> loc;
  if (m_path == "/") {
    std::shared_ptr<H5::H5File> proot = std::make_shared<H5::H5File>(getRoot().getId());
    loc = std::dynamic_pointer_cast<T>(proot);
  } else {
    loc = std::dynamic_pointer_cast<T>(m_current);
  }
  if (loc.get() == nullptr) {
    std::stringstream msg;
    msg << "Could not cast current location to needed H5Cpp type (requested=" << H5TypeAsString<T>() << ") at "
        << getPath();
    throw NXEXCEPTION(msg.str());
  }
  return loc;
}

/** Verify that the class name attribute set on the group
 *  matches the class name being looked up.
 */
bool File::verifyGroupClass(H5::Group const &grp, std::string const &class_name) const {
  return H5Util::keyHasValue(grp, group_class_spec, class_name);
}

bool File::hasPath(std::string const &name) {
  bool ret = false;
  try {
    std::string path = formAbsolutePath(name);
    ret = m_descriptor.isEntry(path);
  } catch (...) {
    ret = false;
  }
  return ret;
}

bool File::hasGroup(std::string const &name, std::string const &class_type) {
  bool ret = false;
  try {
    std::string path = formAbsolutePath(name);
    ret = m_descriptor.isEntry(path, class_type);
  } catch (...) {
    ret = false;
  }
  return ret;
}

bool File::hasData(std::string const &name) {
  bool ret = false;
  try {
    std::string path = formAbsolutePath(name);
    ret = m_descriptor.isEntry(path, scientific_data_set);
  } catch (...) {
    ret = false;
  }
  return ret;
}

std::filesystem::path File::formAbsolutePath(std::string const &name) {
  // try forming path relative to current location
  std::string new_name(name);
  if (new_name.ends_with('/')) {
    new_name.pop_back();
  }
  std::filesystem::path new_path = std::filesystem::path(new_name).lexically_normal();
  if (new_name.starts_with('/')) {
    // the path is already absolute
  } else {
    // if not already absolute path, make relative to current location
    new_path = (m_path / new_name).lexically_normal();
    if (!m_descriptor.isEntry(new_path)) {
      // else, try making path relative to parent
      new_path = (m_path.parent_path() / new_name).lexically_normal();
      if (!m_descriptor.isEntry(new_path)) {
        // else, try making relative to root
        new_path = std::filesystem::path("/" + new_name).lexically_normal();
        if (!m_descriptor.isEntry(new_path)) {
          // else, the name does not exist: throw an error
          throw NXEXCEPTION("Path error: " + new_name + " cannot be opened from " + m_path.string());
        }
      }
    }
  }
  return new_path;
}

void File::registerEntry(std::string const &path, std::string const &name) {
  if (path.front() != '/') {
    throw NXEXCEPTION("Paths must be absolute: " + path);
  } else if (!this->nameExists(path)) {
    throw NXEXCEPTION("Attempt to register non-existent entry: " + path + " | " + name);
  } else {
    m_descriptor.addEntry(path, name);
  }
}

void File::makeGroup(std::string const &name, std::string const &class_name, bool open_group) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name");
  }
  if (class_name.empty()) {
    throw NXEXCEPTION("Supplied empty class name");
  }
  // make the group
  std::shared_ptr<H5::Group> current = this->getCurrentLocationAs<H5::Group>();
  H5::Group newGrp = H5Util::createGroupNXS(*current, name, class_name);
  registerEntry(m_path / name, class_name);
  if (open_group) {
    m_path /= name;
    m_current = std::make_shared<H5::Group>(newGrp);
  }
}

void File::openGroup(std::string const &name, std::string const &class_name) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name");
  }
  if (class_name.empty()) {
    throw NXEXCEPTION("Supplied empty class name");
  }
  std::filesystem::path new_path = formAbsolutePath(name);
  if (new_path == m_path) {
    return;
  }
  if (m_descriptor.isEntry(new_path, class_name)) {
    H5::Group grp = H5::H5File::openGroup(new_path);
    if (!verifyGroupClass(grp, class_name)) {
      throw NXEXCEPTION("Invalid group class=" + class_name + " name=" + name);
    } else {
      m_path = new_path;
      m_current = std::make_shared<H5::Group>(grp);
    }
  } else {
    throw NXEXCEPTION("The supplied group name=" + name + " does not exist");
  }
}

void File::openPath(std::string const &pathname) {
  if (pathname.empty()) {
    throw NXEXCEPTION("Supplied empty path");
  } else if (pathname == m_path) {
    return;
  } else if (pathname == "/") {
    m_path = pathname;
    m_current = nullptr;
  } else {
    std::filesystem::path new_path = formAbsolutePath(pathname);
    if (m_descriptor.isEntry(new_path)) {
      // if the path exists:
      // -- check the type of the entry, Group or DataSet
      // -- open with appropriate method
      if (m_descriptor.isEntry(new_path, scientific_data_set)) {
        m_current = std::make_shared<H5::DataSet>(H5::H5File::openDataSet(new_path));
      } else {
        m_current = std::make_shared<H5::Group>(H5::H5File::openGroup(new_path));
      }
      m_path = new_path;
    } else {
      throw NXEXCEPTION("Attempted to open invalid path: " + new_path.string() + " from " + m_path.string());
    }
  }
}

void File::openGroupPath(std::string const &pathname) {
  if (pathname.empty()) {
    throw NXEXCEPTION("Supplied empty path");
  }
  std::filesystem::path new_path = formAbsolutePath(pathname);
  if (new_path == m_path) {
    return;
  }
  if (new_path == "/") {
    m_path = new_path;
    m_current = nullptr;
  }
  if (m_descriptor.isEntry(new_path)) {
    // if this refers to an SDS, open the parent group
    // otherwise, this is a group: open it
    if (m_descriptor.isEntry(new_path, scientific_data_set)) {
      new_path = new_path.parent_path();
    }
    m_current = std::make_shared<H5::Group>(H5::H5File::openGroup(new_path));
    m_path = new_path;
  } else {
    throw NXEXCEPTION("Attempted to open invalid path: " + new_path.string());
  }
}

string File::getPath() { return m_path; }

void File::closeGroup() {
  if (m_path == "/") {
    // do nothing in the root -- this preserves behavior from napi
    return;
  } else {
    try {
      std::shared_ptr<H5::Group> grp = this->getCurrentLocationAs<H5::Group>();
      grp->close();
      m_path = m_path.parent_path();
      if (m_path == "/") {
        m_current = nullptr;
      } else {
        m_current = std::make_shared<H5::Group>(H5::H5File::openGroup(m_path));
      }
    } catch (...) {
    }
  }
}

void File::makeData(std::string const &name, NXnumtype datatype, DimVector const &dims, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty label");
  }
  if (dims.empty()) {
    throw NXEXCEPTION("Supplied empty dimensions");
  }
  // ensure we are not at root -- NeXus should not allow datsets at root
  if (m_path == "/") {
    throw NXEXCEPTION("Cannot create dataset at root level in NeXus");
  }

  // ensure we are in a group -- we cannot make data inside a dataset
  std::shared_ptr<H5::Group> current = this->getCurrentLocationAs<H5::Group>();

  // check if any dimension is unlimited
  bool unlimited = std::any_of(dims.cbegin(), dims.cend(), [](dimsize_t const x) -> bool { return x == NX_UNLIMITED; });

  // if no unlimited dimensions, use normal
  if (!unlimited) {
    // make the data set
    H5::DataSpace ds((int)dims.size(), toDimArray(dims).data());
    try {
      H5::DataSet data = current->createDataSet(name, nxToHDF5Type(datatype), ds);
      registerEntry(m_path / name, scientific_data_set);
      if (open_data) {
        m_path /= name;
        m_current = std::make_shared<H5::DataSet>(data);
      }
    } catch (...) {
      throw NXEXCEPTION("Datasets cannot be created at current location: " + getPath());
    }
  } else {
    // farm out to makeCompData
    try {
      DimSizeVector chunk(dims.cbegin(), dims.cend());
      for (std::size_t i = 0; i < dims.size(); i++) {
        if (dims[i] == NX_UNLIMITED)
          chunk[i] = 1;
      }
      this->makeCompData(name, datatype, dims, NXcompression::NONE, chunk, open_data);
    } catch (...) {
      throw NXEXCEPTION("Datasets cannot be created at current location");
    }
  }
}

void File::makeData(const string &name, const NXnumtype type, const dimsize_t length, bool open_data) {
  this->makeData(name, type, DimVector({length}), open_data);
}

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
  DimVector dims(1, static_cast<dimsize_t>(value.size()));
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

void File::makeCompData(std::string const &name, NXnumtype const type, DimVector const &dims, NXcompression comp,
                        DimSizeVector const &chunk, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name");
  }
  if (dims.empty()) {
    throw NXEXCEPTION("Supplied empty dimensions name=" + name);
  }
  if (chunk.empty()) {
    throw NXEXCEPTION("Supplied empty bufsize name=" + name);
  }
  if (dims.size() != chunk.size()) {
    stringstream msg;
    msg << "Supplied dims rank=" << dims.size() << " must match supplied bufsize rank=" << chunk.size()
        << " name=" << name;
    throw NXEXCEPTION(msg.str());
  }
  std::set<NXcompression> const supported_comp{NXcompression::LZW, NXcompression::CHUNK, NXcompression::NONE};
  if (supported_comp.count(comp) == 0) {
    std::cerr << "HDF5 doesn't support selected compression method " << int(comp) << "!  Using NONE.\n";
    comp = NXcompression::NONE;
  }
  // ensure current location is a group
  std::shared_ptr<H5::Group> current = this->getCurrentLocationAs<H5::Group>();

  // check if any data is unlimited
  bool unlimited = std::any_of(dims.cbegin(), dims.cend(), [](auto x) -> bool { return x == NX_UNLIMITED; });

  // set the dimensions for use
  std::size_t rank = dims.size();
  DimArray mydims{0}, maxdims{0};
  DimArray chunkdims = toDimArray(chunk);
  mydims = maxdims = toDimArray(dims);
  // handle unlimited data
  if (unlimited) {
    for (std::size_t i = 0; i < rank; i++) {
      if (dims[i] == NX_UNLIMITED) {
        mydims[i] = 1;
        maxdims[i] = H5S_UNLIMITED;
      }
    }
  }

  H5::DataType datatype = nxToHDF5Type(type);

  // handle char data
  if (type == NXnumtype::CHAR) {
    std::size_t byte_zahl = (std::size_t)dims.back();
    if (unlimited) {
      mydims.front() = 1;
      maxdims.front() = H5S_UNLIMITED;
    }
    if (mydims[rank - 1] > 1) {
      maxdims[rank - 1] = 1;
    }
    mydims[rank - 1] = 1;
    chunkdims[rank - 1] = 1;
    H5Tset_size(datatype.getId(), byte_zahl);
  }

  // create a dataspace
  H5::DataSpace dataspace((int)rank, mydims.data(), maxdims.data());

  // set the compression parameters
  H5::DSetCreatPropList cparms(H5P_DATASET_CREATE);
  if (comp == NXcompression::LZW) {
    cparms.setChunk((int)rank, chunkdims.data());
    cparms.setShuffle();
    cparms.setDeflate(default_deflate_level);
  }
  // NOTE if compression is NONE but a dimension is unlimited,
  // then it still compresses by CHUNK.
  // this behavior is inherited from napi
  else if (comp == NXcompression::CHUNK || unlimited) {
    cparms.setChunk((int)rank, chunkdims.data());
  } else {
    cparms = H5::DSetCreatPropList::DEFAULT;
  }

  // create the dataset with the compression parameters
  H5::DataSet dataset = current->createDataSet(name, datatype, dataspace, cparms);
  registerEntry(m_path / name, scientific_data_set);

  if (unlimited) {
    dataset.extend(mydims.data());
  }
  if (open_data) {
    this->openData(name);
  }
}

template <typename NumT>
void File::writeCompData(std::string const &name, vector<NumT> const &value, DimVector const &dims,
                         NXcompression const comp, DimSizeVector const &bufsize) {
  this->makeCompData(name, getType<NumT>(), dims, comp, bufsize, true);
  this->putData(value);
  this->closeData();
}

void File::openData(std::string const &name) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name");
  }
  // napi used to allow opening datasets at same level without closing previous
  // many client codes follow this (bad) usage, therefore it needs to be enabled
  std::filesystem::path new_path = formAbsolutePath(name);
  if (m_descriptor.isEntry(new_path, scientific_data_set)) {
    m_path = new_path;
    m_current = std::make_shared<H5::DataSet>(this->openDataSet(m_path));
  } else {
    throw NXEXCEPTION("The indicated dataset does not exist name=" + name);
  }
  // if (this->isDataSetOpen()) {
  //   std::cerr << "Warning: Previous dataset unclosed before opening another. "
  //             << "Current dataset will be closed and requested data found in parent group.\n";
  //   this->closeData();
  // }
}

void File::closeData() {
  if (m_path == "/") {
    // do nothing in the root -- this preserves behavior from napi
    return;
  } else if (!isDataSetOpen()) {
    throw NXEXCEPTION("Attempting to close a data set while within group at " + getPath() + " of class " +
                      m_descriptor.classTypeForName(m_path));
  } else {
    try {
      std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
      dataset->close();
      m_path = m_path.parent_path();
      if (m_path == "/") {
        m_current = nullptr;
      } else {
        m_current = std::make_shared<H5::Group>(H5::H5File::openGroup(m_path));
      }
    } catch (...) {
    }
  }
}

template <typename NumT> void File::putData(NumT const *data) {
  if (data == NULL) {
    throw NXEXCEPTION("Data specified as null");
  }

  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
  DimArray dims{0}, maxdims{0}, start{0}, size{0};

  auto ds = dataset->getSpace();
  auto rank = ds.getSimpleExtentNdims();
  bool unlimited = false;
  ds.getSimpleExtentDims(dims.data(), maxdims.data());

  for (int i = 0; i < rank; i++) {
    if (maxdims[i] == H5S_UNLIMITED) {
      unlimited = true;
      start[i] = static_cast<hsize_t>(dims[i] + 1);
      size[i] = 1;
    } else {
      start[i] = 0;
      size[i] = static_cast<hsize_t>(dims[i]);
    }
  }
  if (unlimited) {
    DimSizeVector slab_start(rank), slab_size(rank);
    for (int i = 0; i < rank; i++) {
      slab_start[i] = start[i];
      slab_size[i] = size[i];
    }
    this->putSlab(data, slab_start, slab_size);
  } else {
    try {
      dataset->write(data, H5Util::getType<NumT>());
    } catch (...) {
      throw NXEXCEPTION("Failed to write data");
    }
  }
}

template <typename NumT> void File::putData(vector<NumT> const &data) {
  if (data.empty()) {
    throw NXEXCEPTION("Supplied empty data");
  }
  this->putData<NumT>(data.data());
}

void File::putData(std::string const &data) { this->putData(data.c_str()); }

template <> MANTID_NEXUS_DLL void File::putData<std::string>(std::string const *data) { this->putData(data->c_str()); }

template <typename NumT> void File::putAttr(std::string const &name, NumT const &value) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name to putAttr");
  }

  std::shared_ptr<H5::H5Object> current = this->getCurrentLocationAs<H5::H5Object>();
  H5Util::writeNumAttribute<NumT>(*current, name, value);
}

void File::putAttr(std::string const &name, std::string const &value, bool const empty_add_space) {
  if (name.empty()) {
    throw NXEXCEPTION("Supplied empty name");
  }
  std::string my_value(value);
  if (my_value.empty() && empty_add_space) {
    my_value = " "; // Make a default "space" to avoid errors.
  }
  std::shared_ptr<H5::H5Object> current = this->getCurrentLocationAs<H5::H5Object>();
  H5Util::writeStrAttribute(*current, name, my_value);
}

// this will handle string literals, which is the preferred way to pass string attributes
void File::putAttr(std::string const &name, char const *value) {
  if (value == nullptr) {
    throw NXEXCEPTION("cannot write null data to attribute " + name);
  } else {
    putAttr(name, std::string(value), false);
  }
}

template <typename NumT>
void File::putSlab(NumT const *const data, const DimSizeVector &start, const DimSizeVector &size) {
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
  // check if there is a dataset open
  std::shared_ptr<H5::DataSet> iCurrentD = this->getCurrentLocationAs<H5::DataSet>();

  // get the datatype, dataspace, and dimensions
  auto iCurrentT = iCurrentD->getDataType();
  auto iCurrentS = iCurrentD->getSpace();
  auto rank = iCurrentS.getSimpleExtentNdims();
  DimArray dims{0}, maxdims{0};
  iCurrentS.getSimpleExtentDims(dims.data(), maxdims.data());

  // copy input dimension vectors into dimension arrays
  DimArray myStart = toDimArray(start);
  DimArray mySize = toDimArray(size);
  DimArray mSize{0};
  std::transform(start.cbegin(), start.cend(), size.cbegin(), mSize.begin(), std::plus<hsize_t>());
  bool unlimiteddim =
      std::any_of(maxdims.cbegin(), maxdims.cend(), [](hsize_t const x) -> bool { return x == H5S_UNLIMITED; });

  // strings have one less dimension than otherwise indicated
  if (iCurrentT.getClass() == H5T_STRING) {
    mySize.back() = 1;
    myStart.back() = 0;
    mSize.back() = 1;
  }

  // make a new dataspace for reasons -- good reasons, most assuredly
  H5::DataSpace dataspace(rank, mySize.data(), NULL);

  // if we have unlimited data in any dimension
  if (unlimiteddim) {
    // in each dimension, set the size to the max of mSize or dims
    std::transform(mSize.cbegin(), mSize.cend(), dims.cbegin(), mSize.begin(),
                   [](hsize_t const a, hsize_t const b) -> hsize_t { return std::max(a, b); });

    // extend the slab by the new dimensions
    iCurrentD->extend(mSize.data());

    // filespace
    auto filespace = iCurrentD->getSpace();

    // define slab
    filespace.selectHyperslab(H5S_SELECT_SET, mySize.data(), myStart.data(), NULL, NULL);
    // write slab
    iCurrentD->write(data, iCurrentT, dataspace, filespace, H5::DSetMemXferPropList::DEFAULT);
    // close
    iCurrentS.close();
    // update with new size
    // TODO pointer magic to replace pFile->iCurrentS = filespace;
    iCurrentS = filespace; // ???
  } else {
    // define slab
    iCurrentS.selectHyperslab(H5S_SELECT_SET, mySize.data(), myStart.data(), NULL, NULL);
    // write slab
    iCurrentD->write(data, iCurrentT, dataspace, iCurrentS, H5::DSetMemXferPropList::DEFAULT);
  }

  dataspace.close();
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

NXlink File::getDataID() {
  // make sure current location is a dataset
  std::shared_ptr<H5::DataSet> current = this->getCurrentLocationAs<H5::DataSet>();

  /*
    this means: if the item is already linked: use the target attribute;
    else, use the path to the current node
  */
  NXlink link;
  try {
    link.targetPath = this->getAttr<std::string>(target_attr_name);
  } catch (Exception &) {
    link.targetPath = current->getObjName();
  }
  link.linkType = NXentrytype::sds;
  return link;
}

bool File::isDataSetOpen() { return m_descriptor.isEntry(m_path, scientific_data_set); }
/*----------------------------------------------------------------------*/

void File::makeLink(NXlink &link) {
  // construct a path to the target
  std::shared_ptr<H5::H5Object> current = this->getCurrentLocationAs<H5::H5Object>();
  std::filesystem::path linkTarget(m_path);
  linkTarget /= std::filesystem::path(link.targetPath).stem();

  // create link
  current->link(link.targetPath, H5L_SAME_LOC, linkTarget);
  registerEntry(linkTarget, m_descriptor.classTypeForName(link.targetPath));
  // set a target attribute on the target
  std::string here = this->getPath();
  this->openPath(linkTarget);
  this->putAttr(target_attr_name, link.targetPath);
  this->openPath(here);
}

template <typename NumT> void File::getData(NumT *const data) {
  if (data == NULL) {
    throw NXEXCEPTION("Supplied null pointer to write data to");
  }

  // make sure this is a data set
  std::shared_ptr<H5::DataSet> iCurrentD = this->getCurrentLocationAs<H5::DataSet>();
  H5::DataType iCurrentT = iCurrentD->getDataType();

  // make sure the data has the correct type
  if (hdf5ToNXType(iCurrentT) != getType<NumT>()) {
    std::stringstream msg;
    msg << "inconsistent NXnumtype file datatype=" << hdf5ToNXType(iCurrentT)
        << " supplied datatype=" << getType<NumT>();
    throw NXEXCEPTION(msg.str());
  }
  // // make sure this is not a string
  // if (iCurrentT.getClass() == H5T_STRING) {
  //   throw Exception("NeXusFile::getData() failed -- attemtping to read string in non-string method", m_filename);
  // }

  // now try to read
  try {
    H5::DataSpace iCurrentS = iCurrentD->getSpace();
    int rank = iCurrentS.getSimpleExtentNdims();
    if (rank == 0) { /* SCALAR dataset*/
      H5::DataSpace memtype_id(H5S_SCALAR);
      iCurrentS.selectAll();
      iCurrentD->read(data, iCurrentT, memtype_id, iCurrentS);
    } else {
      iCurrentD->read(data, iCurrentT);
    }
  } catch (...) {
    throw NXEXCEPTION("Failed to get data");
  }
}

template <> MANTID_NEXUS_DLL void File::getData<std::string>(std::string *const data) {
  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5::DataType datatype = dataset->getDataType();
  H5T_class_t typeclass = datatype.getClass();
  if (datatype != H5::PredType::NATIVE_CHAR && typeclass != H5T_STRING) {
    stringstream msg;
    msg << "Cannot use on-character data. Found type=" << typeclass;
    throw NXEXCEPTION(msg.str());
  }
  H5::DataSpace dataspace = dataset->getSpace();
  int rank = dataspace.getSimpleExtentNdims();
  if (rank > 1) {
    stringstream msg;
    msg << "Only understand rank=1 data. Found rank=" << rank;
    throw NXEXCEPTION(msg.str());
  }
  *data = H5Util::readString(*dataset);
}

template <typename NumT> void File::getData(vector<NumT> &data) {
  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5Util::readArray1DCoerce(*dataset, data);
}

template <> MANTID_NEXUS_DLL void File::getData<std::string>(std::vector<std::string> &data) {
  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
  std::vector<std::string> res = H5Util::readStringVector(*dataset);
  data.assign(res.cbegin(), res.cend());
}

std::string File::getStrData() {
  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
  const std::string value = H5Util::readString(*dataset);
  if (value == " ") {
    return "";
  } else {
    return value;
  }
}

template <typename NumT> void File::getDataCoerce(vector<NumT> &data) {
  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5Util::readArray1DCoerce(*dataset, data);
}

template <typename NumT> void File::getSlab(NumT *const data, const DimSizeVector &start, const DimSizeVector &size) {
  if (data == NULL) {
    throw NXEXCEPTION("Supplied null pointer for data");
  }
  if (start.size() == 0) {
    stringstream msg;
    msg << "Supplied empty start offset, rank = " << start.size();
    throw NXEXCEPTION(msg.str());
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "start rank=" << start.size() << " must match size rank=" << size.size();
    throw NXEXCEPTION(msg.str());
  }

  // check if there is a dataset open
  std::shared_ptr<H5::DataSet> iCurrentD = this->getCurrentLocationAs<H5::DataSet>();

  // get the datatype, dataspace, and dimensions
  auto iCurrentT = iCurrentD->getDataType();
  auto iCurrentS = iCurrentD->getSpace();
  auto rank = iCurrentS.getSimpleExtentNdims();

  // check the class
  H5::DataType memtype_id = H5Util::getType<NumT>();

  if (rank == 0) {
    // this is an unslabbable SCALAR
    H5::DataSpace filespace = iCurrentD->getSpace();
    H5::DataSpace memspace(H5S_SCALAR);
    filespace.selectAll();
    iCurrentD->read(data, memtype_id, memspace, filespace, H5::DSetMemXferPropList::DEFAULT);
    filespace.close();
  } else {
    DimArray mySize = toDimArray(size), myStart = toDimArray(start), mStart{0};
    iCurrentS.selectHyperslab(H5S_SELECT_SET, mySize.data(), myStart.data(), NULL, NULL);
    auto memspace = H5::DataSpace(rank, mySize.data(), NULL);
    memspace.selectHyperslab(H5S_SELECT_SET, mySize.data(), mStart.data(), NULL, NULL);
    iCurrentD->read(data, memtype_id, memspace, iCurrentS, H5::DSetMemXferPropList::DEFAULT);
  }
}

template <>
MANTID_NEXUS_DLL void File::getSlab<std::string>(std::string *const data, const DimSizeVector &start,
                                                 const DimSizeVector &size) {
  if (data == NULL) {
    throw NXEXCEPTION("Supplied null pointer for data");
  }
  if (start.size() == 0) {
    stringstream msg;
    msg << "Supplied empty start offset, rank = " << start.size();
    throw NXEXCEPTION(msg.str());
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "start rank=" << start.size() << " must match size rank=" << size.size();
    throw NXEXCEPTION(msg.str());
  }

  // check if there is a dataset open
  std::shared_ptr<H5::DataSet> iCurrentD = this->getCurrentLocationAs<H5::DataSet>();

  // get the datatype, dataspace, and dimensions
  auto iCurrentT = iCurrentD->getDataType();
  auto iCurrentS = iCurrentD->getSpace();
  auto rank = iCurrentS.getSimpleExtentNdims();

  // check the class
  H5::DataType memtype_id = H5Util::getType<string>();

  if (rank == 0) {
    // this is an unslabbable SCALAR
    H5::DataSpace filespace = iCurrentD->getSpace();
    H5::DataSpace memspace(H5S_SCALAR);
    filespace.selectAll();
    iCurrentD->read(data, memtype_id, memspace, filespace, H5::DSetMemXferPropList::DEFAULT);
    filespace.close();
  } else {
    DimArray mySize = toDimArray(size), mStart{0};
    if (mySize[0] == 1) {
      mySize[0] = iCurrentT.getSize();
    }
    // char *tmp_data = new char[mySize[0]]{0};
    iCurrentS.selectHyperslab(H5S_SELECT_SET, mySize.data(), mStart.data(), NULL, NULL);
    iCurrentD->read(data, iCurrentT, H5S_ALL, H5S_ALL, H5::DSetMemXferPropList::DEFAULT);
    // char const *data1 = tmp_data + start[0];
    // strncpy(static_cast<char *>(data), data1, (size_t)size[0]);
    // delete[] tmp_data;
  }
}

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

bool File::isDataInt() {
  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5::DataType datatype = dataset->getDataType();
  return datatype.getClass() == H5T_INTEGER;
}

Info File::getInfo() {
  // ensure current location is a dataset
  std::shared_ptr<H5::DataSet> dataset = this->getCurrentLocationAs<H5::DataSet>();

  // get the datatype
  auto dt = dataset->getDataType();
  auto ds = dataset->getSpace();
  std::size_t rank = ds.getSimpleExtentNdims();
  DimArray dims{0};
  ds.getSimpleExtentDims(dims.data());

  // strings need the length of the string hiding in there too
  // this doesn't work for variable length strings
  if (hdf5ToNXType(dt) == NXnumtype::CHAR) {
    if (dt.isVariableStr()) {
      throw Exception("Do not have implementation for variable length strings", "getInfo", m_filename);
    }
    dims[rank - 1] = dataset->getStorageSize();
  }

  Info info;
  info.type = hdf5ToNXType(dt);
  info.dims.push_back(dims.front());
  for (std::size_t i = 1; i < rank; i++) {
    info.dims.push_back(dims[i]);
  }
  return info;
}

Entries File::getEntries() {
  Entries result;
  this->getEntries(result);
  return result;
}

void File::getEntries(Entries &result) {
  result.clear();
  auto current = this->getCurrentLocationAs<H5::Group>();
  for (size_t i = 0; i < current->getNumObjs(); i++) {
    std::string name = current->getObjnameByIdx(i), className;
    H5G_obj_t type = current->getObjTypeByIdx(i);
    if (type == H5G_GROUP) {
      H5::Group grp = current->openGroup(name);
      H5::Attribute attr = grp.openAttribute(group_class_spec);
      attr.read(attr.getDataType(), className);
    } else if (type == H5G_DATASET) {
      className = "SDS";
    }
    result[name] = className;
  }
}

void File::getEntryDirectory(Entries &result) {
  result.clear();
  recursivePopulateEntries(getRoot(), result);
}

std::string File::getTopLevelEntryName() {
  std::string top("");

  // look for first group off of root of class NXentry
  std::size_t firstGrp = 0;
  for (; firstGrp < this->getNumObjs(); firstGrp++) {
    if (this->getObjTypeByIdx(firstGrp) == H5G_GROUP) {
      top = this->getObjnameByIdx(firstGrp);
      H5::Group grp = H5::H5File::openGroup(top);
      if (this->verifyGroupClass(grp, "NXentry")) {
        break;
      } else {
        top = "";
      }
    }
  }
  if (top.empty()) {
    throw NXEXCEPTION("unable to find top-level entry, no valid groups");
  }
  return top;
}

template <typename NumT> void File::getAttr(std::string const &name, NumT &value) {
  // verify the current location can hold an attribute, and has the attribute named
  std::shared_ptr<H5::H5Object const> const current = this->getCurrentLocationAs<H5::H5Object>();
  if (!current->attrExists(name)) {
    throw NXEXCEPTION("Could not find attribute " + name + " at " + m_path.string());
  }

  // now open the attribute, read it, and close
  H5::Attribute attr = current->openAttribute(name);
  auto dt = attr.getDataType();
  attr.read(dt, &value);
  attr.close();
}

// for string case, use H5::Attribute::read(const DataType &mem_type, H5std_string &strg)
template <> MANTID_NEXUS_DLL void File::getAttr(std::string const &name, std::string &value) {
  // verify the current location can hold an attribute, and has the attribute named
  std::shared_ptr<H5::H5Object> current = this->getCurrentLocationAs<H5::H5Object>();
  if (!current->attrExists(name)) {
    throw NXEXCEPTION("Could not find string attribute " + name + " at " + m_path.string());
  }

  // open the attribute, and read it
  H5::Attribute attr = current->openAttribute(name);
  attr.read(attr.getDataType(), value);
  attr.close();
}

template <typename NumT> NumT File::getAttr(std::string const &name) {
  NumT ret;
  this->getAttr<NumT>(name, ret);
  return ret;
}

std::vector<AttrInfo> File::getAttrInfos() {
  std::shared_ptr<H5::H5Object> current = this->getCurrentLocationAs<H5::H5Object>();
  int num_attr = current->getNumAttrs();
  std::vector<AttrInfo> infos;
  infos.reserve(num_attr);
  for (int i = 0; i < num_attr; i++) {
    H5::Attribute attr = current->openAttribute(i);
    std::string name = attr.getName();
    // do not include the group class spec, as that is for internal use only
    if (name == group_class_spec) {
      continue;
    }
    NXnumtype type = hdf5ToNXType(attr.getDataType());
    // all of this is just to set the length...
    auto ds = attr.getSpace();
    int rank = ds.getSimpleExtentNdims();
    // TODO - AttrInfo cannot handle more complex ranks/dimensions, we need to throw an error
    if (rank > 2 || (rank == 2 && type != NXnumtype::CHAR)) {
      throw Exception("ERROR iterating through attributes found array attribute not understood by this api",
                      "getAttrInfos", m_filename);
    }
    DimArray dims{0};
    ds.getSimpleExtentDims(dims.data());
    std::size_t length = 1;
    for (int d = 0; d < rank; ++d) {
      length *= dims[d];
    }
    // now add attribute info to vector
    infos.emplace_back(type, length, name);
    attr.close();
  }
  return infos;
}

std::set<std::string> File::getAttrNames() {
  std::shared_ptr<H5::H5Object> current = this->getCurrentLocationAs<H5::H5Object>();
  int num_attr = current->getNumAttrs();
  std::set<std::string> infos;
  for (int i = 0; i < num_attr; i++) {
    H5::Attribute attr = current->openAttribute(i);
    std::string name = attr.getName();
    // do not include the group class spec, as that is for internal use only
    if (name == group_class_spec) {
      continue;
    }
    infos.insert(name);
    attr.close();
  }
  return infos;
}

bool File::hasAttr(std::string const &name) {
  std::shared_ptr<H5::H5Object> current = this->getCurrentLocationAs<H5::H5Object>();
  return current->attrExists(name);
}

NXlink File::getGroupID() {
  // make sure current location is a group
  std::shared_ptr<H5::Group> current = this->getCurrentLocationAs<H5::Group>();

  /*
    this means: if the item is already linked: use the target attribute;
    else, use the path to the current node
  */
  NXlink link;
  if (current->attrExists(target_attr_name)) {
    link.targetPath = this->getAttr<std::string>(target_attr_name);
  } else {
    link.targetPath = current->getObjName();
  }
  link.linkType = NXentrytype::group;
  return link;
}

} // namespace NeXus

// -------------------------- NXnumtype ----------------------------------------------------------------------------//

int NXnumtype::validate_val(int const x) {
  int val = BAD;
  if ((x == FLOAT32) || (x == FLOAT64) || (x == INT8) || (x == UINT8) || (x == BOOLEAN) || (x == INT16) ||
      (x == UINT16) || (x == INT32) || (x == UINT32) || (x == INT64) || (x == UINT64) || (x == CHAR) || (x == BINARY) ||
      (x == BAD)) {
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
  return ret;
}

std::ostream &operator<<(std::ostream &os, const NXnumtype &value) {
  os << std::string(value);
  return os;
}

/* ---------------------------------------------------------------- */
/* Concrete instantiations of template definitions.                 */
/* ---------------------------------------------------------------- */

template MANTID_NEXUS_DLL std::shared_ptr<H5::H5File> File::getCurrentLocationAs();

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
template MANTID_NEXUS_DLL void File::getData(std::string *data);

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

template MANTID_NEXUS_DLL void File::getDataCoerce(vector<int> &data);
template MANTID_NEXUS_DLL void File::getDataCoerce(vector<double> &data);

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
