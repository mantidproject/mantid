#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NeXusException.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sstream>
#include <typeinfo>

#define LOG_ERROR(func)                                                                                                \
  printf("NeXusFile %s L%d %s\n", func, __LINE__, this->getPath().c_str());                                            \
  fflush(stdout);

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

/**
 * \file NeXusFile.cpp
 * The implementation of the NeXus C++ API
 */

namespace { // anonymous namespace to keep it in the file

constexpr std::string group_class_spec("NX_class");
constexpr std::string target_attr_name("target");
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
      throw Exception("ERROR: hdf5ToNXtype: invalid type");
    }
  }
  return iPtype;
}

void recursivePopulateEntries(H5::Group *grp, Entries &entries) {
  // get the path and class of this group
  std::string const groupNameStr = grp->getObjName();
  std::string nxClass = "";
  if (groupNameStr != "/") {
    if (grp->attrExists(group_class_spec)) {
      H5::Attribute const attr = grp->openAttribute(group_class_spec);
      attr.read(attr.getDataType(), nxClass);
    }
  }

  if (!nxClass.empty()) {
    entries[groupNameStr] = nxClass;
  }

  // recursively grab all entries within this group
  for (hsize_t i = 0; i < grp->getNumObjs(); i++) {
    H5G_obj_t type = grp->getObjTypeByIdx(i);
    H5std_string memberName = grp->getObjnameByIdx(i);

    if (type == H5G_GROUP) {
      H5::Group subgrp = grp->openGroup(memberName);
      recursivePopulateEntries(&subgrp, entries);
    } else if (type == H5G_DATASET) {
      std::string absoluteEntryName = groupNameStr + "/" + memberName;
      entries[absoluteEntryName] = "SDS";
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
    : H5File(filename, access, H5Util::defaultFileAcc()), m_filename(filename), m_access(access), m_close_handle(true) {
  this->m_stack.push_back(nullptr);
};

File::File(char const *filename, NXaccess const access)
    : H5File(filename, access, H5Util::defaultFileAcc()), m_filename(filename), m_access(access), m_close_handle(true) {
  this->m_stack.push_back(nullptr);
};

// copy constructors

File::File(File const &f)
    : H5File(f), m_filename(f.m_filename), m_access(f.m_access), m_stack(f.m_stack), m_close_handle(false) {}

File::File(File const *const pf)
    : H5File(*pf), m_filename(pf->m_filename), m_access(pf->m_access), m_stack(pf->m_stack), m_close_handle(false) {}

File::File(std::shared_ptr<File> pf)
    : H5File(*pf), m_filename(pf->m_filename), m_access(pf->m_access), m_stack(pf->m_stack), m_close_handle(false) {}

File &File::operator=(File const &f) {
  if (this == &f) {
  } else {
    this->m_filename = f.m_filename;
    this->m_access = f.m_access;
    this->m_stack = f.m_stack;
    this->m_close_handle = f.m_close_handle;
  }
  return *this;
}

// deconstructor

// deconstructor

File::~File() {
  if (this->m_close_handle) {
    H5::H5File::close();
  }
  m_stack.clear();
}

void File::close() {
  H5::H5File::close();
  m_stack.clear();
}

void File::flush(H5F_scope_t scope) const { H5::H5File::flush(scope); }

H5::Group *File::getRoot() { return dynamic_cast<H5::Group *>(this); }

/** Return a pointer corresponding to current location in file stack. */
H5::H5Location *File::getCurrentLocation() {
  H5::H5Location *ret = m_stack.back().get();
  if (m_stack.back() == nullptr) {
    ret = this->getRoot();
  }
  return ret;
}

/** Return a pointer corresponding to current location in file stack,
 * cast to pointer of indicated type.
 */
template <typename T> T *File::getCurrentLocationAs() {
  T *loc = dynamic_cast<T *>(this->getCurrentLocation());
  if (loc == nullptr) {
    throw Exception("NeXusFile::getCurrentLocationAs -- Could not cast current location to needed H5Cpp type",
                    m_filename);
  }
  return loc;
}

/** Verify that the class name attribute set on the group
 *  matches the class name being looked up.
 */
bool File::verifyGroupClass(H5::Group const &grp, std::string const &class_name) const {
  return H5Util::keyHasValue(grp, group_class_spec, class_name);
}

void File::makeGroup(std::string const &name, std::string const &class_name, bool open_group) {
  if (name.empty()) {
    throw Exception("NeXusFile::makeGroup -- Supplied empty name to makeGroup", m_filename);
  }
  if (class_name.empty()) {
    throw Exception("NeXusFile::makeGroup -- Supplied empty class name to makeGroup", m_filename);
  }
  // make the group
  H5::Group *current = this->getCurrentLocationAs<H5::Group>();
  H5::Group newGrp = H5Util::createGroupNXS(*current, name, class_name);
  if (open_group) {
    this->m_stack.push_back(std::make_shared<H5::Group>(newGrp));
  }
}

void File::openGroup(std::string const &name, std::string const &class_name) {
  if (name.empty()) {
    throw Exception("NeXusFile::openGroup -- Supplied empty name to openGroup", m_filename);
  }
  if (class_name.empty()) {
    throw Exception("NeXusFile::openGroup -- Supplied empty class name to openGroup", m_filename);
  }
  auto current = this->getCurrentLocationAs<H5::Group>();
  if (!H5Util::groupExists(*current, name)) {
    throw Exception("NeXusFile::openGroup -- The supplied group name does not exist", m_filename);
  }
  std::shared_ptr<H5::Group> grp = std::make_shared<H5::Group>(current->openGroup(name));
  if (!verifyGroupClass(*(grp.get()), class_name)) {
    throw Exception("NeXusFile::openGroup -- Invalid group class name", m_filename);
  }
  this->m_stack.push_back(grp);
}

void File::openPath(std::string const &pathname) {
  if (pathname.empty()) {
    throw Exception("NeXusFile::openPath -- Supplied empty path to openPath", m_filename);
  }
  std::filesystem::path path(pathname);
  // if (path.is_relative()) {
  //   printf("PATH %s %s %s\n", path.c_str(), this->getPath().c_str(), path.relative_path().c_str()); fflush(stdout);
  //   path = this->getPath() / path.relative_path();
  // }
  if (!path.is_absolute()) {
    throw Exception("NeXusFile::openPath -- paths must be absolute, beginning with /", m_filename);
  }
  // create a new stack -- will replace old if opening succeeds
  std::vector<std::shared_ptr<H5::H5Location>> new_stack(1, nullptr);

  H5::H5Object *current;

  // open all entries in path iteratively
  for (auto isubpath = ++path.begin(); isubpath != path.end(); isubpath++) {
    std::string name = (*isubpath);
    // get most recent pointer on new stack
    current = dynamic_cast<H5::H5Object *>(new_stack.back().get());
    if (current == nullptr) {
      current = this;
    }
    if (!current->nameExists(name)) {
      throw Exception("NeXusFile::openPath -- invalid path element " + name + " in " + string(path) + ".", m_filename);
    }
    H5O_type_t type = current->childObjType(name);
    if (type == H5O_TYPE_GROUP) {
      if (current == this) {
        // open the top-level entry -- has to use H5::H5File openGroup
        new_stack.push_back(std::make_shared<H5::Group>(H5::H5File::openGroup(name)));
      } else {
        new_stack.push_back(std::make_shared<H5::Group>(current->openGroup(name)));
      }
    } else if (type == H5O_TYPE_DATASET) {
      new_stack.push_back(std::make_shared<H5::DataSet>(current->openDataSet(name)));
    }
  }
  // copy the new stack onto the old stack
  this->m_stack.clear();
  this->m_stack.resize(new_stack.size());
  std::copy(new_stack.cbegin(), new_stack.cend(), this->m_stack.begin());
}

void File::openGroupPath(std::string const &pathname) {
  if (pathname.empty()) {
    throw Exception("NeXusFile::openPath -- Supplied empty path to openPath", m_filename);
  }
  std::filesystem::path path(pathname);
  if (!path.is_absolute()) {
    throw Exception("NeXusFile::openPath -- paths must be absolute, beginning with /", m_filename);
  }
  // create a new stack to replace old -- will only happen if opening succeeds
  std::vector<std::shared_ptr<H5::H5Location>> new_stack(1, nullptr);

  H5::H5Object *current;

  // open all entries in path iteratively
  for (auto isubpath = ++path.begin(); isubpath != path.end(); isubpath++) {
    std::string name = (*isubpath);
    // get most recent pointer on new stack
    current = dynamic_cast<H5::H5Object *>(new_stack.back().get());
    if (current == nullptr) {
      current = this;
    }
    if (!current->nameExists(name)) {
      throw Exception("NeXusFile::openPath -- invalid path element " + name + " in " + string(path) + ".", m_filename);
    }
    H5O_type_t type = current->childObjType(name);
    if (type == H5O_TYPE_GROUP) {
      if (current == this) {
        // open the top-level entry -- has to use H5::H5File openGroup
        new_stack.push_back(std::make_shared<H5::Group>(H5::H5File::openGroup(name)));
      } else {
        new_stack.push_back(std::make_shared<H5::Group>(current->openGroup(name)));
      }
    } else {
      // only want to get groups
      break;
    }
  }
  // copy the new stack onto the old stack
  this->m_stack.clear();
  this->m_stack.resize(new_stack.size());
  std::copy(new_stack.cbegin(), new_stack.cend(), this->m_stack.begin());
}

string File::getPath() { return this->getCurrentLocationAs<H5::H5Object>()->getObjName(); }

void File::closeGroup() {
  auto loc = this->getCurrentLocation();
  if (loc == this) {
    // do nothing in the root -- this preserves behavior from napi
    return;
  } else {
    H5::Group *grp = this->getCurrentLocationAs<H5::Group>();
    grp->close();
    this->m_stack.pop_back();
  }
}

void File::makeData(std::string const &name, NXnumtype datatype, DimVector const &dims, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw Exception("NeXusFile::makeData -- Supplied empty label to makeData", m_filename);
  }
  if (dims.empty()) {
    throw Exception("NeXusFile::makeData -- Supplied empty dimensions to makeData", m_filename);
  }
  // ensure we are in a group
  H5::Group *current = this->getCurrentLocationAs<H5::Group>();
  // ensure we are not at root -- NeXus should not allow datsets at root
  if (current == this->getRoot()) {
    throw Exception("NeXusFile::makeData -- Cannot create dataset at root level in NeXus", m_filename);
  }

  // check if any dimension is unlimited
  bool unlimited = std::any_of(dims.cbegin(), dims.cend(), [](dimsize_t const x) -> bool { return x == NX_UNLIMITED; });

  // if no unlimited dimensions, use normal
  if (!unlimited) {
    // make the data set
    H5::DataSpace ds((int)dims.size(), toDimArray(dims).data());
    try {
      H5::DataSet data = current->createDataSet(name, nxToHDF5Type(datatype), ds);
      if (open_data) {
        this->m_stack.push_back(std::make_shared<H5::DataSet>(data));
      }
    } catch (...) {
      throw Exception("NeXusFile::makeData -- Datasets cannot be created at current location", m_filename);
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
      throw Exception("NeXusFile::makeData -- Datasets cannot be created at current location", m_filename);
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
    throw Exception("NeXusFile::makeCompData -- Supplied empty name to makeCompData", m_filename);
  }
  if (dims.empty()) {
    throw Exception("NeXusFile::makeCompData -- Supplied empty dimensions to makeCompData", m_filename);
  }
  if (chunk.empty()) {
    throw Exception("NeXusFile::makeCompData -- Supplied empty bufsize to makeCompData", m_filename);
  }
  if (dims.size() != chunk.size()) {
    stringstream msg;
    msg << "NeXusFile::makeCompData -- Supplied dims rank=" << dims.size()
        << " must match supplied bufsize rank=" << chunk.size() << "in makeCompData";
    throw Exception(msg.str(), m_filename);
  }
  std::vector<NXcompression> const supported_comp{NXcompression::LZW, NXcompression::CHUNK, NXcompression::NONE};
  if (std::find(supported_comp.cbegin(), supported_comp.cend(), comp) == supported_comp.cend()) {
    std::cerr << "HDF5 doesn't support selected compression method " << int(comp) << "!  Using NONE.\n";
    comp = NXcompression::NONE;
  }
  // ensure current location is a group
  H5::Group *current = this->getCurrentLocationAs<H5::Group>();

  // check if any data is unlimited
  bool unlimited = std::any_of(dims.cbegin(), dims.cend(), [](auto x) -> bool { return x == NX_UNLIMITED; });

  // set the dimensions for use
  std::size_t rank = dims.size();
  DimArray mydims, maxdims, chunkdims = toDimArray(chunk);
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

  // TODO pointer magic?
  // pFile->iCurrentD = dataset;
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
    throw Exception("NeXusFile::openData -- Supplied empty name to openData", m_filename);
  }
  auto current = this->getCurrentLocationAs<H5::H5Object>();
  try {
    H5::DataSet data = current->openDataSet(name);
    std::shared_ptr<H5::DataSet> pdata = std::make_shared<H5::DataSet>(data);
    this->m_stack.push_back(pdata);
  } catch (...) {
    // printf("OPEN DATA FAILURE: %s NOT IN %s | %d\n", name.c_str(), current->getObjName().c_str(),
    // current->attrExists(name));
    throw Exception("NeXusFile::openData -- The indicated dataset does not exist", m_filename);
  }
}

// void File::closeData() {
//   H5::DataSet *current;
//   current = this->getCurrentLocationAs<H5::DataSet>();
//   current->close();
//   this->m_stack.pop_back();
// }
void File::closeData() {
  try {
    H5::DataSet *data = this->getCurrentLocationAs<H5::DataSet>();
    data->close();
    this->m_stack.pop_back();
  } catch (...) {
    throw Exception("NeXusFile::closeData -- Object at current location is not a dataset", m_filename);
  }
}

template <typename NumT> void File::putData(NumT const *data) {
  if (data == NULL) {
    throw NXEXCEPTION("Data specified as null");
  }

  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();
  DimArray dims, maxdims, start, size;

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
      throw Exception("NeXusFile::putData -- Failed to write data\n", m_filename);
    }
  }
}

