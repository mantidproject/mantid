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

constexpr string group_class_spec("NX_class");

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

static vector<int64_t> toDimSize(const vector<int> &small_v) {
  // copy the dims over to call the int64_t version
  return DimSizeVector(small_v.begin(), small_v.end());
}

static vector<int64_t> toDimSize(const DimArray &small_v) {
  // copy the dims over to call the int64_t version
  return DimSizeVector(small_v.begin(), small_v.end());
}

template <typename T> static DimArray toDimArray(const vector<T> &small_v) {
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

static std::unordered_map<int, void const *> const nxToHDF5Map {
    std::pair<int, void const*>(NXnumtype::CHAR, &H5::PredType::NATIVE_CHAR),
    std::pair<int, void const*>(NXnumtype::INT8, &H5::PredType::NATIVE_SCHAR),
    std::pair<int, void const*>(NXnumtype::UINT8, &H5::PredType::NATIVE_UCHAR),
    std::pair<int, void const*>(NXnumtype::INT16, &H5::PredType::NATIVE_INT16),
    std::pair<int, void const*>(NXnumtype::UINT16, &H5::PredType::NATIVE_UINT16),
    std::pair<int, void const*>(NXnumtype::INT32, &H5::PredType::NATIVE_INT32),
    std::pair<int, void const*>(NXnumtype::UINT32, &H5::PredType::NATIVE_UINT32),
    std::pair<int, void const*>(NXnumtype::INT64, &H5::PredType::NATIVE_INT64),
    std::pair<int, void const*>(NXnumtype::UINT64, &H5::PredType::NATIVE_UINT64),
    std::pair<int, void const*>(NXnumtype::FLOAT32, &H5::PredType::NATIVE_FLOAT),
    std::pair<int, void const*>(NXnumtype::FLOAT64, &H5::PredType::NATIVE_DOUBLE)
  };

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
  for(; *static_cast<H5::PredType const *>(findit->second) != dt && findit != nxToHDF5Map.end(); findit++){}
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
      // but this method is not defined on any other DataType class, and 
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

/**Return a pointer corresponding to current location in file stack. */
H5::H5Location *File::getCurrentLocation() {
  H5::H5Location *ret = m_stack.back().get();
  if (m_stack.back() == nullptr) {
    ret = &(*this);
  }
  return ret;
}

/** Verify that the class name attribute set on the group
 *  matches the class name being looked up.
 */
void File::verifyGroupClass(H5::Group const &grp, string const &class_name) const {
  H5::DataType dt(H5T_STRING, class_name.size());
  if (!grp.attrExists(group_class_spec)) {
    throw Exception("This was not found.\n");
  }
  H5::Attribute attr = grp.openAttribute(group_class_spec);
  string res("");
  attr.read(dt, res);
  if (res == "") {
    throw Exception("Error reading the group class name\n", m_filename);
  }
  if (class_name != res) {
    throw Exception("Invalid group class name\n", m_filename);
  }
}

File::File(string const &filename, H5access const access)
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

void File::makeGroup(const string &name, const string &class_name, bool open_group) {
  if (name.empty()) {
    throw Exception("Supplied empty name to makeGroup", m_filename);
  }
  if (class_name.empty()) {
    throw Exception("Supplied empty class name to makeGroup", m_filename);
  }
  // make the group
  H5::Group grp = this->getCurrentLocation()->createGroup(name);
  // add the class name as a string attribute
  H5::DataSpace aid2(H5S_SCALAR);
  H5::DataType aid1(H5T_STRING, class_name.size());
  H5::Attribute attr = grp.createAttribute(group_class_spec, aid1, aid2);
  attr.write(aid1, class_name);
  if (open_group) {
    this->m_stack.push_back(std::make_shared<H5::Group>(grp));
  }
}

void File::openGroup(const string &name, const string &class_name) {
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
  verifyGroupClass(*(grp.get()), class_name);
  this->m_stack.push_back(grp);
}

// void File::openPath(const string &path) {
//   if (path.empty()) {
//     throw Exception("Supplied empty path to openPath");
//   }
//   CALL_NAPI(NXopenpath(*(this->m_pfile_id), path.c_str()), "NXopenpath(" + path + ") failed");
// }

// void File::openGroupPath(const string &path) {
//   if (path.empty()) {
//     throw Exception("Supplied empty path to openGroupPath");
//   }
//   CALL_NAPI(NXopengrouppath(*(this->m_pfile_id), path.c_str()), "NXopengrouppath(" + path + ") failed");
// }

string File::getPath() {
  string output = "";
  for (auto loc : this->m_stack) {
    output += "/";
    if (loc == nullptr) {
      // skip the root entry
    } else {
      try {
        output = dynamic_cast<H5::Group *>(loc.get())->getObjName();
      } catch (...) {
        throw Exception("nope!", m_filename);
      }
    }
  }
  return output;
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

void File::makeData(const string &name, NXnumtype datatype, const vector<int> &dims, bool open_data) {
  this->makeData(name, datatype, toDimSize(dims), open_data);
}

void File::makeData(const string &name, NXnumtype datatype, const DimVector &dims, bool open_data) {
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
void File::makeData(const string &name, const NXnumtype type, const NumT length, bool open_data) {
  DimVector dims;
  dims.push_back(static_cast<dimsize_t>(length));
  this->makeData(name, type, dims, open_data);
}

/*
template <typename NumT> void File::writeData(const string &name, const NumT &value) {
  std::vector<NumT> v(1, value);
  this->writeData(name, v);
}

void File::writeData(const string &name, const char *value) { this->writeData(name, std::string(value)); }

void File::writeData(const string &name, const string &value) {
  string my_value(value);
  // Allow empty strings by defaulting to a space
  if (my_value.empty())
    my_value = " ";
  vector<int> dims;
  dims.push_back(static_cast<int>(my_value.size()));
  this->makeData(name, NXnumtype::CHAR, dims, true);

  this->putData(my_value.data());

  this->closeData();
}

template <typename NumT> void File::writeData(const string &name, const vector<NumT> &value) {
  DimVector dims(1, static_cast<dimsize_t>(value.size()));
  this->writeData(name, value, dims);
}

template <typename NumT> void File::writeData(const string &name, const vector<NumT> &value, const vector<int> &dims)
{
  this->makeData(name, getType<NumT>(), dims, true);
  this->putData(value);
  this->closeData();
}

template <typename NumT> void File::writeData(const string &name, const vector<NumT> &value, const DimVector &dims) {
  this->makeData(name, getType<NumT>(), dims, true);
  this->putData(value);
  this->closeData();
}
*/

// template <typename NumT> void File::writeExtendibleData(const string &name, vector<NumT> &value) {
//   // Use a default chunk size of 4096 bytes. TODO: Is this optimal?
//   writeExtendibleData(name, value, 4096);
// }

// template <typename NumT>
// void File::writeExtendibleData(const string &name, vector<NumT> &value, const dimsize_t chunk) {
//   DimVector dims(1, NX_UNLIMITED);
//   DimSizeVector chunk_dims(1, chunk);
//   // Use chunking without using compression
//   this->makeCompData(name, getType<NumT>(), dims, NONE, chunk_dims, true);
//   this->putSlab(value, dimsize_t(0), dimsize_t(value.size()));
//   this->closeData();
// }

// template <typename NumT>
// void File::writeExtendibleData(const string &name, vector<NumT> &value, DimVector &dims, std::vector<int64_t> &chunk)
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

// template <typename NumT> void File::writeUpdatedData(const std::string &name, std::vector<NumT> &value) {
//   this->openData(name);
//   this->putSlab(value, dimsize_t(0), dimsize_t(value.size()));
//   this->closeData();
// }

// template <typename NumT>
// void File::writeUpdatedData(const std::string &name, std::vector<NumT> &value, DimVector &dims) {
//   this->openData(name);
//   DimSizeVector start(dims.size(), 0);
//   this->putSlab(value, start, dims);
//   this->closeData();
// }

// void File::makeCompData(const string &name, const NXnumtype type, const vector<int> &dims, const NXcompression comp,
//                         const vector<int> &bufsize, bool open_data) {
//   this->makeCompData(name, type, toDimSize(dims), comp, toDimSize(bufsize), open_data);
// }

// void File::makeCompData(const string &name, const NXnumtype type, const DimVector &dims, const NXcompression comp,
//                         const DimSizeVector &bufsize, bool open_data) {
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
// void File::writeCompData(const string &name, const vector<NumT> &value, const vector<int> &dims,
//                          const NXcompression comp, const vector<int> &bufsize) {
//   this->writeCompData(name, value, toDimSize(dims), comp, toDimSize(bufsize));
// }

// template <typename NumT>
// void File::writeCompData(const string &name, const vector<NumT> &value, const DimVector &dims, const NXcompression
// comp,
//                          const DimSizeVector &bufsize) {
//   this->makeCompData(name, getType<NumT>(), dims, comp, bufsize, true);
//   this->putData(value);
//   this->closeData();
// }

void File::openData(const string &name) {
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

template <typename NumT> void File::putData(const NumT *data) {
  if (data == NULL) {
    throw Exception("Data specified as null in putData", m_filename);
  }

  H5::DataSet *dataset = dynamic_cast<H5::DataSet *>(this->getCurrentLocation());
  if (dataset == nullptr) {
    throw Exception("Failed to write data, current location is not a DataSet", m_filename);
  }
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

template <typename NumT> void File::putData(const vector<NumT> &data) {
  if (data.empty()) {
    throw Exception("Supplied empty data to putData", m_filename);
  }
  this->putData<NumT>(data.data());
}

template <> MANTID_NEXUS_DLL void File::putData<std::string>(const string *data) {
  char const * chardata = data->c_str();
  this->putData(chardata);
}

template <typename NumT>
void File::putAttr(const AttrInfo &info, const NumT *data) {
  if (info.name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (info.name.empty()) {
    throw Exception("Supplied empty name to putAttr");
  }

  H5::H5Object *obj = dynamic_cast<H5::H5Object *>(this->getCurrentLocation());
  if (obj == nullptr) {
    throw Exception("Cannot add an attribute at current location", m_filename);
  }

  H5::DataSpace aid2((int)info.dims.size(), toDimArray(info.dims).data());
  H5::DataType aid1 = Mantid::NeXus::H5Util::getType<NumT>();
  H5::Attribute attr = obj->createAttribute(info.name, aid1, aid2);
  attr.write(aid1, data);
  attr.close();
}

template <typename NumT> void File::putAttr(const std::string &name, const NumT value) {
  AttrInfo info;
  info.name = name;
  info.length = 1;
  info.type = getType<NumT>();
  this->putAttr<NumT>(info, &value);
}

void File::putAttr(const char *name, const char *value) {
  if (name == NULL) {
    throw Exception("Specified name as null to putAttr");
  }
  if (value == NULL) {
    throw Exception("Specified value as null to putAttr");
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
  this->putAttr(info, &(my_value[0]));
}

// void File::putSlab(const void *data, const vector<int> &start, const vector<int> &size) {
//   this->putSlab(data, toDimSize(start), toDimSize(size));
// }

// void File::putSlab(const void *data, const DimSizeVector &start, const DimSizeVector &size) {
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
// void File::putSlab(const vector<NumT> &data, const vector<int> &start, const vector<int> &size) {
//   this->putSlab(data, toDimSize(start), toDimSize(size));
// }

// template <typename NumT>
// void File::putSlab(const vector<NumT> &data, const DimSizeVector &start, const DimSizeVector &size) {
//   if (data.empty()) {
//     throw Exception("Supplied empty data to putSlab");
//   }
//   this->putSlab(&(data[0]), start, size);
// }

// template <typename NumT> void File::putSlab(const vector<NumT> &data, int start, int size) {
//   this->putSlab(data, static_cast<dimsize_t>(start), static_cast<dimsize_t>(size));
// }

// template <typename NumT> void File::putSlab(const vector<NumT> &data, dimsize_t start, dimsize_t size) {
//   DimSizeVector start_v(1, start);
//   DimSizeVector size_v(1, size);
//   this->putSlab(data, start_v, size_v);
// }

// NXlink File::getDataID() {
//   NXlink link;
//   CALL_NAPI(NXgetdataID(*(this->m_pfile_id), &link), "NXgetdataID failed");
//   return link;
// }

// bool File::isDataSetOpen() {
//   NXlink id;
//   NXstatus status = NAPI_STATUS(NXgetdataID(*(this->m_pfile_id), &id), "");
//   return (status == NXstatus::NX_ERROR ? false; true);
// }
// /*----------------------------------------------------------------------*/

// void File::makeLink(NXlink &link) {
//  CALL_NAPI(NXmakelink(*(this->m_pfile_id), &link), "NXmakelink failed");
// }

template <typename NumT> void File::getData(NumT *data) {
  if (data == NULL) {
    throw Exception("Supplied null pointer to getData");
  }

  // make sure this is a data set
  H5::DataSet *dataset = dynamic_cast<H5::DataSet *>(this->getCurrentLocation());
  if (dataset == nullptr) {
    throw Exception("Failed to get data, current location is not a DataSet", m_filename);
  }

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

template <> MANTID_NEXUS_DLL void File::getData<string>(string *data) {
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
  } catch (const Exception &) {
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
                      [](const auto subtotal, const auto &value) { return subtotal * static_cast<size_t>(value); });

  // allocate memory to put the data into
  // need to use resize() rather than reserve() so vector length gets set
  data.resize(length);

  // fetch the data
  this->getData<NumT>(data.data());
}

// void File::getDataCoerce(vector<int> &data) {
//   Info info = this->getInfo();
//   if (info.type == NXnumtype::INT8) {
//     vector<int8_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::UINT8) {
//     vector<uint8_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::INT16) {
//     vector<int16_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::UINT16) {
//     vector<uint16_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::INT32) {
//     vector<int32_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::UINT32) {
//     vector<uint32_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else {
//     throw Exception("NexusFile::getDataCoerce(): Could not coerce to int.");
//   }
// }

// void File::getDataCoerce(vector<double> &data) {
//   Info info = this->getInfo();
//   if (info.type == NXnumtype::INT8) {
//     vector<int8_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::UINT8) {
//     vector<uint8_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::INT16) {
//     vector<int16_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::UINT16) {
//     vector<uint16_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::INT32) {
//     vector<int32_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::UINT32) {
//     vector<uint32_t> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::FLOAT32) {
//     vector<float> result;
//     this->getData(result);
//     data.assign(result.begin(), result.end());
//   } else if (info.type == NXnumtype::FLOAT64) {
//     this->getData(data);
//   } else {
//     throw Exception("NexusFile::getDataCoerce(): Could not coerce to double.");
//   }
// }

// template <typename NumT> void File::readData(const std::string &dataName, std::vector<NumT> &data) {
//   this->openData(dataName);
//   this->getData(data);
//   this->closeData();
// }

// template <typename NumT> void File::readData(const std::string &dataName, NumT &data) {
//   std::vector<NumT> dataVector;
//   this->openData(dataName);
//   this->getData(dataVector);
//   if (dataVector.size() > 0)
//     data = dataVector[0];
//   this->closeData();
// }

// void File::readData(const std::string &dataName, std::string &data) {
//   this->openData(dataName);
//   data = this->getStrData();
//   this->closeData();
// }

// bool File::isDataInt() {
//   Info info = this->getInfo();
//   switch (info.type) {
//   case NXnumtype::INT8:
//   case NXnumtype::UINT8:
//   case NXnumtype::INT16:
//   case NXnumtype::UINT16:
//   case NXnumtype::INT32:
//   case NXnumtype::UINT32:
//     return true;
//   default:
//     return false;
//   }
// }

string File::getStrData() {
  string res;
  this->getData<string>(&res);
  return res;
}

Info File::getInfo() {
  // ensure current location is a dataset
  H5::DataSet *dataset = dynamic_cast<H5::DataSet *>(this->getCurrentLocation());
  if (dataset == nullptr) {
    throw Exception("Trying to read info of non-DataSet location\n");
  }
  // get the datatype
  auto dt = dataset->getDataType();
  auto ds = dataset->getSpace();
  std::size_t rank = ds.getSimpleExtentNdims();
  DimArray dims;
  ds.getSimpleExtentDims(dims.data());

  Info info;
  info.type = hdf5ToNXType(dt);
  for (size_t i = 0; i < rank; i++) {
    info.dims.push_back(dims[i]);
  }
  return info;
}

// Entry File::getNextEntry() {
//   // set up temporary variables to get the information
//   NXname name, class_name;
//   NXnumtype datatype;

//   NXstatus status = NAPI_STATUS((*(this->m_pfile_id), name, class_name, &datatype), "NXgetnextentry failed");
//   if (status == NXstatus::NX_OK) {
//     string str_name(name);
//     string str_class(class_name);
//     return Entry(str_name, str_class);
//   } else if (status == NXstatus::NX_EOD) {
//     return EOD_ENTRY;
//   }
// }

// Entries File::getEntries() {
//   Entries result;
//   // this->getEntries(result);
//   return result;
// }

// void File::getEntries(Entries &result) {
//   result.clear();
//   this->initGroupDir();
//   Entry temp;
//   while (true) {
//     temp = this->getNextEntry();
//     if (temp == EOD_ENTRY) {
//       break;
//     } else {
//       result.insert(temp);
//     }
//   }
// }

// void File::getSlab(void *data, const vector<int> &start, const vector<int> &size) {
//   this->getSlab(data, toDimSize(start), toDimSize(size));
// }

// void File::getSlab(void *data, const DimSizeVector &start, const DimSizeVector &size) {
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

// AttrInfo File::getNextAttr() {
//   // string & name, int & length, NXnumtype type) {
//   NXname name;
//   NXnumtype type;

//   int rank;
//   int dim[NX_MAXRANK];
//   NXstatus status = NAPI_STATUS(
//     NXgetnextattra(*(this->m_pfile_id), name, &rank, dim, &type),
//     "NXgetnextattra failed"
//   );
//   if (status == NXstatus::NX_OK) {
//     AttrInfo info;
//     info.type = type;
//     info.name = string(name);

//     // scalar value
//     if (rank == 0 || (rank == 1 && dim[0] == 1)) {
//       info.length = 1;
//       return info;
//     }

//     // char (=string) or number array (1 dim)
//     if (rank == 1) {
//       info.length = static_cast<unsigned int>(dim[0]);
//       return info;
//     }

//     // string array (2 dim char array)
//     if (rank == 2 && type == NXnumtype::CHAR) {
//       info.length = 1;
//       for (int d = 0; d < rank; ++d) {
//         info.dims.push_back(dim[d]);
//         info.length *= static_cast<unsigned int>(dim[d]);
//       }
//       return info;
//     }

//     // TODO - AttrInfo cannot handle more complex ranks/dimensions, we need to throw an error
//     std::cerr << "ERROR iterating through attributes found array attribute not understood by this api" << std::endl;
//     throw Exception("getNextAttr failed", NXstatus::NX_ERROR);

//   } else if (status == NXstatus::NX_EOD) {
//     AttrInfo info;
//     info.name = NULL_STR;
//     info.length = 0;
//     info.type = NXnumtype::BINARY; // junk value that shouldn't be checked for
//     return info;
//   }
// }

void File::getAttr(const AttrInfo &info, void *data, int length) {
  // char name[NX_MAXNAMELEN];
  // strcpy(name, info.name.c_str());
  // NXnumtype type = info.type;
  // if (length < 0) {
  //   length = static_cast<int>(info.length);
  // }
  // CALL_NAPI(NXgetattr(*(this->m_pfile_id), name, data, &length, &type), "NXgetattr(" + info.name + ") failed");
  // if (type != info.type) {
  //   stringstream msg;
  //   msg << "NXgetattr(" << info.name << ") changed type [" << info.type << "->" << type << "]";
  //   throw Exception(msg.str());
  // }
  // // char attributes are always NULL terminated and so may change length
  // if (static_cast<unsigned>(length) != info.length && type != NXnumtype::CHAR) {
  //   stringstream msg;
  //   msg << "NXgetattr(" << info.name << ") change length [" << info.length << "->" << length << "]";
  //   throw Exception(msg.str());
  // }
}

template <typename NumT> NumT File::getAttr(const AttrInfo &info) {
  NumT value;
  this->getAttr(info, &value);
  return value;
}

template <> MANTID_NEXUS_DLL void File::getAttr(const std::string &name, std::string &value) {
  AttrInfo info;
  info.type = getType<char>();
  info.length = 2000; ///< @todo need to find correct length of attribute
  info.name = name;
  value = this->getStrAttr(info);
}

template <typename NumT> void File::getAttr(const std::string &name, NumT &value) {
  AttrInfo info;
  info.type = getType<NumT>();
  info.length = 1;
  info.name = name;
  value = this->getAttr<NumT>(info);
}

string File::getStrAttr(const AttrInfo &info) {
  string res;
  if (info.type != NXnumtype::CHAR) {
    stringstream msg;
    msg << "getStrAttr only works with strings (type=" << NXnumtype::CHAR << ") found type=" << info.type;
    throw Exception(msg.str());
  }
  char *value = new char[info.length + 1];
  try {
    this->getAttr(info, value, static_cast<int>(info.length) + 1);
  } catch (const Exception &) {
    // Avoid memory leak
    delete[] value;
    throw; // rethrow original exception
  }

  // res = string(value, info.length);
  // allow the constructor to find the ending point of the string. Janik Zikovsky, sep 22, 2010
  res = string(value);
  delete[] value;

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

// bool File::hasAttr(const std::string &name) {
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

// NXlink File::getGroupID() {
//   NXlink link;
//   CALL_NAPI(NXgetgroupID(*(this->m_pfile_id), &link), "NXgetgroupID failed");
//   return link;
// }

// void File::initGroupDir() { CALL_NAPI(NXinitgroupdir(*(this->m_pfile_id)), "NXinitgroupdir failed"); }

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
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const float value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const double value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const int8_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const uint8_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const int16_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const uint16_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const int32_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const uint32_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const int64_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const uint64_t value);
template MANTID_NEXUS_DLL void File::putAttr(const string &name, const char value);

template MANTID_NEXUS_DLL float File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL double File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL int8_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL uint8_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL int16_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL uint16_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL int32_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL uint32_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL int64_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL uint64_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUS_DLL char File::getAttr(const AttrInfo &info);

template MANTID_NEXUS_DLL void File::makeData(const string &name, const NXnumtype type, const int length,
                                              bool open_data);
template MANTID_NEXUS_DLL void File::makeData(const string &name, const NXnumtype type, const int64_t length,
                                              bool open_data);
template MANTID_NEXUS_DLL void File::makeData(const string &name, const NXnumtype type, const size_t length,
                                              bool open_data);

template MANTID_NEXUS_DLL void File::putData(const int *data);
template MANTID_NEXUS_DLL void File::putData(const int64_t *data);
template MANTID_NEXUS_DLL void File::putData(const size_t *data);
template MANTID_NEXUS_DLL void File::putData(const float *data);
template MANTID_NEXUS_DLL void File::putData(const double *data);
template MANTID_NEXUS_DLL void File::putData(const char *data);
template MANTID_NEXUS_DLL void File::putData(const string *data);

template MANTID_NEXUS_DLL void File::putData(const vector<int> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<int64_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<size_t> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<float> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<double> &data);
template MANTID_NEXUS_DLL void File::putData(const vector<string> &data);

// template MANTID_NEXUS_DLL void File::writeData(const string &name, const float &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const double &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const int8_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const uint8_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const int16_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const uint16_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const int32_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const uint32_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const int64_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const uint64_t &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const char &value);

// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<float> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<double> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int8_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint8_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int16_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint16_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int32_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint32_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int64_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint64_t> &value);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<char> &value);

// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<float> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<double> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int8_t> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint8_t> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int16_t> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint16_t> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int32_t> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint32_t> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<int64_t> &value,
//                                                const std::vector<int> &dims);
// template MANTID_NEXUS_DLL void File::writeData(const string &name, const vector<uint64_t> &value,
//                                                const std::vector<int> &dims);

// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<float> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<double> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int8_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint8_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int16_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint16_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int32_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint32_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int64_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint64_t> &value);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<char> &value);

// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<float> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<double> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int8_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint8_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int16_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint16_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int32_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint32_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int64_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint64_t> &value,
//                                                          const dimsize_t chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<char> &value,
//                                                          const dimsize_t chunk);

// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<float> &value, DimVector
// &dims,
//                                                          DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<double> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int8_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint8_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int16_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint16_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int32_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint32_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<int64_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<uint64_t> &value,
//                                                          DimVector &dims, DimSizeVector &chunk);
// template MANTID_NEXUS_DLL void File::writeExtendibleData(const string &name, std::vector<char> &value, DimVector
// &dims,
//                                                          DimSizeVector &chunk);

// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<float> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<double> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int8_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint8_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int16_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint16_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int32_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint32_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int64_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint64_t> &value);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<char> &value);

// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<float> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<double> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int8_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint8_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int16_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint16_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int32_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint32_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<int64_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<uint64_t> &value, DimVector &dims);
// template MANTID_NEXUS_DLL void File::writeUpdatedData(const string &name, vector<char> &value, DimVector &dims);

// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<float> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<float> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<double> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<double> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int8_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int8_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint8_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint8_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int16_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int16_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint16_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint16_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int32_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int32_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint32_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint32_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int64_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<int64_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint64_t> &value,
//                                                    const vector<int> &dims, const NXcompression comp,
//                                                    const vector<int> &bufsize);
// template MANTID_NEXUS_DLL void File::writeCompData(const string &name, const vector<uint64_t> &value,
//                                                    const DimVector &dims, const NXcompression comp,
//                                                    const DimSizeVector &bufsize);

template MANTID_NEXUS_DLL void File::getData(int *data);
template MANTID_NEXUS_DLL void File::getData(int64_t *data);
template MANTID_NEXUS_DLL void File::getData(size_t *data);
template MANTID_NEXUS_DLL void File::getData(float *data);
template MANTID_NEXUS_DLL void File::getData(double *data);
template MANTID_NEXUS_DLL void File::getData(char *data);
template MANTID_NEXUS_DLL void File::getData(string *data);

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

// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<float> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<double> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int8_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint8_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int16_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint16_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int32_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint32_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<int64_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<uint64_t> &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, vector<char> &data);

// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, float &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, double &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int8_t &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint8_t &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int16_t &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint16_t &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int32_t &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint32_t &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, int64_t &data);
// template MANTID_NEXUS_DLL void File::readData(const std::string &dataName, uint64_t &data);

// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<float> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<double> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int8_t> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint8_t> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int16_t> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint16_t> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int32_t> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint32_t> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int64_t> &data, int start, int size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint64_t> &data, int start, int size);

// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<float> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<double> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int8_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint8_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int16_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint16_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int32_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint32_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<int64_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);
// template MANTID_NEXUS_DLL void File::putSlab(const std::vector<uint64_t> &data, const DimSizeVector &start,
//                                              const DimSizeVector &size);

template MANTID_NEXUS_DLL void File::getAttr(const AttrInfo &info, int*, int length);
template MANTID_NEXUS_DLL void File::getAttr(const AttrInfo &info, double*, int length);
template MANTID_NEXUS_DLL void File::getAttr(const AttrInfo &info, char*, int length);

template MANTID_NEXUS_DLL void File::getAttr(const std::string &name, double &value);
template MANTID_NEXUS_DLL void File::getAttr(const std::string &name, int &value);
