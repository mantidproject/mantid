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

using namespace NeXus;
using std::string;
using std::stringstream;
using std::vector;

typedef std::array<hsize_t, NX_MAXRANK> DimArray;

/**
 * \file NeXusFile.cpp
 * The implementation of the NeXus C++ API
 */

namespace { // anonymous namespace to keep it in the file

constexpr std::string group_class_spec("NX_class");
constexpr std::string target_attr_name("target");

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

static H5::FileAccPropList defaultFileAcc() {
  H5::FileAccPropList access_plist;
  access_plist.setFcloseDegree(H5F_CLOSE_STRONG);
  return access_plist;
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

} // end of anonymous namespace

namespace NeXus {

// catch for undefined types
template <typename NumT> NXnumtype getType(NumT const number) {
  stringstream msg;
  msg << "NeXus::getType() does not know type of " << typeid(number).name();
  throw Exception(msg.str());
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
    : H5File(filename, access, defaultFileAcc()), m_filename(filename), m_access(access), m_close_handle(true) {
  this->m_stack.push_back(nullptr);
};

File::File(char const *filename, NXaccess const access)
    : H5File(filename, access, defaultFileAcc()), m_filename(filename), m_access(access), m_close_handle(true) {
  this->m_stack.push_back(nullptr);
};

// copy constructors

File::File(File const &f)
    : H5File(f), m_filename(f.m_filename), m_access(f.m_access), m_stack(file.m_stack), m_close_handle(false) {}

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

void File::makeGroup(std::string const &name, std::string const &class_name, bool open_group) {
  if (name.empty()) {
    throw Exception("Supplied empty name to makeGroup", m_filename);
  }
  if (class_name.empty()) {
    throw Exception("Supplied empty class name to makeGroup", m_filename);
  }
  NAPI_CALL(NXmakegroup(*(this->m_pfile_id), name.c_str(), class_name.c_str()),
            "NXmakegroup(" + name + ", " + class_name + ") failed");
  if (open_group) {
    this->openGroup(name, class_name);
  }
}

void File::openGroup(std::string const &name, std::string const &class_name) {
  if (name.empty()) {
    throw Exception("Supplied empty name to openGroup", m_filename);
  }
  if (class_name.empty()) {
    throw Exception("Supplied empty class name to openGroup", m_filename);
  }
  NAPI_CALL(NXopengroup(*(this->m_pfile_id), name.c_str(), class_name.c_str()),
            "NXopengroup(" + name + ", " + class_name + ") failed");
}

void File::openPath(const string &path) {
  if (path.empty()) {
    throw Exception("Supplied empty path to openPath", m_filename);
  }
  NAPI_CALL(NXopenpath(*(this->m_pfile_id), path.c_str()), "NXopenpath(" + path + ") failed");
}

void File::openGroupPath(const string &path) {
  if (path.empty()) {
    throw Exception("Supplied empty path to openGroupPath", m_filename);
  }
  NAPI_CALL(NXopengrouppath(*(this->m_pfile_id), path.c_str()), "NXopengrouppath(" + path + ") failed");
}

std::string File::getPath() {
  char cPath[2048];

  memset(cPath, 0, sizeof(cPath));
  NAPI_CALL(NXgetpath(*(this->m_pfile_id), cPath, sizeof(cPath) - 1), "NXgetpath() failed");
  std::string path(cPath);
  // openPath expects "/" to open root
  // for consitency, this should return "/" at the root
  if (path == "") {
    path = "/";
  }
  return path;
}

void File::closeGroup() { NAPI_CALL(NXclosegroup(*(this->m_pfile_id.get())), "NXclosegroup failed"); }

void File::makeData(const string &name, NXnumtype type, const DimVector &dims, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw Exception("Supplied empty label to makeData", m_filename);
  }
  if (dims.empty()) {
    throw Exception("Supplied empty dimensions to makeData", m_filename);
  }

  // do the work
  NXstatus status = NXmakedata64(*(this->m_pfile_id), name.c_str(), type, static_cast<int>(dims.size()),
                                 const_cast<int64_t *>(dims.data()));
  // report errors
  NAPI_CALL(status, "NXmakedata(" + name + ", " + (string)type + ", " + std::to_string(dims.size()) + ", " +
                        toString(dims) + ") failed");

  if (open_data) {
    this->openData(name);
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

  this->putData(my_value.data());

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
  DimVector dims(1, NX_UNLIMITED);
  DimSizeVector chunk_dims(1, chunk);
  // Use chunking without using compression
  this->makeCompData(name, getType<NumT>(), dims, NONE, chunk_dims, true);
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
  this->makeCompData(name, getType<NumT>(), unlim_dims, NONE, chunk, true);
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

void File::makeCompData(std::string const &name, NXnumtype const type, DimVector const &dims, NXcompression const comp,
                        DimSizeVector const &bufsize, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw Exception("Supplied empty name to makeCompData", m_filename);
  }
  if (dims.empty()) {
    throw Exception("Supplied empty dimensions to makeCompData", m_filename);
  }
  if (bufsize.empty()) {
    throw Exception("Supplied empty bufsize to makeCompData", m_filename);
  }
  if (dims.size() != bufsize.size()) {
    stringstream msg;
    msg << "Supplied dims rank=" << dims.size() << " must match supplied bufsize rank=" << bufsize.size()
        << "in makeCompData";
    throw Exception(msg.str(), m_filename);
  }

  // do the work
  int i_type = static_cast<int>(type);
  int i_comp = static_cast<int>(comp);
  NXstatus status = NXcompmakedata64(*(this->m_pfile_id), name.c_str(), type, static_cast<int>(dims.size()),
                                     const_cast<int64_t *>(&(dims[0])), i_comp, const_cast<int64_t *>(&(bufsize[0])));

  // report errors
  if (status != NXstatus::NX_OK) {
    stringstream msg;
    msg << "NXcompmakedata64(" << name << ", " << i_type << ", " << dims.size() << ", " << toString(dims) << ", "
        << comp << ", " << toString(bufsize) << ") failed";
    throw Exception(msg.str(), m_filename);
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
    throw Exception("Supplied empty name to openData", m_filename);
  }
  NAPI_CALL(NXopendata(*(this->m_pfile_id), name.c_str()), "NXopendata(" + name + ") failed");
}

void File::closeData() { NAPI_CALL(NXclosedata(*(this->m_pfile_id).get()), "NXclosedata() failed"); }

template <typename NumT> void File::putData(NumT const *data) {
  if (data == NULL) {
    throw Exception("Data specified as null in putData", m_filename);
  }
  NAPI_CALL(NXputdata(*(this->m_pfile_id), data), "NXputdata failed");
}

template <typename NumT> void File::putData(vector<NumT> const &data) {
  if (data.empty()) {
    throw Exception("Supplied empty data to putData", m_filename);
  }
  this->putData<NumT>(data.data());
}

void File::putAttr(const AttrInfo &info, const void *data) {
  if (info.name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"", m_filename);
  }
  if (info.name.empty()) {
    throw Exception("Supplied empty name to putAttr", m_filename);
  }
  NAPI_CALL(NXputattr(*(this->m_pfile_id), info.name.c_str(), data, static_cast<int>(info.length), info.type),
            "NXputattr(" + info.name + ", data, " + std::to_string(info.length) + ", " + (string)info.type +
                ") failed");
}

template <typename NumT> void File::putAttr(const std::string &name, const NumT value) {
  AttrInfo info;
  info.name = name;
  info.length = 1;
  info.type = getType<NumT>();
  this->putAttr(info, &value);
}

void File::putAttr(const char *name, const char *value) {
  if (name == NULL) {
    throw Exception("Specified name as null to putAttr", m_filename);
  }
  if (value == NULL) {
    throw Exception("Specified value as null to putAttr", m_filename);
  }
  string s_name(name);
  string s_value(value);
  this->putAttr(s_name, s_value);
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
    throw Exception("Data specified as null in putSlab", m_filename);
  }
  if (start.empty()) {
    throw Exception("Supplied empty start to putSlab", m_filename);
  }
  if (size.empty()) {
    throw Exception("Supplied empty size to putSlab", m_filename);
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "Supplied start rank=" << start.size() << " must match supplied size rank=" << size.size() << "in putSlab";
    throw Exception(msg.str(), m_filename);
  }
  NXstatus status = NXputslab64(*(this->m_pfile_id), data, &(start[0]), &(size[0]));
  if (status != NXstatus::NX_OK) {
    stringstream msg;
    msg << "NXputslab64(data, " << toString(start) << ", " << toString(size) << ") failed";
    throw Exception(msg.str(), m_filename);
  }
}

template <typename NumT>
void File::putSlab(vector<NumT> const &data, DimSizeVector const &start, DimSizeVector const &size) {
  if (data.empty()) {
    throw Exception("Supplied empty data to putSlab", m_filename);
  }
  this->putSlab(&(data[0]), start, size);
}

template <typename NumT> void File::putSlab(const vector<NumT> &data, const dimsize_t start, const dimsize_t size) {
  DimSizeVector start_v(1, start);
  DimSizeVector size_v(1, size);
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
  H5::DataSet *current = dynamic_cast<H5::DataSet *>(m_stack.back());
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
    throw Exception("Supplied null pointer to getData", m_filename);
  }
  NAPI_CALL(NXgetdata(*(this->m_pfile_id), data), "NXgetdata failed");
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

template <typename NumT> void File::getDataCoerce(vector<NumT> &data) {
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
    throw Exception("NexusFile::getDataCoerce(): Could not coerce to double.", m_filename);
  }
}

