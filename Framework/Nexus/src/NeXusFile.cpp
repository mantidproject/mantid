#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NeXusException.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <numeric>
#include <sstream>
#include <typeinfo>
#include <unordered_map>

using namespace NeXus;
using std::string;
using std::stringstream;
using std::vector;

typedef std::array<size_t, 4> DimArray;

#define CALL_NAPI(status, msg)                                                                                         \
  NXstatus tmp = (status);                                                                                             \
  if (tmp != NXstatus::NX_OK) {                                                                                        \
    throw NeXus::Exception(msg, m_filename);                                                                           \
  }

/**
 * \file NeXusFile.cpp
 * The implementation of the NeXus C++ API
 */

namespace { // anonymous namespace to keep it in the file

constexpr std::string group_class_spec("NX_class");

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

static vector<int64_t> toDimSize(vector<int> const &small_v) {
  // copy the dims over to call the int64_t version
  return DimSizeVector(small_v.begin(), small_v.end());
}

static vector<int64_t> toDimSize(DimArray const &small_v) {
  // copy the dims over to call the int64_t version
  return DimSizeVector(small_v.begin(), small_v.end());
}

template <typename T> static DimArray toDimArray(vector<T> const &small_v) {
  DimArray ret{0};
  for (size_t i = 0; i < small_v.size(); i++) {
    ret.at(i) = static_cast<size_t>(small_v[i]);
  }
  return ret;
}

static H5::FileAccPropList defaultFileAcc() {
  H5::FileAccPropList access_plist;
  access_plist.setFcloseDegree(H5F_CLOSE_STRONG);
  return access_plist;
}

static std::unordered_map<int, void const *> const nxToHDF5Map{
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
    std::pair<int, void const *>(NXnumtype::FLOAT64, &H5::PredType::NATIVE_DOUBLE)};

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