template <typename NumT> void File::putData(vector<NumT> const &data) {
  if (data.empty()) {
    throw Exception("NeXusFile::putData -- Supplied empty data to putData", m_filename);
  }
  this->putData<NumT>(data.data());
}

template <> MANTID_NEXUS_DLL void File::putData<std::string>(std::string const *data) {
  char const *chardata = data->c_str();
  this->putData(chardata);
}

template <typename NumT> void File::putAttr(std::string const &name, NumT const &value) {
  if (name.empty()) {
    throw Exception("NeXusFile::putAttr -- Supplied empty name to putAttr", m_filename);
  }

  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  H5Util::writeNumAttribute<NumT>(*current, name, value);
}

// in std::string case use H5::Attribute::write(const DataType &mem_type, const H5std_string &strg)
template <> MANTID_NEXUS_DLL void File::putAttr<std::string>(std::string const &name, std::string const &value) {
  if (name.empty()) {
    throw Exception("NeXusFile::putAttr -- Supplied empty name to putAttr", m_filename);
  }
  if (value == "") {
    throw Exception("NeXusFile::putAttr -- Supplied empty std::string value to putAttr", m_filename);
  }

  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  H5Util::writeStrAttribute(*current, name, value);
}

void File::putAttr(std::string const &name, std::string const &value, bool const empty_add_space) {
  std::string my_value(value);
  if (my_value.empty() && empty_add_space)
    my_value = " "; // Make a default "space" to avoid errors.
  this->putAttr<std::string>(name, my_value);
}