template <typename NumT> void File::readData(const std::string &dataName, std::vector<NumT> &data) {
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

Info File::getInfo() {
  int64_t dims[NX_MAXRANK];
  NXnumtype type;
  int rank;
  NAPI_CALL(NXgetinfo64(*(this->m_pfile_id), &rank, dims, &type), "NXgetinfo failed");
  Info info;
  info.type = static_cast<NXnumtype>(type);
  for (int i = 0; i < rank; i++) {
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

template <typename NumT> void File::getSlab(NumT *data, const DimSizeVector &start, const DimSizeVector &size) {
  if (data == NULL) {
    throw Exception("Supplied null pointer to getSlab", m_filename);
  }
  if (start.size() == 0) {
    stringstream msg;
    msg << "Supplied empty start offset, rank = " << start.size() << " in getSlab";
    throw Exception(msg.str(), m_filename);
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "In getSlab start rank=" << start.size() << " must match size rank=" << size.size();
    throw Exception(msg.str(), m_filename);
  }

  NAPI_CALL(NXgetslab64(*(this->m_pfile_id), data, &(start[0]), &(size[0])), "NXgetslab failed");
}

AttrInfo File::getNextAttr() {
  // string & name, int & length, NXnumtype type) {
  NXname name;
  NXnumtype type;

  int rank;
  int dim[NX_MAXRANK];
  NXstatus status = NXgetnextattra(*(this->m_pfile_id), name, &rank, dim, &type);
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
      for (int d = 0; d < rank; ++d) {
        info.dims.push_back(dim[d]);
        info.length *= static_cast<unsigned int>(dim[d]);
      }
      return info;
    }

    // TODO - AttrInfo cannot handle more complex ranks/dimensions, we need to throw an error
    std::cerr << "ERROR iterating through attributes found array attribute not understood by this api" << std::endl;
    throw Exception("getNextAttr failed", m_filename);

  } else if (status == NXstatus::NX_EOD) {
    AttrInfo info;
    info.name = NULL_STR;
    info.length = 0;
    info.type = NXnumtype::BINARY; // junk value that shouldn't be checked for
    return info;
  } else {
    throw Exception("NXgetnextattra failed", m_filename);
  }
}

void File::getAttr(const AttrInfo &info, void *data, int length) {
  char name[NX_MAXNAMELEN];
  strcpy(name, info.name.c_str());
  NXnumtype type = info.type;
  if (length < 0) {
    length = static_cast<int>(info.length);
  }
  NAPI_CALL(NXgetattr(*(this->m_pfile_id), name, data, &length, &type), "NXgetattr(" + info.name + ") failed");
  if (type != info.type) {
    stringstream msg;
    msg << "NXgetattr(" << info.name << ") changed type [" << info.type << "->" << type << "]";
    throw Exception(msg.str(), m_filename);
  }
  // char attributes are always NULL terminated and so may change length
  if (static_cast<unsigned>(length) != info.length && type != NXnumtype::CHAR) {
    stringstream msg;
    msg << "NXgetattr(" << info.name << ") change length [" << info.length << "->" << length << "]";
    throw Exception(msg.str(), m_filename);
  }
}

template <typename NumT> NumT File::getAttr(std::string const &name) {
  NumT ret;
  this->getAttr<NumT>(name, ret);
  return ret;
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
    throw Exception(msg.str(), m_filename);
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