  // if the type is one of the predefined datatypes used by us, just get it
  auto findit = nxToHDF5Map.begin();
  for (; *static_cast<H5::PredType const *>(findit->second) != dt && findit != nxToHDF5Map.end(); findit++) {
  }
  if (dt == H5::PredType::NATIVE_CHAR) {
    iPtype = NXnumtype::CHAR;
  } else if (*static_cast<H5::PredType const *>(findit->second) == dt && findit != nxToHDF5Map.end()) {
    iPtype = findit->first;
  } else {
    // if it's not a usual type, try to deduce the type from the datatype object
    auto tclass = dt.getClass();

    if (tclass == H5T_STRING) {
      iPtype = NXnumtype::CHAR;
    } else if (tclass == H5T_INTEGER) {
      size_t size = dt.getSize();
      H5T_sign_t sign = H5T_SGN_2;
      // NOTE H5cpp only defines the getSign method for IntType objects
      // but getSign is not defined on any other DataType class, and
      // can cause segfault if used on any other object.  Therefore we
      // have to default to assuming signed int and only go to unsigned
      // if we are able to deduce otherwise
      H5::IntType const *it = dynamic_cast<H5::IntType const *>(&dt);
      if (it != nullptr) {
        sign = it->getSign();
      }
      // signed integers
      if (sign == H5T_SGN_2) {
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
  for (hsize_t i=0; i < grp->getNumObjs(); i++) {
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
  throw Exception(msg.str());
}

template <> MANTID_NEXUS_DLL NXnumtype getType(char const) { return NXnumtype::CHAR; }

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

} // namespace NeXus

namespace NeXus {

H5::Group *File::getRoot() {
  return dynamic_cast<H5::Group *>(this);
}

/**Return a pointer corresponding to current location in file stack. */
H5::H5Location *File::getCurrentLocation() {
  H5::H5Location *ret = m_stack.back().get();
  if (m_stack.back() == nullptr) {
    ret = this->getRoot();
  }
  return ret;
}

template <typename T> T *File::getCurrentLocationAs() {
  T *loc = dynamic_cast<T *>(this->getCurrentLocation());
  if (loc == nullptr) {
    throw Exception("Could not cast current location to needed H5Cpp type\n", m_filename);
  }
  return loc;
}

/** Verify that the class name attribute set on the group
 *  matches the class name being looked up.
 */
bool File::verifyGroupClass(H5::Group const &grp, std::string const &class_name) const {
  H5::DataType dt(H5T_STRING, class_name.size());
  if (!grp.attrExists(group_class_spec)) {
    throw Exception("This was not found.\n");
  }
  H5::Attribute attr = grp.openAttribute(group_class_spec);
  std::string res("");
  attr.read(dt, res);
  if (res == "") {
    throw Exception("Error reading the group class name\n", m_filename);
  }
  return class_name == res;
}

File::File(std::string const &filename, H5access const access)
    : H5File(filename, access, defaultFileAcc()), m_close_handle(true), m_filename(filename) {
  this->m_stack.push_back(nullptr);
};

File::File(char const *filename, H5access const access)
    : H5File(filename, access, defaultFileAcc()), m_close_handle(true), m_filename(filename) {
  this->m_stack.push_back(nullptr);
};

// copy constructors

File::File(File const &f) : H5File(f), m_close_handle(f.m_close_handle), m_filename(f.m_filename) {}

File::File(H5::H5File const &hf) : H5File(hf), m_close_handle(true), m_filename(hf.getFileName()) {}

// File::File(File const *const pf) : m_close_handle(pf->m_close_handle) {}

// File::File(std::shared_ptr<File> pf) : m_close_handle(pf->m_close_handle) {}

// assignment operator

File &File::operator=(File const &f) {
  if (this == &f) {
  } else {
    this->m_close_handle = f.m_close_handle;
    this->m_filename = f.m_filename;
    this->m_stack = f.m_stack;
  }
  return *this;
}

// deconstructor

File::~File() {
  if (this->m_close_handle) {
    this->close();
  }
  m_stack.clear();
}

void File::flush(H5F_scope_t scope) const { H5::H5File::flush(scope); }

void File::makeGroup(std::string const &name, std::string const &class_name, bool open_group) {
  if (name.empty()) {
    throw Exception("Supplied empty name to makeGroup", m_filename);
  }
  if (class_name.empty()) {
    throw Exception("Supplied empty class name to makeGroup", m_filename);
  }
  // make the group
  H5::Group grp = this->getCurrentLocation()->createGroup(name);
  // add the class name as a std::string attribute
  H5::DataSpace aid2(H5S_SCALAR);
  H5::DataType aid1(H5T_STRING, class_name.size());
  H5::Attribute attr = grp.createAttribute(group_class_spec, aid1, aid2);
  attr.write(aid1, class_name);
  if (open_group) {
    this->m_stack.push_back(std::make_shared<H5::Group>(grp));
  }
}

void File::openGroup(std::string const &name, std::string const &class_name) {
  if (name.empty()) {
    throw Exception("Supplied empty name to openGroup", m_filename);
  }
  if (class_name.empty()) {
    throw Exception("Supplied empty class name to openGroup", m_filename);
  }
  auto current = this->getCurrentLocation();
  if (!current->nameExists(name)) {
    throw Exception("The supplied dataset name does not exist", m_filename);
  }
  std::shared_ptr<H5::Group> grp;
  if (current == this) {
    grp = std::make_shared<H5::Group>(H5::H5File::openGroup(name));
  } else {
    grp = std::make_shared<H5::Group>(current->openGroup(name));
  }
  if(!verifyGroupClass(*(grp.get()), class_name)) {
    throw Exception("Invalid group class name\n", m_filename);
  }
  this->m_stack.push_back(grp);
}

// void File::openPath(std::string const &path) {
//   if (path.empty()) {
//     throw Exception("Supplied empty path to openPath");
//   }
//   CALL_NAPI(NXopenpath(*(this->m_pfile_id), path.c_str()), "NXopenpath(" + path + ") failed");
// }

// void File::openGroupPath(std::string const &path) {
//   if (path.empty()) {
//     throw Exception("Supplied empty path to openGroupPath");
//   }
//   CALL_NAPI(NXopengrouppath(*(this->m_pfile_id), path.c_str()), "NXopengrouppath(" + path + ") failed");
// }

string File::getPath() {
  return this->getCurrentLocationAs<H5::H5Object>()->getObjName();
}

void File::closeGroup() {
  auto loc = this->getCurrentLocation();
  if (loc == this) {
    throw Exception("No group to close\n", m_filename);
  }
  try {
    H5::Group *grp = static_cast<H5::Group *>(loc);
    grp->close();
    this->m_stack.pop_back();
  } catch (...) {
    throw Exception("Object at current location is not a group\n", m_filename);
  }
}

void File::makeData(std::string const &name, NXnumtype datatype, vector<int> const &dims, bool open_data) {
  this->makeData(name, datatype, toDimSize(dims), open_data);
}

void File::makeData(std::string const &name, NXnumtype datatype, DimVector const &dims, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw Exception("Supplied empty label to makeData", m_filename);
  }
  if (dims.empty()) {
    throw Exception("Supplied empty dimensions to makeData", m_filename);
  }

  // make the data set
  H5::DataSpace ds((int)dims.size(), toDimArray(dims).data());
  try {
    H5::DataSet data = this->getCurrentLocation()->createDataSet(name, nxToHDF5Type(datatype), ds);
    if (open_data) {
      this->m_stack.push_back(std::make_shared<H5::DataSet>(data));
    }
  } catch (...) {
    throw Exception("Datasets cannot be created at current location", m_filename);
  }
}

template <typename NumT>
void File::makeData(std::string const &name, NXnumtype const type, NumT const length, bool open_data) {
  DimVector dims;
  dims.push_back(static_cast<dimsize_t>(length));
  this->makeData(name, type, dims, open_data);
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
  vector<int> dims;
  dims.push_back(static_cast<int>(my_value.size()));
  this->makeData(name, NXnumtype::CHAR, dims, true);

  this->putData(my_value.data());

  this->closeData();
}

template <typename NumT> void File::writeData(std::string const &name, vector<NumT> const &value) {
  DimVector dims(1, static_cast<dimsize_t>(value.size()));
  this->writeData(name, value, dims);
}

template <typename NumT> void File::writeData(std::string const &name, vector<NumT> const &value, vector<int> const &dims)
{
  this->makeData(name, getType<NumT>(), dims, true);
  this->putData(value);
  this->closeData();
}

template <typename NumT> void File::writeData(std::string const &name, vector<NumT> const &value, DimVector const &dims) {
  this->makeData(name, getType<NumT>(), dims, true);
  this->putData(value);
  this->closeData();
}

// template <typename NumT> void File::writeExtendibleData(std::string const &name, vector<NumT> &value) {
//   // Use a default chunk size of 4096 bytes. TODO: Is this optimal?
//   writeExtendibleData(name, value, 4096);
// }

// template <typename NumT>
// void File::writeExtendibleData(std::string const &name, vector<NumT> &value, dimsize_t const chunk) {
//   DimVector dims(1, NX_UNLIMITED);
//   DimSizeVector chunk_dims(1, chunk);
//   // Use chunking without using compression
//   this->makeCompData(name, getType<NumT>(), dims, NONE, chunk_dims, true);
//   this->putSlab(value, dimsize_t(0), dimsize_t(value.size()));
//   this->closeData();
// }

// template <typename NumT>
// void File::writeExtendibleData(std::string const &name, vector<NumT> &value, DimVector &dims, std::vector<int64_t> &chunk)
// {
//   // Create the data with unlimited 0th dimensions
//   DimVector unlim_dims(dims);
//   unlim_dims[0] = NX_UNLIMITED;
//   // Use chunking without using compression
//   this->makeCompData(name, getType<NumT>(), unlim_dims, NONE, chunk, true);
//   // And put that slab of that of that given size in there
//   DimSizeVector start(dims.size(), 0);
//   this->putSlab(value, start, dims);
//   this->closeData();
// }

// template <typename NumT> void File::writeUpdatedData(std::string const &name, std::vector<NumT> &value) {
//   this->openData(name);
//   this->putSlab(value, dimsize_t(0), dimsize_t(value.size()));
//   this->closeData();
// }

// template <typename NumT>
// void File::writeUpdatedData(std::string const &name, std::vector<NumT> &value, DimVector &dims) {
//   this->openData(name);
//   DimSizeVector start(dims.size(), 0);
//   this->putSlab(value, start, dims);
//   this->closeData();
// }

// void File::makeCompData(std::string const &name, NXnumtype const type, vector<int> const &dims, NXcompression const comp,
//                         vector<int> const &bufsize, bool open_data) {
//   this->makeCompData(name, type, toDimSize(dims), comp, toDimSize(bufsize), open_data);
// }

// void File::makeCompData(std::string const &name, NXnumtype const type, DimVector const &dims, NXcompression const comp,
//                         DimSizeVector const &bufsize, bool open_data) {
//   // error check the parameters
//   if (name.empty()) {
//     throw Exception("Supplied empty name to makeCompData");
//   }
//   if (dims.empty()) {
//     throw Exception("Supplied empty dimensions to makeCompData");
//   }
//   if (bufsize.empty()) {
//     throw Exception("Supplied empty bufsize to makeCompData");
//   }
//   if (dims.size() != bufsize.size()) {
//     stringstream msg;
//     msg << "Supplied dims rank=" << dims.size() << " must match supplied bufsize rank=" << bufsize.size()
//         << "in makeCompData";
//     throw Exception(msg.str());
//   }

//   // do the work
//   int i_type = static_cast<int>(type);
//   int i_comp = static_cast<int>(comp);
//   NXstatus status = NXcompmakedata64(*(this->m_pfile_id), name.c_str(), type, static_cast<int>(dims.size()),
//                                      const_cast<int64_t *>(&(dims[0])), i_comp, const_cast<int64_t
//                                      *>(&(bufsize[0])));

//   // report errors
//   if (status != NXstatus::NX_OK) {
//     stringstream msg;
//     msg << "NXcompmakedata64(" << name << ", " << i_type << ", " << dims.size() << ", " << toString(dims) << ", "
//         << comp << ", " << toString(bufsize) << ") failed";
//     throw Exception(msg.str(), status);
//   }
//   if (open_data) {
//     this->openData(name);
//   }
// }

// template <typename NumT>
// void File::writeCompData(std::string const &name, vector<NumT> const &value, vector<int> const &dims,
//                          NXcompression const comp, vector<int> const &bufsize) {
//   this->writeCompData(name, value, toDimSize(dims), comp, toDimSize(bufsize));
// }

// template <typename NumT>
// void File::writeCompData(std::string const &name, vector<NumT> const &value, DimVector const &dims, const NXcompression
// comp,
//                          DimSizeVector const &bufsize) {
//   this->makeCompData(name, getType<NumT>(), dims, comp, bufsize, true);
//   this->putData(value);
//   this->closeData();
// }

void File::openData(std::string const &name) {
  if (name.empty()) {
    throw Exception("Supplied empty name to openData", m_filename);
  }
  auto current = this->getCurrentLocation();
  if (!current->nameExists(name)) {
    throw Exception("The indicated dataset does not exist", m_filename);
  }
  auto data = std::make_shared<H5::DataSet>(current->openDataSet(name));
  this->m_stack.push_back(data);
}

void File::closeData() {
  auto current = this->getCurrentLocation();
  if (current == this) {
    throw Exception("No data to close\n", m_filename);
  }
  try {
    H5::DataSet *data = static_cast<H5::DataSet *>(current);
    data->close();
    this->m_stack.pop_back();
  } catch (...) {
    throw Exception("Object at current location is not a dataset\n", m_filename);
  }
}

template <typename NumT> void File::putData(NumT const *data) {
  if (data == NULL) {
    throw Exception("Data specified as null in putData", m_filename);
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
      start[i] = static_cast<int64_t>(dims[i] + 1);
      size[i] = 1;
    } else {
      start[i] = 0;
      size[i] = static_cast<int64_t>(dims[i]);
    }
  }
  if (unlimited) {
    std::runtime_error("Not implemented");
  } else {
    try {
      dataset->write(data, Mantid::NeXus::H5Util::getType<NumT>());
    } catch (...) {
      throw Exception("Failed to write data\n", m_filename);
    }
  }
}

template <typename NumT> void File::putData(vector<NumT> const &data) {
  if (data.empty()) {
    throw Exception("Supplied empty data to putData", m_filename);
  }
  this->putData<NumT>(data.data());
}

template <> MANTID_NEXUS_DLL void File::putData<std::string>(std::string const *data) {
  char const *chardata = data->c_str();
  this->putData(chardata);
}

template <typename NumT> void File::putAttr(std::string const &name, NumT const &value) {
  if (name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (name.empty()) {
    throw Exception("Supplied empty name to putAttr");
  }

  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  DimArray dims{1, 0, 0, 0};
  H5::DataSpace aid2(1, dims.data());
  H5::DataType aid1 = Mantid::NeXus::H5Util::getType<NumT>();
  H5::Attribute attr = current->createAttribute(name, aid1, aid2);
  attr.write(aid1, &value);
  attr.close();
}

// in std::string case use H5::Attribute::write(const DataType &mem_type, const H5std_string &strg)
template <> MANTID_NEXUS_DLL void File::putAttr<std::string>(std::string const &name, std::string const &value) {
  if (name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (name.empty()) {
    throw Exception("Supplied empty name to putAttr");
  }
  if (value == "") {
    throw Exception("Supplied empty std::string value to putAttr");
  }

  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  DimArray dims{value.size(), 0, 0, 0};
  H5::DataSpace aid2(1, dims.data());
  H5::DataType aid1(H5T_STRING, value.size());
  H5::Attribute attr = current->createAttribute(name, aid1, aid2);
  attr.write(aid1, value);
  attr.close();
}

void File::putAttr(std::string const &name, std::string const &value, bool const empty_add_space) {
  std::string my_value(value);
  if (my_value.empty() && empty_add_space)
    my_value = " "; // Make a default "space" to avoid errors.
  this->putAttr<std::string>(name, my_value);
}

// void File::putSlab(void const *data, vector<int> const &start, vector<int> const &size) {
//   this->putSlab(data, toDimSize(start), toDimSize(size));
// }

// void File::putSlab(void const *data, DimSizeVector const &start, DimSizeVector const &size) {
//   if (data == NULL) {
//     throw Exception("Data specified as null in putSlab");
//   }
//   if (start.empty()) {
//     throw Exception("Supplied empty start to putSlab");
//   }
//   if (size.empty()) {
//     throw Exception("Supplied empty size to putSlab");
//   }
//   if (start.size() != size.size()) {
//     stringstream msg;
//     msg << "Supplied start rank=" << start.size() << " must match supplied size rank=" << size.size() << "in
//     putSlab"; throw Exception(msg.str());
//   }
//   NXstatus status = NXputslab64(*(this->m_pfile_id), data, &(start[0]), &(size[0]));
//   if (status != NXstatus::NX_OK) {
//     stringstream msg;
//     msg << "NXputslab64(data, " << toString(start) << ", " << toString(size) << ") failed";
//     throw Exception(msg.str(), status);
//   }
// }

// template <typename NumT>
// void File::putSlab(vector<NumT> const &data, vector<int> const &start, vector<int> const &size) {
//   this->putSlab(data, toDimSize(start), toDimSize(size));
// }

// template <typename NumT>
// void File::putSlab(vector<NumT> const &data, DimSizeVector const &start, DimSizeVector const &size) {
//   if (data.empty()) {
//     throw Exception("Supplied empty data to putSlab");
//   }
//   this->putSlab(&(data[0]), start, size);
// }

// template <typename NumT> void File::putSlab(vector<NumT> const &data, int start, int size) {
//   this->putSlab(data, static_cast<dimsize_t>(start), static_cast<dimsize_t>(size));
// }

// template <typename NumT> void File::putSlab(vector<NumT> const &data, dimsize_t start, dimsize_t size) {
//   DimSizeVector start_v(1, start);
//   DimSizeVector size_v(1, size);
//   this->putSlab(data, start_v, size_v);
// }

NXlink File::getDataID() {
  // make sure current location is a dataset
  auto *current = this->getCurrentLocationAs<H5::DataSet>();

  /*
    this means: if the item is already linked: use the target attribute; 
    else, use the path to the current node
  */
  NXlink link;
  AttrInfo targetInfo {"target", NXnumtype::CHAR, DimVector({-1})};
  try {
    link.targetPath = this->getStrAttr(targetInfo);
  } catch (Exception &) {
    link.targetPath = current->getObjName();
  }
  link.linkType = NXlinktype::sds;
  return link;
}

bool File::isDataSetOpen() {
  try {
    this->getCurrentLocationAs<H5::DataSet>();
  } catch (...) {
    return false;
  }
  return true;
}
// /*----------------------------------------------------------------------*/

void File::makeLink(NXlink &link) {

  // locate name of the element to link
  std::string itemName = link.targetPath.substr(link.targetPath.find_last_of("/")+1, link.targetPath.npos) ;//(link.targetPath, '/');

  // build pathname to link from our current group and name of thing to link
  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  std::string linkTarget = current->getObjName() + itemName;

  // create link
  current->link(link.targetPath, H5L_SAME_LOC, linkTarget);
}

template <typename NumT> void File::getData(NumT *data) {
  if (data == NULL) {
    throw Exception("Supplied null pointer to getData");
  }

  // make sure this is a data set
  H5::DataSet *dataset = this->getCurrentLocationAs<H5::DataSet>();

  // make sure the data has the correct type
  Info info = this->getInfo();
  if (info.type != getType<NumT>()) {
    throw Exception("File::getInfo() failed -- inconsistent NXnumtype");
  }

  // now try to read
  try {
    dataset->read(data, Mantid::NeXus::H5Util::getType<NumT>());
  } catch (...) {
    throw Exception("Failed to get data\n", m_filename);
  }
}

template <> MANTID_NEXUS_DLL void File::getData<std::string>(std::string *data) {
  Info info = this->getInfo();

  if (info.type != NXnumtype::CHAR) {
    stringstream msg;
    msg << "Cannot use File::getData<string>() on non-character data. Found type=" << info.type;
    throw Exception(msg.str(), m_filename);
  }
  if (info.dims.size() != 1) {
    stringstream msg;
    msg << "File::getData<string>() only understand rank=1 data. Found rank=" << info.dims.size();
    throw Exception(msg.str());
  }
  char *value = new char[static_cast<size_t>(info.dims[0]) + 1];
  try {
    this->getData<char>(value);
  } catch (Exception const &) {
    delete[] value;
    throw; // rethrow the original exception
  }
  *data = string(value, static_cast<size_t>(info.dims[0]));
  delete[] value;
}

template <typename NumT> void File::getData(vector<NumT> &data) {
  Info info = this->getInfo();

  if (info.type != getType<NumT>()) {
    throw Exception("NXgetdata failed - invalid vector type", m_filename);
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
    throw Exception("NexusFile::getDataCoerce(): Could not coerce to int.");
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
    throw Exception("NexusFile::getDataCoerce(): Could not coerce to double.");
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
  if (dataVector.size() > 0)
    data = dataVector[0];
  this->closeData();
}

void File::readData(std::string const &dataName, std::string &data) {
  this->openData(dataName);
  data = this->getStrData();
  this->closeData();
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

string File::getStrData() {
  std::string res;
  this->getData<std::string>(&res);
  return res;
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
  for (std::size_t i = 0; i < rank; i++) {
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
  recursivePopulateEntries(this->getRoot(), result);
}

std::string File::getTopLevelEntryName() {
  std::string top("");
  
  // go to root and verify
  H5::Group *root = this->getRoot();
  if (root == nullptr){
    throw Exception("NeXusFile: invalid file has no root", m_filename);
  }
  // look for first group of class NXentry
  std::size_t firstGrp = 0;
  for (; firstGrp < root->getNumObjs(); firstGrp++){
    if (root->getObjTypeByIdx(firstGrp) == H5G_GROUP) {
      top = root->getObjnameByIdx(firstGrp);
      H5::Group grp = root->openGroup(top);
      if(this->verifyGroupClass(grp, "NXentry")) {
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

// void File::getSlab(void *data, vector<int> const &start, vector<int> const &size) {
//   this->getSlab(data, toDimSize(start), toDimSize(size));
// }

// void File::getSlab(void *data, DimSizeVector const &start, DimSizeVector const &size) {
//   if (data == NULL) {
//     throw Exception("Supplied null pointer to getSlab");
//   }
//   if (start.size() == 0) {
//     stringstream msg;
//     msg << "Supplied empty start offset, rank = " << start.size() << " in getSlab";
//     throw Exception(msg.str());
//   }
//   if (start.size() != size.size()) {
//     stringstream msg;
//     msg << "In getSlab start rank=" << start.size() << " must match size rank=" << size.size();
//     throw Exception(msg.str());
//   }

//   CALL_NAPI(NXgetslab64(*(this->m_pfile_id), data, &(start[0]), &(size[0])), "NXgetslab failed");
// }

template <typename NumT> void File::getAttr(std::string const &name, NumT &value) {
  // verify the current location can hold an attribute, and has the attribute named
  H5::H5Object *current = this->getCurrentLocationAs<H5::H5Object>();
  if (!current->attrExists(name)) {
    throw Exception("This was not found.\n");
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
    throw Exception("This was not found.\n");
  }

  // open the attribute, and read it
  H5::Attribute attr = current->openAttribute(name);
  auto dt = attr.getDataType();
  value = "";
  attr.read(dt, value);
  if (value == "") {
    throw Exception("Error reading string attribute\n", m_filename);
  }
  attr.close();
}

string File::getStrAttr(AttrInfo const &info) {
  if (info.type != NXnumtype::CHAR) {
    stringstream msg;
    msg << "getStrAttr only works with strings (type=" << NXnumtype::CHAR << ") found type=" << info.type;
    throw Exception(msg.str());
  }
  std::string res;
  this->getAttr<std::string>(info.name, res);
  return res;
}

// vector<AttrInfo> File::getAttrInfos() {
//   vector<AttrInfo> infos;
//   this->initAttrDir();
//   AttrInfo temp;
//   while (true) {
//     temp = this->getNextAttr();
//     if (temp.name == NULL_STR) {
//       break;
//     }
//     infos.push_back(temp);
//   }
//   return infos;
// }

// bool File::hasAttr(std::string const &name) {
//   this->initAttrDir();
//   AttrInfo temp;
//   while (true) {
//     temp = this->getNextAttr();
//     if (temp.name == NULL_STR) {
//       break;
//     }
//     if (temp.name == name)
//       return true;
//   }
//   return false;
// }

NXlink File::getGroupID() {
  // make sure current location is a group
  auto *current = this->getCurrentLocationAs<H5::Group>();

  /*
    this means: if the item is already linked: use the target attribute; 
    else, use the path to the current node
  */
  NXlink link;
  AttrInfo targetInfo {"target", NXnumtype::CHAR, DimVector({-1})};
  try {
    link.targetPath = this->getStrAttr(targetInfo);
  } catch (Exception &) {
    link.targetPath == current->getObjName();
  }
  link.linkType = NXlinktype::group;
  return link;
}

// void File::initAttrDir() { CALL_NAPI(NXinitattrdir(*(this->m_pfile_id)), "NXinitattrdir failed"); }

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

// /* ---------------------------------------------------------------- */
// /* Concrete instantiations of template definitions.                 */
// /* ---------------------------------------------------------------- */
// /*
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

template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, float &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, double &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int8_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint8_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int16_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint16_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int32_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint32_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, int64_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, uint64_t &value);
template MANTID_NEXUS_DLL void File::getAttr(std::string const &name, char &value);

template MANTID_NEXUS_DLL void File::makeData(std::string const &name, NXnumtype const type, int const length,
                                              bool open_data);
template MANTID_NEXUS_DLL void File::makeData(std::string const &name, NXnumtype const type, int64_t const length,
                                              bool open_data);
template MANTID_NEXUS_DLL void File::makeData(std::string const &name, NXnumtype const type, size_t const length,
                                              bool open_data);

template MANTID_NEXUS_DLL void File::putData(int const *data);
template MANTID_NEXUS_DLL void File::putData(int64_t const *data);
template MANTID_NEXUS_DLL void File::putData(size_t const *data);
template MANTID_NEXUS_DLL void File::putData(float const *data);
template MANTID_NEXUS_DLL void File::putData(double const *data);
template MANTID_NEXUS_DLL void File::putData(char const *data);
template MANTID_NEXUS_DLL void File::putData(std::string const *data);

template MANTID_NEXUS_DLL void File::putData(vector<int> const &data);
template MANTID_NEXUS_DLL void File::putData(vector<int64_t> const &data);
template MANTID_NEXUS_DLL void File::putData(vector<size_t> const &data);
template MANTID_NEXUS_DLL void File::putData(vector<float> const &data);
template MANTID_NEXUS_DLL void File::putData(vector<double> const &data);
template MANTID_NEXUS_DLL void File::putData(vector<std::string> const &data);

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
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<double> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int8_t> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint8_t> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int16_t> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint16_t> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int32_t> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint32_t> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<int64_t> const &value,
                                               std::vector<int> const &dims);
template MANTID_NEXUS_DLL void File::writeData(std::string const &name, vector<uint64_t> const &value,
                                               std::vector<int> const &dims);

template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<float> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<double> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int8_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint8_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int16_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint16_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int32_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint32_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int64_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint64_t> &value);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<char> &value);

template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<float> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<double> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int8_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint8_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int16_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint16_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int32_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint32_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int64_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint64_t> &value,
                                                         dimsize_t const chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<char> &value,
                                                         dimsize_t const chunk);

template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<float> &value, DimVector
&dims,
                                                         DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<double> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int8_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint8_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int16_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint16_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int32_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint32_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<int64_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<uint64_t> &value,
                                                         DimVector &dims, DimSizeVector &chunk);
template MANTID_NEXUS_DLL void File::writeExtendibleData(std::string const &name, std::vector<char> &value, DimVector
&dims,
                                                         DimSizeVector &chunk);

template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<float> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<double> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int8_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint8_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int16_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint16_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int32_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint32_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int64_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint64_t> &value);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<char> &value);

template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<float> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<double> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int8_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint8_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int16_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint16_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int32_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint32_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<int64_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<uint64_t> &value, DimVector &dims);
template MANTID_NEXUS_DLL void File::writeUpdatedData(std::string const &name, vector<char> &value, DimVector &dims);

template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<float> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<float> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<double> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<double> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int8_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int8_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint8_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint8_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int16_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int16_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint16_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint16_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int32_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int32_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint32_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint32_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int64_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<int64_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint64_t> const &value,
                                                   vector<int> const &dims, NXcompression const comp,
                                                   vector<int> const &bufsize);
template MANTID_NEXUS_DLL void File::writeCompData(std::string const &name, vector<uint64_t> const &value,
                                                   DimVector const &dims, NXcompression const comp,
                                                   DimSizeVector const &bufsize);

template MANTID_NEXUS_DLL void File::getData(int *data);
template MANTID_NEXUS_DLL void File::getData(int64_t *data);
template MANTID_NEXUS_DLL void File::getData(size_t *data);
template MANTID_NEXUS_DLL void File::getData(float *data);
template MANTID_NEXUS_DLL void File::getData(double *data);
template MANTID_NEXUS_DLL void File::getData(char *data);
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

template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<float> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<double> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<int8_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<uint8_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<int16_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<uint16_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<int32_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<uint32_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<int64_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<uint64_t> &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, vector<char> &data);

template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, float &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, double &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, int8_t &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, uint8_t &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, int16_t &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, uint16_t &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, int32_t &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, uint32_t &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, int64_t &data);
template MANTID_NEXUS_DLL void File::readData(std::string const &dataName, uint64_t &data);

// template MANTID_NEXUS_DLL void File::putSlab(std::vector<float> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<double> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int8_t> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint8_t> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int16_t> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint16_t> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int32_t> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint32_t> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int64_t> const &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint64_t> const &data, int start, int size);

// template MANTID_NEXUS_DLL void File::putSlab(std::vector<float> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<double> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int8_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint8_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int16_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint16_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int32_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint32_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<int64_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
// template MANTID_NEXUS_DLL void File::putSlab(std::vector<uint64_t> const &data, DimSizeVector const &start,
//                                              DimSizeVector const &size);