// this will handle string literals, which is the preferred way to pass string attributes
void File::putAttr(std::string const &name, char const *const value) { putAttr(name, std::string(value)); }

template <typename NumT>
void File::putSlab(NumT const *const data, const DimSizeVector &start, const DimSizeVector &size) {
  if (data == NULL) {
    throw Exception("NeXusFile::putSlab -- Data specified as null in putSlab", m_filename);
  }
  if (start.empty()) {
    throw Exception("NeXusFile::putSlab -- Supplied empty start to putSlab", m_filename);
  }
  if (size.empty()) {
    throw Exception("NeXusFile::putSlab -- Supplied empty size to putSlab", m_filename);
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "NeXusFile::putSlab -- Supplied start rank=" << start.size()
        << " must match supplied size rank=" << size.size() << "in putSlab";
    throw Exception(msg.str(), m_filename);
  }
  if (start.size() > NX_MAXRANK) {
    throw Exception("NeXusFile::putSlab -- The supplied rank exceeds the max rank for Mantid", m_filename);
  }
  // check if there is a dataset open
  auto *iCurrentD = this->getCurrentLocationAs<H5::DataSet>();

  // get the datatype, dataspace, and dimensions
  auto iCurrentT = iCurrentD->getDataType();
  auto iCurrentS = iCurrentD->getSpace();
  auto rank = iCurrentS.getSimpleExtentNdims();
  DimArray dims, maxdims;
  iCurrentS.getSimpleExtentDims(dims.data(), maxdims.data());

  // copy input dimension vectors into dimension arrays
  DimArray myStart = toDimArray(start);
  DimArray mySize = toDimArray(size);
  DimArray mSize;
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
  auto *current = this->getCurrentLocationAs<H5::DataSet>();

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

bool File::isDataSetOpen() {
  H5::DataSet const *current = dynamic_cast<H5::DataSet const *>(getCurrentLocation());
  return current != nullptr;
}
/*----------------------------------------------------------------------*/

void File::makeLink(NXlink &link) {
  // construct a path to the target
  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  std::filesystem::path linkTarget = std::string(current->getObjName());
  linkTarget /= std::filesystem::path(link.targetPath).stem();

  // create link
  current->link(link.targetPath, H5L_SAME_LOC, linkTarget);

  // set a target attribute on the target
  std::string here = this->getPath();
  this->openPath(linkTarget);
  this->putAttr(target_attr_name, link.targetPath);
  this->openPath(here);
}

template <typename NumT> void File::getData(NumT *const data) {
  if (data == NULL) {
    throw Exception("NeXusFile::getData -- Supplied null pointer to getData", m_filename);
  }

  // make sure this is a data set
  H5::DataSet *iCurrentD = this->getCurrentLocationAs<H5::DataSet>();
  H5::DataType iCurrentT = iCurrentD->getDataType();

  // make sure the data has the correct type
  if (hdf5ToNXType(iCurrentT) != getType<NumT>()) {
    throw Exception("NeXusFile::getData() failed -- inconsistent NXnumtype", m_filename);
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
    throw Exception("NeXusFile::getData() -- Failed to get data", m_filename);
  }
}

template <> MANTID_NEXUS_DLL void File::getData<std::string>(std::string *const data) {
  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5::DataType datatype = dataset->getDataType();
  H5T_class_t typeclass = datatype.getClass();
  if (datatype != H5::PredType::NATIVE_CHAR && typeclass != H5T_STRING) {
    stringstream msg;
    msg << "Cannot use File::getData<string>() on non-character data. Found type=" << typeclass;
    throw Exception(msg.str(), m_filename);
  }
  H5::DataSpace dataspace = dataset->getSpace();
  int rank = dataspace.getSimpleExtentNdims();
  if (rank > 1) {
    stringstream msg;
    msg << "File::getData<string>() only understand rank=1 data. Found rank=" << rank;
    throw Exception(msg.str(), m_filename);
  }
  *data = H5Util::readString(*dataset);
  // H5::DataSet *iCurrentD = this->getCurrentLocationAs<H5::DataSet>();
  // H5::DataType iCurrentT = iCurrentD->getDataType();
  // H5::DataSpace iCurrentS = iCurrentD->getSpace();
  // int rank = iCurrentS.getSimpleExtentNdims();
  // if (rank == 0) { /* SCALAR dataset*/
  //   if (iCurrentT.isVariableStr()) {
  //     iCurrentD->read(*data, iCurrentT);
  //   } else {
  //     H5::DataSpace memtype_id(H5S_SCALAR);
  //     iCurrentS.selectAll();
  //     iCurrentD->read(*data, iCurrentT, memtype_id, iCurrentS);
  //   }
  // } else {
  //   if (iCurrentT.isVariableStr()) {
  //     H5::DataType memtype_id(H5T_STRING, H5T_VARIABLE);
  //     iCurrentD->read(*data, memtype_id);
  //   } else {
  //     iCurrentD->read(*data, iCurrentT);
  //   }
  // }
}

template <typename NumT> void File::getData(vector<NumT> &data) {
  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5Util::readArray1DCoerce(*dataset, data);
  // Info info = this->getInfo();

  // if (info.type != getType<NumT>()) {
  //   throw Exception("NeXusFile::getData -- invalid vector type", m_filename);
  // }
  // // determine the number of elements
  // size_t length =
  //     std::accumulate(info.dims.cbegin(), info.dims.cend(), static_cast<size_t>(1),
  //                     [](auto const subtotal, auto const &value) { return subtotal * static_cast<size_t>(value);
  //                     });

  // // allocate memory to put the data into
  // // need to use resize() rather than reserve() so vector length gets set
  // data.resize(length);

  // // fetch the data
  // this->getData<NumT>(data.data());
}

std::string File::getStrData() {
  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();
  return H5Util::readString(*dataset);
}

template <typename NumT> void File::getDataCoerce(vector<NumT> &data) {
  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5Util::readArray1DCoerce(*dataset, data);
  // H5::DataType dt = dataset->getDataType();
  // NXnumtype type = hdfToNXType(dt);
  // if (type == NXnumtype::INT8) {
  //   vector<int8_t> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else if (type == NXnumtype::UINT8) {
  //   vector<uint8_t> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else if (type == NXnumtype::INT16) {
  //   vector<int16_t> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else if (type == NXnumtype::UINT16) {
  //   vector<uint16_t> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else if (type == NXnumtype::INT32) {
  //   vector<int32_t> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else if (type == NXnumtype::UINT32) {
  //   vector<uint32_t> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else if (type == NXnumtype::FLOAT32) {
  //   vector<float> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else if (type == NXnumtype::FLOAT64) {
  //   vector<double> result;
  //   this->getData(result);
  //   data.assign(result.begin(), result.end());
  // } else {
  //   throw Exception("NexusFile::getDataCoerce(): Could not coerce data.", m_filename);
  // }
}

template <typename NumT> void File::getSlab(NumT *const data, const DimSizeVector &start, const DimSizeVector &size) {
  if (data == NULL) {
    throw Exception("NeXusFile::getSlab -- Supplied null pointer to getSlab", m_filename);
  }
  if (start.size() == 0) {
    stringstream msg;
    msg << "NeXusFile::getSlab -- Supplied empty start offset, rank = " << start.size() << " in getSlab";
    throw Exception(msg.str(), m_filename);
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "NeXusFile::getSlab -- In getSlab start rank=" << start.size() << " must match size rank=" << size.size();
    throw Exception(msg.str(), m_filename);
  }

  // check if there is a dataset open
  auto *iCurrentD = this->getCurrentLocationAs<H5::DataSet>();

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
    throw Exception("NeXusFile::getSlab -- Supplied null pointer to getSlab", m_filename);
  }
  if (start.size() == 0) {
    stringstream msg;
    msg << "NeXusFile::getSlab -- Supplied empty start offset, rank = " << start.size() << " in getSlab";
    throw Exception(msg.str(), m_filename);
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "NeXusFile::getSlab -- In getSlab start rank=" << start.size() << " must match size rank=" << size.size();
    throw Exception(msg.str(), m_filename);
  }

  // check if there is a dataset open
  auto *iCurrentD = this->getCurrentLocationAs<H5::DataSet>();

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
  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();
  H5::DataType datatype = dataset->getDataType();
  return datatype.getClass() == H5T_INTEGER;
}

Info File::getInfo() {
  // ensure current location is a dataset
  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();

  // get the datatype
  auto dt = dataset->getDataType();
  auto ds = dataset->getSpace();
  std::size_t rank = ds.getSimpleExtentNdims();
  DimArray dims;
  ds.getSimpleExtentDims(dims.data());

  Info info;
  info.type = hdf5ToNXType(dt);
  info.dims.push_back(dims[0]);
  for (std::size_t i = 1; i < rank; i++) {
    // if (dt.getClass() == H5T_STRING && dims[i] == 1) {
    //   continue;
    // } else {
    info.dims.push_back(dims[i]);
    // }
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
  recursivePopulateEntries(this->getRoot(), result);
}

std::string File::getTopLevelEntryName() {
  std::string top("");

  // go to root and verify
  H5::Group *root = this->getRoot();
  if (root == nullptr) {
    throw Exception("NeXusFile: invalid file has no root", m_filename);
  }
  // look for first group of class NXentry
  std::size_t firstGrp = 0;
  for (; firstGrp < root->getNumObjs(); firstGrp++) {
    if (root->getObjTypeByIdx(firstGrp) == H5G_GROUP) {
      top = root->getObjnameByIdx(firstGrp);
      H5::Group grp = root->openGroup(top);
      if (this->verifyGroupClass(grp, "NXentry")) {
        break;
      } else {
        top = "";
      }
    }
  }
  if (top.empty()) {
    throw Exception("NeXusFile: unable to find top-level entry, no valid groups", m_filename);
  }
  return top;
}

template <typename NumT> void File::getAttr(std::string const &name, NumT &value) {
  // verify the current location can hold an attribute, and has the attribute named
  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  if (!current->attrExists(name)) {
    throw Exception("NeXusFile::getAttr -- This was not found.", m_filename);
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
  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  if (!current->attrExists(name)) {
    throw Exception("NeXusFile::getAttr -- This was not found", m_filename);
  }

  // open the attribute, and read it
  H5::Attribute attr = current->openAttribute(name);
  auto dt = attr.getDataType();
  value = "";
  attr.read(dt, value);
  if (value == "") {
    throw Exception("NeXusFile::getAttr -- Error reading string attribute", m_filename);
  }
  attr.close();
}

template <typename NumT> NumT File::getAttr(std::string const &name) {
  NumT ret;
  this->getAttr<NumT>(name, ret);
  return ret;
}

std::vector<AttrInfo> File::getAttrInfos() {
  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
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
      std::cerr << "ERROR iterating through attributes found array attribute not understood by this api\n";
      throw Exception("NeXusFile::getAttrInfos -- getNextAttr failed", m_filename);
    }
    DimArray dims;
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

bool File::hasAttr(std::string const &name) {
  auto *current = this->getCurrentLocationAs<H5::H5Object>();
  return current->attrExists(name);
}

NXlink File::getGroupID() {
  // make sure current location is a group
  auto *current = this->getCurrentLocationAs<H5::Group>();

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
