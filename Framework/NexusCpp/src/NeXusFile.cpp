#include <cstring>
// REMOVE
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"
#include <iostream>
#include <numeric>
#include <sstream>
#include <typeinfo>

using namespace NeXus;
using std::map;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;

/**
 * \file NeXusFile.cpp
 * The implementation of the NeXus C++ API
 */

static const string NULL_STR = "NULL";

namespace { // anonymous namespace to keep it in the file
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

static vector<int64_t> toInt64(const vector<int> &small_v) {
  // copy the dims over to call the int64_t version
  return vector<int64_t>(small_v.begin(), small_v.end());
}

} // end of anonymous namespace

namespace NeXus {

// catch for undefined types
template <typename NumT> NXnumtype getType(NumT number) {
  stringstream msg;
  msg << "NeXus::getType() does not know type of " << typeid(number).name();
  throw Exception(msg.str());
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(char number) {
  (void)number; // Avoid compiler warning
  return CHAR;
}

// template specialisations for types we know
template <> MANTID_NEXUSCPP_DLL NXnumtype getType(float number) {
  (void)number; // Avoid compiler warning
  return FLOAT32;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(double number) {
  (void)number; // Avoid compiler warning
  return FLOAT64;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(int8_t number) {
  (void)number; // Avoid compiler warning
  return INT8;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(uint8_t number) {
  (void)number; // Avoid compiler warning
  return UINT8;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(int16_t number) {
  (void)number; // Avoid compiler warning
  return INT16;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(uint16_t number) {
  (void)number; // Avoid compiler warning
  return UINT16;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(int32_t number) {
  (void)number; // Avoid compiler warning
  return INT32;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(uint32_t number) {
  (void)number; // Avoid compiler warning
  return UINT32;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(int64_t number) {
  (void)number; // Avoid compiler warning
  return INT64;
}

template <> MANTID_NEXUSCPP_DLL NXnumtype getType(uint64_t number) {
  (void)number; // Avoid compiler warning
  return UINT64;
}

} // namespace NeXus

// check type sizes - uses a trick that you cannot allocate an
// array of negative length
#ifdef _MSC_VER
#define ARRAY_OFFSET 1 /* cannot dimension an array with zero elements */
#else
#define ARRAY_OFFSET 0 /* can dimension an array with zero elements */
#endif                 /* _MSC_VER */

/*
static int check_float_too_big[4 - sizeof(float) + ARRAY_OFFSET]; // error if float > 4 bytes
static int check_float_too_small[sizeof(float) - 4 + ARRAY_OFFSET]; // error if float < 4 bytes
static int check_double_too_big[8 - sizeof(double) + ARRAY_OFFSET]; // error if double > 8 bytes
static int check_double_too_small[sizeof(double) - 8 + ARRAY_OFFSET]; // error if double < 8 bytes
static int check_char_too_big[1 - sizeof(char) + ARRAY_OFFSET]; // error if char > 1 byte
*/

namespace {

static void inner_malloc(void *&data, const std::vector<int64_t> &dims, NXnumtype type) {
  const auto rank = dims.size();
  int64_t c_dims[NX_MAXRANK];
  for (size_t i = 0; i < rank; i++) {
    c_dims[i] = dims[i];
  }
  NXstatus status = NXmalloc64(&data, static_cast<int>(rank), c_dims, type);
  if (status != NX_OK) {
    throw Exception("NXmalloc failed", status);
  }
}

static void inner_free(void *&data) {
  NXstatus status = NXfree(&data);
  if (status != NX_OK) {
    throw Exception("NXfree failed", status);
  }
}

} // end of anonymous namespace

namespace NeXus {
File::File(NXhandle handle, bool close_handle) : m_file_id(handle), m_close_handle(close_handle) {}

File::File(const string &filename, const NXaccess access) : m_close_handle(true) {
  this->initOpenFile(filename, access);
}

File::File(const char *filename, const NXaccess access) : m_close_handle(true) {
  this->initOpenFile(string(filename), access);
}

void File::initOpenFile(const string &filename, const NXaccess access) {
  if (filename.empty()) {
    throw Exception("Filename specified is empty constructor");
  }

  NXstatus status = NXopen(filename.c_str(), access, &(this->m_file_id));
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXopen(" << filename << ", " << access << ") failed";
    throw Exception(msg.str(), status);
  }
}

File::~File() {
  if (m_close_handle && m_file_id != NULL) {
    NXstatus status = NXclose(&(this->m_file_id));
    this->m_file_id = NULL;
    if (status != NX_OK) {
      stringstream msg;
      msg << "NXclose failed with status: " << status << "\n";
      NXReportError(const_cast<char *>(msg.str().c_str()));
    }
  }
}

void File::close() {
  if (this->m_file_id != NULL) {
    NXstatus status = NXclose(&(this->m_file_id));
    this->m_file_id = NULL;
    if (status != NX_OK) {
      throw Exception("NXclose failed", status);
    }
  }
}

void File::flush() {
  NXstatus status = NXflush(&(this->m_file_id));
  if (status != NX_OK) {
    throw Exception("NXflush failed", status);
  }
}

void File::makeGroup(const string &name, const string &class_name, bool open_group) {
  if (name.empty()) {
    throw Exception("Supplied empty name to makeGroup");
  }
  if (class_name.empty()) {
    throw Exception("Supplied empty class name to makeGroup");
  }
  NXstatus status = NXmakegroup(this->m_file_id, name.c_str(), class_name.c_str());
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXmakegroup(" << name << ", " << class_name << ") failed";
    throw Exception(msg.str(), status);
  }
  if (open_group) {
    this->openGroup(name, class_name);
  }
}

void File::openGroup(const string &name, const string &class_name) {
  if (name.empty()) {
    throw Exception("Supplied empty name to openGroup");
  }
  if (class_name.empty()) {
    throw Exception("Supplied empty class name to openGroup");
  }
  NXstatus status = NXopengroup(this->m_file_id, name.c_str(), class_name.c_str());
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXopengroup(" << name << ", " << class_name << ") failed";
    throw Exception(msg.str(), status);
  }
}

void File::openPath(const string &path) {
  if (path.empty()) {
    throw Exception("Supplied empty path to openPath");
  }
  NXstatus status = NXopenpath(this->m_file_id, path.c_str());
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXopenpath(" << path << ") failed";
    throw Exception(msg.str(), status);
  }
}

void File::openGroupPath(const string &path) {
  if (path.empty()) {
    throw Exception("Supplied empty path to openGroupPath");
  }
  NXstatus status = NXopengrouppath(this->m_file_id, path.c_str());
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXopengrouppath(" << path << ") failed";
    throw Exception(msg.str(), status);
  }
}

std::string File::getPath() {
  char cPath[2048];

  memset(cPath, 0, sizeof(cPath));
  NXstatus status = NXgetpath(this->m_file_id, cPath, sizeof(cPath) - 1);
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXgetpath() failed";
    throw Exception(msg.str(), status);
  }
  return std::string(cPath);
}

void File::closeGroup() {
  NXstatus status = NXclosegroup(this->m_file_id);
  if (status != NX_OK) {
    throw Exception("NXclosegroup failed", status);
  }
}

void File::makeData(const string &name, NXnumtype type, const vector<int> &dims, bool open_data) {
  this->makeData(name, type, toInt64(dims), open_data);
}

void File::makeData(const string &name, NXnumtype type, const vector<int64_t> &dims, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw Exception("Supplied empty label to makeData");
  }
  if (dims.empty()) {
    throw Exception("Supplied empty dimensions to makeData");
  }

  // do the work
  NXstatus status = NXmakedata64(this->m_file_id, name.c_str(), static_cast<int>(type), static_cast<int>(dims.size()),
                                 const_cast<int64_t *>(&(dims[0])));
  // report errors
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXmakedata(" << name << ", " << type << ", " << dims.size() << ", " << toString(dims) << ") failed";
    throw Exception(msg.str(), status);
  }
  if (open_data) {
    this->openData(name);
  }
}

template <typename NumT>
void File::makeData(const string &name, const NXnumtype type, const NumT length, bool open_data) {
  vector<int64_t> dims;
  dims.push_back(static_cast<int64_t>(length));
  this->makeData(name, type, dims, open_data);
}

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
  this->makeData(name, CHAR, dims, true);

  this->putData(&(my_value[0]));

  this->closeData();
}

template <typename NumT> void File::writeData(const string &name, const vector<NumT> &value) {
  vector<int64_t> dims(1, static_cast<int64_t>(value.size()));
  this->writeData(name, value, dims);
}

template <typename NumT> void File::writeData(const string &name, const vector<NumT> &value, const vector<int> &dims) {
  this->makeData(name, getType<NumT>(), dims, true);
  this->putData(value);
  this->closeData();
}

template <typename NumT>
void File::writeData(const string &name, const vector<NumT> &value, const vector<int64_t> &dims) {
  this->makeData(name, getType<NumT>(), dims, true);
  this->putData(value);
  this->closeData();
}

template <typename NumT> void File::writeExtendibleData(const string &name, vector<NumT> &value) {
  // Use a default chunk size of 4096 bytes. TODO: Is this optimal?
  writeExtendibleData(name, value, 4096);
}

template <typename NumT> void File::writeExtendibleData(const string &name, vector<NumT> &value, const int64_t chunk) {
  vector<int64_t> dims(1, NX_UNLIMITED);
  vector<int64_t> chunk_dims(1, chunk);
  // Use chunking without using compression
  this->makeCompData(name, getType<NumT>(), dims, NONE, chunk_dims, true);
  this->putSlab(value, int64_t(0), int64_t(value.size()));
  this->closeData();
}

template <typename NumT>
void File::writeExtendibleData(const string &name, vector<NumT> &value, vector<int64_t> &dims,
                               std::vector<int64_t> &chunk) {
  // Create the data with unlimited 0th dimensions
  std::vector<int64_t> unlim_dims(dims);
  unlim_dims[0] = NX_UNLIMITED;
  // Use chunking without using compression
  this->makeCompData(name, getType<NumT>(), unlim_dims, NONE, chunk, true);
  // And put that slab of that of that given size in there
  std::vector<int64_t> start(dims.size(), 0);
  this->putSlab(value, start, dims);
  this->closeData();
}

template <typename NumT> void File::writeUpdatedData(const std::string &name, std::vector<NumT> &value) {
  this->openData(name);
  this->putSlab(value, int64_t(0), int64_t(value.size()));
  this->closeData();
}

template <typename NumT>
void File::writeUpdatedData(const std::string &name, std::vector<NumT> &value, std::vector<int64_t> &dims) {
  this->openData(name);
  std::vector<int64_t> start(dims.size(), 0);
  this->putSlab(value, start, dims);
  this->closeData();
}

void File::makeCompData(const string &name, const NXnumtype type, const vector<int> &dims, const NXcompression comp,
                        const vector<int> &bufsize, bool open_data) {
  this->makeCompData(name, type, toInt64(dims), comp, toInt64(bufsize), open_data);
}

void File::makeCompData(const string &name, const NXnumtype type, const vector<int64_t> &dims, const NXcompression comp,
                        const vector<int64_t> &bufsize, bool open_data) {
  // error check the parameters
  if (name.empty()) {
    throw Exception("Supplied empty name to makeCompData");
  }
  if (dims.empty()) {
    throw Exception("Supplied empty dimensions to makeCompData");
  }
  if (bufsize.empty()) {
    throw Exception("Supplied empty bufsize to makeCompData");
  }
  if (dims.size() != bufsize.size()) {
    stringstream msg;
    msg << "Supplied dims rank=" << dims.size() << " must match supplied bufsize rank=" << bufsize.size()
        << "in makeCompData";
    throw Exception(msg.str());
  }

  // do the work
  int i_type = static_cast<int>(type);
  int i_comp = static_cast<int>(comp);
  NXstatus status = NXcompmakedata64(this->m_file_id, name.c_str(), i_type, static_cast<int>(dims.size()),
                                     const_cast<int64_t *>(&(dims[0])), i_comp, const_cast<int64_t *>(&(bufsize[0])));

  // report errors
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXcompmakedata64(" << name << ", " << type << ", " << dims.size() << ", " << toString(dims) << ", " << comp
        << ", " << toString(bufsize) << ") failed";
    throw Exception(msg.str(), status);
  }
  if (open_data) {
    this->openData(name);
  }
}

template <typename NumT>
void File::writeCompData(const string &name, const vector<NumT> &value, const vector<int> &dims,
                         const NXcompression comp, const vector<int> &bufsize) {
  this->writeCompData(name, value, toInt64(dims), comp, toInt64(bufsize));
}

template <typename NumT>
void File::writeCompData(const string &name, const vector<NumT> &value, const vector<int64_t> &dims,
                         const NXcompression comp, const vector<int64_t> &bufsize) {
  this->makeCompData(name, getType<NumT>(), dims, comp, bufsize, true);
  this->putData(value);
  this->closeData();
}

void File::openData(const string &name) {
  if (name.empty()) {
    throw Exception("Supplied empty name to openData");
  }
  NXstatus status = NXopendata(this->m_file_id, name.c_str());
  if (status != NX_OK) {
    throw Exception("NXopendata(" + name + ") failed", status);
  }
}

void File::closeData() {
  NXstatus status = NXclosedata(this->m_file_id);
  if (status != NX_OK) {
    throw Exception("NXclosedata() failed", status);
  }
}

void File::putData(const void *data) {
  if (data == NULL) {
    throw Exception("Data specified as null in putData");
  }
  NXstatus status = NXputdata(this->m_file_id, const_cast<void *>(data));
  if (status != NX_OK) {
    throw Exception("NXputdata(void *) failed", status);
  }
}

template <typename NumT> void File::putData(const vector<NumT> &data) {
  if (data.empty()) {
    throw Exception("Supplied empty data to putData");
  }
  this->putData(&(data[0]));
}

void File::putAttr(const AttrInfo &info, const void *data) {
  if (info.name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (info.name.empty()) {
    throw Exception("Supplied empty name to putAttr");
  }
  NXstatus status =
      NXputattr(this->m_file_id, info.name.c_str(), data, static_cast<int>(info.length), static_cast<int>(info.type));
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXputattr(" << info.name << ", data, " << info.length << ", " << info.type << ") failed";
    throw Exception(msg.str(), status);
  }
}

void File::putAttr(const std::string &name, const std::vector<std::string> &array) {
  if (name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (name.empty()) {
    throw Exception("Supplied empty name to putAttr");
  }

  size_t maxLength = 0;
  for (std::vector<std::string>::const_iterator it = array.begin(); it != array.end(); ++it) {
    if (maxLength < it->size()) {
      maxLength = it->size();
    }
  }

  // fill data
  std::string data(maxLength * array.size(), '\0');
  std::size_t pos = 0;
  for (std::vector<std::string>::const_iterator it = array.begin(); it != array.end(); ++it, pos += maxLength) {
    if (!(it->empty())) {
      data.replace(pos, it->size(), *it);
    }
  }

  // set rank and dim
  const int rank = 2;
  const int dim[rank] = {static_cast<int>(array.size()), static_cast<int>(maxLength)};

  // write data
  NXstatus status = NXputattra(this->m_file_id, name.c_str(), data.c_str(), rank, dim, CHAR);

  if (status != NX_OK) {
    stringstream msg;
    msg << "NXputattra(" << name << ", data, " << rank << ", [" << dim[0] << ", " << dim[1] << "], CHAR) failed";
    throw Exception(msg.str(), status);
  }
}

template <typename NumT> void File::putAttr(const std::string &name, const std::vector<NumT> &array) {
  if (name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (name.empty()) {
    throw Exception("Supplied empty name to putAttr");
  }

  // set rank and dim
  const int rank = 1;
  int dim[rank] = {static_cast<int>(array.size())};

  // write data
  NXnumtype type = getType<NumT>();
  NXstatus status = NXputattra(this->m_file_id, name.c_str(), &(array[0]), rank, dim, type);

  if (status != NX_OK) {
    stringstream msg;
    msg << "NXputattra(" << name << ", data, " << rank << ", [" << dim[0] << "], " << type << ") failed";
    throw Exception(msg.str(), status);
  }
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
    throw Exception("Specified name as null to putAttr");
  }
  if (value == NULL) {
    throw Exception("Specified value as null to putAttr");
  }
  string s_name(name);
  string s_value(value);
  this->putAttr(s_name, s_value);
}

void File::putAttr(const std::string &name, const string &value) {
  string my_value(value);
  if (my_value.empty())
    my_value = " "; // Make a default "space" to avoid errors.
  AttrInfo info;
  info.name = name;
  info.length = static_cast<unsigned int>(my_value.size());
  info.type = CHAR;
  this->putAttr(info, &(my_value[0]));
}

void File::putSlab(const void *data, const vector<int> &start, const vector<int> &size) {
  vector<int64_t> start_big = toInt64(start);
  vector<int64_t> size_big = toInt64(size);
  this->putSlab(data, start_big, size_big);
}

void File::putSlab(const void *data, const vector<int64_t> &start, const vector<int64_t> &size) {
  if (data == NULL) {
    throw Exception("Data specified as null in putSlab");
  }
  if (start.empty()) {
    throw Exception("Supplied empty start to putSlab");
  }
  if (size.empty()) {
    throw Exception("Supplied empty size to putSlab");
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "Supplied start rank=" << start.size() << " must match supplied size rank=" << size.size() << "in putSlab";
    throw Exception(msg.str());
  }
  NXstatus status = NXputslab64(this->m_file_id, data, &(start[0]), &(size[0]));
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXputslab64(data, " << toString(start) << ", " << toString(size) << ") failed";
    throw Exception(msg.str(), status);
  }
}

template <typename NumT>
void File::putSlab(const vector<NumT> &data, const vector<int> &start, const vector<int> &size) {
  vector<int64_t> start_big = toInt64(start);
  vector<int64_t> size_big = toInt64(size);
  this->putSlab(data, start_big, size_big);
}

template <typename NumT>
void File::putSlab(const vector<NumT> &data, const vector<int64_t> &start, const vector<int64_t> &size) {
  if (data.empty()) {
    throw Exception("Supplied empty data to putSlab");
  }
  this->putSlab(&(data[0]), start, size);
}

template <typename NumT> void File::putSlab(const vector<NumT> &data, int start, int size) {
  this->putSlab(data, static_cast<int64_t>(start), static_cast<int64_t>(size));
}

template <typename NumT> void File::putSlab(const vector<NumT> &data, int64_t start, int64_t size) {
  vector<int64_t> start_v(1, start);
  vector<int64_t> size_v(1, size);
  this->putSlab(data, start_v, size_v);
}

NXlink File::getDataID() {
  NXlink link;
  NXstatus status = NXgetdataID(this->m_file_id, &link);
  if (status != NX_OK) {
    throw Exception("NXgetdataID failed", status);
  }
  return link;
}

bool File::isDataSetOpen() {
  NXlink id;
  if (NXgetdataID(this->m_file_id, &id) == NX_ERROR) {
    return false;
  } else {
    return true;
  }
}
/*----------------------------------------------------------------------*/

void File::makeLink(NXlink &link) {
  NXstatus status = NXmakelink(this->m_file_id, &link);
  if (status != NX_OK) {
    throw Exception("NXmakelink failed", status);
  }
}

void File::makeNamedLink(const string &name, NXlink &link) {
  if (name.empty()) {
    throw Exception("Supplied empty name to makeNamedLink");
  }
  NXstatus status = NXmakenamedlink(this->m_file_id, name.c_str(), &link);
  if (status != NX_OK) {
    throw Exception("NXmakenamedlink(" + name + ", link)", status);
  }
}

void File::openSourceGroup() {
  NXstatus status = NXopensourcegroup(this->m_file_id);
  if (status != NX_OK) {
    throw Exception("NXopensourcegroup failed");
  }
}

void File::getData(void *data) {
  if (data == NULL) {
    throw Exception("Supplied null pointer to getData");
  }
  NXstatus status = NXgetdata(this->m_file_id, data);
  if (status != NX_OK) {
    throw Exception("NXgetdata failed", status);
  }
}

template <typename NumT> std::vector<NumT> *File::getData() {
  Info info = this->getInfo();
  if (info.type != getType<NumT>()) {
    throw Exception("NXgetdata failed - invalid vector type");
  }

  // determine the number of elements
  int64_t length = 1;
  for (vector<int64_t>::const_iterator it = info.dims.begin(); it != info.dims.end(); ++it) {
    length *= *it;
  }

  // allocate memory to put the data into
  void *temp;
  inner_malloc(temp, info.dims, info.type);

  // fetch the data
  this->getData(temp);

  // put it in the vector
  vector<NumT> *result =
      new vector<NumT>(static_cast<NumT *>(temp), static_cast<NumT *>(temp) + static_cast<size_t>(length));

  inner_free(temp);
  return result;
}

template <typename NumT> void File::getData(vector<NumT> &data) {
  Info info = this->getInfo();

  if (info.type != getType<NumT>()) {
    throw Exception("NXgetdata failed - invalid vector type");
  }
  // determine the number of elements
  size_t length =
      std::accumulate(info.dims.cbegin(), info.dims.cend(), static_cast<size_t>(1),
                      [](const auto subtotal, const auto &value) { return subtotal * static_cast<size_t>(value); });

  // allocate memory to put the data into
  // need to use resize() rather than reserve() so vector length gets set
  data.resize(length);

  // fetch the data
  this->getData(&(data[0]));
}

void File::getDataCoerce(vector<int> &data) {
  Info info = this->getInfo();
  if (info.type == INT8) {
    vector<int8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == UINT8) {
    vector<uint8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == INT16) {
    vector<int16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == UINT16) {
    vector<uint16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == INT32) {
    vector<int32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == UINT32) {
    vector<uint32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else {
    throw Exception("NexusFile::getDataCoerce(): Could not coerce to int.");
  }
}

void File::getDataCoerce(vector<double> &data) {
  Info info = this->getInfo();
  if (info.type == INT8) {
    vector<int8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == UINT8) {
    vector<uint8_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == INT16) {
    vector<int16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == UINT16) {
    vector<uint16_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == INT32) {
    vector<int32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == UINT32) {
    vector<uint32_t> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == FLOAT32) {
    vector<float> result;
    this->getData(result);
    data.assign(result.begin(), result.end());
  } else if (info.type == FLOAT64) {
    this->getData(data);
  } else {
    throw Exception("NexusFile::getDataCoerce(): Could not coerce to double.");
  }
}

template <typename NumT> void File::readData(const std::string &dataName, std::vector<NumT> &data) {
  this->openData(dataName);
  this->getData(data);
  this->closeData();
}

template <typename NumT> void File::readData(const std::string &dataName, NumT &data) {
  std::vector<NumT> dataVector;
  this->openData(dataName);
  this->getData(dataVector);
  if (dataVector.size() > 0)
    data = dataVector[0];
  this->closeData();
}

void File::readData(const std::string &dataName, std::string &data) {
  this->openData(dataName);
  data = this->getStrData();
  this->closeData();
}

bool File::isDataInt() {
  Info info = this->getInfo();
  switch (info.type) {
  case INT8:
  case UINT8:
  case INT16:
  case UINT16:
  case INT32:
  case UINT32:
    return true;
  default:
    return false;
  }
}

string File::getStrData() {
  string res;
  Info info = this->getInfo();
  if (info.type != NX_CHAR) {
    stringstream msg;
    msg << "Cannot use getStrData() on non-character data. Found type=" << info.type;
    throw Exception(msg.str());
  }
  if (info.dims.size() != 1) {
    stringstream msg;
    msg << "getStrData() only understand rank=1 data. Found rank=" << info.dims.size();
    throw Exception(msg.str());
  }
  char *value = new char[static_cast<size_t>(info.dims[0]) + 1]; // probably do not need +1, but being safe
  try {
    this->getData(value);
  } catch (const Exception &) {
    delete[] value;
    throw; // rethrow the original exception
  }
  res = string(value, static_cast<size_t>(info.dims[0]));
  delete[] value;
  return res;
}

Info File::getInfo() {
  // vector<int> & dims, NXnumtype & type) {
  int64_t dims[NX_MAXRANK];
  int type;
  int rank;
  NXstatus status = NXgetinfo64(this->m_file_id, &rank, dims, &type);
  if (status != NX_OK) {
    throw Exception("NXgetinfo failed", status);
  }
  Info info;
  info.type = static_cast<NXnumtype>(type);
  for (int i = 0; i < rank; i++) {
    info.dims.push_back(dims[i]);
  }
  return info;
}

pair<string, string> File::getNextEntry() {
  // set up temporary variables to get the information
  char name[NX_MAXNAMELEN];
  char class_name[NX_MAXNAMELEN];
  int datatype;

  NXstatus status = NXgetnextentry(this->m_file_id, name, class_name, &datatype);
  if (status == NX_OK) {
    string str_name(name);
    string str_class(class_name);
    return pair<string, string>(str_name, str_class);
  } else if (status == NX_EOD) {
    return pair<string, string>(NULL_STR, NULL_STR); // TODO return the correct thing
  } else {
    throw Exception("NXgetnextentry failed", status);
  }
}

map<string, string> File::getEntries() {
  map<string, string> result;
  this->getEntries(result);
  return result;
}

void File::getEntries(std::map<std::string, std::string> &result) {
  result.clear();
  this->initGroupDir();
  pair<string, string> temp;
  while (true) {
    temp = this->getNextEntry();
    if (temp.first == NULL_STR && temp.second == NULL_STR) { // TODO this needs to be changed when getNextEntry is fixed
      break;
    } else {
      result.insert(temp);
    }
  }
}

void File::getSlab(void *data, const vector<int> &start, const vector<int> &size) {
  this->getSlab(data, toInt64(start), toInt64(size));
}

void File::getSlab(void *data, const vector<int64_t> &start, const vector<int64_t> &size) {
  if (data == NULL) {
    throw Exception("Supplied null pointer to getSlab");
  }
  if (start.size() <= 0) {
    stringstream msg;
    msg << "Supplied empty start offset, rank = " << start.size() << " in getSlab";
    throw Exception(msg.str());
  }
  if (start.size() != size.size()) {
    stringstream msg;
    msg << "In getSlab start rank=" << start.size() << " must match size rank=" << size.size();
    throw Exception(msg.str());
  }

  NXstatus status = NXgetslab64(this->m_file_id, data, &(start[0]), &(size[0]));
  if (status != NX_OK) {
    throw Exception("NXgetslab failed", status);
  }
}

AttrInfo File::getNextAttr() {
  // string & name, int & length, NXnumtype type) {
  char name[NX_MAXNAMELEN];
  int type;

  int rank;
  int dim[NX_MAXRANK];
  NXstatus status = NXgetnextattra(this->m_file_id, name, &rank, dim, &type);
  if (status == NX_OK) {
    AttrInfo info;
    info.type = static_cast<NXnumtype>(type);
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
    if (rank == 2 && type == NX_CHAR) {
      info.length = 1;
      for (int d = 0; d < rank; ++d) {
        info.dims.push_back(dim[d]);
        info.length *= static_cast<unsigned int>(dim[d]);
      }
      return info;
    }

    // TODO - AttrInfo cannot handle more complex ranks/dimensions, we need to throw an error
    std::cerr << "ERROR iterating through attributes found array attribute not understood by this api" << std::endl;
    throw Exception("getNextAttr failed", NX_ERROR);

  } else if (status == NX_EOD) {
    AttrInfo info;
    info.name = NULL_STR;
    info.length = 0;
    info.type = NXnumtype::BINARY; // junk value that shouldn't be checked for
    return info;
  } else {
    throw Exception("NXgetnextattra failed", status);
  }
}

void File::getAttr(const AttrInfo &info, void *data, int length) {
  char name[NX_MAXNAMELEN];
  strcpy(name, info.name.c_str());
  int type = info.type;
  if (length < 0) {
    length = static_cast<int>(info.length);
  }
  NXstatus status = NXgetattr(this->m_file_id, name, data, &length, &type);
  if (status != NX_OK) {
    throw Exception("NXgetattr(" + info.name + ") failed", status);
  }
  if (type != info.type) {
    stringstream msg;
    msg << "NXgetattr(" << info.name << ") changed type [" << info.type << "->" << type << "]";
    throw Exception(msg.str());
  }
  // char attributes are always NULL terminated and so may change length
  if (static_cast<unsigned>(length) != info.length && type != NX_CHAR) {
    stringstream msg;
    msg << "NXgetattr(" << info.name << ") change length [" << info.length << "->" << length << "]";
    throw Exception(msg.str());
  }
}

template <typename NumT> NumT File::getAttr(const AttrInfo &info) {
  NumT value;
  this->getAttr(info, &value);
  return value;
}

template <> MANTID_NEXUSCPP_DLL void File::getAttr(const std::string &name, std::string &value) {
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
  if (info.type != CHAR) {
    stringstream msg;
    msg << "getStrAttr only works with strings (type=" << CHAR << ") found type=" << info.type;
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

void File::getAttr(const std::string &name, std::vector<std::string> &array) {
  if (name == NULL_STR) {
    throw Exception("Supplied bad attribute name \"" + NULL_STR + "\"");
  }
  if (name.empty()) {
    throw Exception("Supplied empty name to getAttr");
  }

  // get attrInfo
  char attr_name[128];
  strcpy(attr_name, name.c_str());

  int type;
  int rank;
  int dim[NX_MAXRANK];
  if (NXgetattrainfo(this->m_file_id, attr_name, &rank, dim, &type) != NX_OK) {
    throw Exception("Attribute \"" + name + "\" not found");
  }

  if (rank != 2 || type != NX_CHAR) {
    throw Exception("Attribute is not an array of strings");
  }

  // read data
  std::string sep(", ");
  char *char_data = new char[static_cast<size_t>(dim[0]) * (static_cast<size_t>(dim[1]) + sep.size())];
  if (NXgetattra(this->m_file_id, attr_name, char_data) != NX_OK)
    throw Exception("Could not iterate to next attribute");

  // split data to strings
  std::string data(char_data);

  std::size_t start = 0;
  std::size_t end = data.find(sep, start);
  while (end != std::string::npos) {
    array.push_back(data.substr(start, (end - start)));
    start = end + sep.size();
    end = data.find(sep, start);
  }
  array.push_back(data.substr(start));
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

NXlink File::getGroupID() {
  NXlink link;
  NXstatus status = NXgetgroupID(this->m_file_id, &link);
  if (status != NX_OK) {
    throw Exception("NXgetgroupID failed", status);
  }
  return link;
}

bool File::sameID(NXlink &first, NXlink &second) {
  NXstatus status = NXsameID(this->m_file_id, &first, &second);
  return (status == NX_OK);
}

void File::printLink(NXlink &link) {
  NXstatus status = NXIprintlink(this->m_file_id, &link);
  if (status != NX_OK) {
    throw Exception("NXprintlink failed");
  }
}

void File::initGroupDir() {
  int status = NXinitgroupdir(this->m_file_id);
  if (status != NX_OK) {
    throw Exception("NXinitgroupdir failed", status);
  }
}

void File::initAttrDir() {
  int status = NXinitattrdir(this->m_file_id);
  if (status != NX_OK) {
    throw Exception("NXinitattrdir failed", status);
  }
}

string File::inquireFile(const int buff_length) {
  string filename;
  char *c_filename = new char[static_cast<size_t>(buff_length)];
  NXstatus status = NXinquirefile(this->m_file_id, c_filename, buff_length);
  if (status != NX_OK) {
    delete[] c_filename;
    stringstream msg;
    msg << "NXinquirefile(" << buff_length << ") failed";
    throw Exception(msg.str(), status);
  }
  filename = c_filename;
  delete[] c_filename;
  return filename;
}

string File::isExternalGroup(const string &name, const string &type, const unsigned buff_length) {
  string url;
  if (name.empty()) {
    throw Exception("Supplied empty name to isExternalGroup");
  }
  if (type.empty()) {
    throw Exception("Supplied empty type to isExternalGroup");
  }
  char *c_url = new char[buff_length];
  NXstatus status =
      NXisexternalgroup(this->m_file_id, name.c_str(), type.c_str(), c_url, static_cast<int>(buff_length));
  if (status != NX_OK) {
    delete[] c_url;
    stringstream msg;
    msg << "NXisexternalgroup(" << type << ", " << buff_length << ")";
    throw Exception(msg.str(), static_cast<int>(buff_length));
  }
  url = c_url;
  delete[] c_url;
  return url;
}

void File::linkExternal(const string &name, const string &type, const string &url) {
  if (name.empty()) {
    throw Exception("Supplied empty name to linkExternal");
  }
  if (type.empty()) {
    throw Exception("Supplied empty type to linkExternal");
  }
  if (url.empty()) {
    throw Exception("Supplied empty url to linkExternal");
  }
  NXstatus status = NXlinkexternal(this->m_file_id, name.c_str(), type.c_str(), url.c_str());
  if (status != NX_OK) {
    stringstream msg;
    msg << "NXlinkexternal(" << name << ", " << type << ", " << url << ") failed";
    throw Exception(msg.str(), status);
  }
}

const string File::makeCurrentPath(const string &currpath, const string &subpath) {
  std::ostringstream temp;
  temp << currpath << "/" << subpath;
  return temp.str();
}

void File::walkFileForTypeMap(const string &path, const string &class_name, TypeMap &tmap) {
  if (!path.empty()) {
    tmap.insert(std::make_pair(class_name, path));
  }
  map<string, string> dirents = this->getEntries();
  map<string, string>::iterator pos;
  for (pos = dirents.begin(); pos != dirents.end(); ++pos) {
    if (pos->second == "SDS") {
      tmap.insert(std::make_pair(pos->second, this->makeCurrentPath(path, pos->first)));
    } else if (pos->second == "CDF0.0") {
      // Do nothing with this
      ;
    } else {
      this->openGroup(pos->first, pos->second);
      this->walkFileForTypeMap(this->makeCurrentPath(path, pos->first), pos->second, tmap);
    }
  }
  this->closeGroup();
}

TypeMap *File::getTypeMap() {
  TypeMap *tmap = new TypeMap();
  // Ensure that we're at the top of the file.
  this->openPath("/");
  this->walkFileForTypeMap("", "", *tmap);
  return tmap;
}

template <typename NumT> void File::malloc(NumT *&data, const Info &info) {
  if (getType<NumT>() != info.type) {
    throw Exception("Type mismatch in malloc()");
  }
  // cppcheck-suppress cstyleCast
  inner_malloc((void *&)data, info.dims, info.type);
}

template <typename NumT> void File::free(NumT *&data) {
  // cppcheck-suppress cstyleCast
  inner_free((void *&)data);
}

} // namespace NeXus

/* ---------------------------------------------------------------- */
/* Concrete instantiations of template definitions.                 */
/* ---------------------------------------------------------------- */
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const float value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const double value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const int8_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const uint8_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const int16_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const uint16_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const int32_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const uint32_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const int64_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const uint64_t value);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const char value);

template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<float> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<double> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<int8_t> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<uint8_t> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<int16_t> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<uint16_t> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<int32_t> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<uint32_t> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<int64_t> &array);
template MANTID_NEXUSCPP_DLL void File::putAttr(const string &name, const std::vector<uint64_t> &array);

template MANTID_NEXUSCPP_DLL float File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL double File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL int8_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL uint8_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL int16_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL uint16_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL int32_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL uint32_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL int64_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL uint64_t File::getAttr(const AttrInfo &info);
template MANTID_NEXUSCPP_DLL char File::getAttr(const AttrInfo &info);

template MANTID_NEXUSCPP_DLL void File::makeData(const string &name, const NXnumtype type, const int length,
                                                 bool open_data);
template MANTID_NEXUSCPP_DLL void File::makeData(const string &name, const NXnumtype type, const int64_t length,
                                                 bool open_data);

template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const float &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const double &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const int8_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const uint8_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const int16_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const uint16_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const int32_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const uint32_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const int64_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const uint64_t &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const char &value);

template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<float> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<double> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int8_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint8_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int16_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint16_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int32_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint32_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int64_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint64_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<char> &value);

template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<float> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<double> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int8_t> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint8_t> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int16_t> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint16_t> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int32_t> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint32_t> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<int64_t> &value,
                                                  const std::vector<int> &dims);
template MANTID_NEXUSCPP_DLL void File::writeData(const string &name, const vector<uint64_t> &value,
                                                  const std::vector<int> &dims);

template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<float> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<double> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int8_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint8_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int16_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint16_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int32_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint32_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int64_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint64_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<char> &value);

template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<float> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<double> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int8_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint8_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int16_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint16_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int32_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint32_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int64_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint64_t> &value,
                                                            const int64_t chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<char> &value,
                                                            const int64_t chunk);

template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<float> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<double> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int8_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint8_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int16_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint16_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int32_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint32_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<int64_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<uint64_t> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);
template MANTID_NEXUSCPP_DLL void File::writeExtendibleData(const string &name, std::vector<char> &value,
                                                            std::vector<int64_t> &dims, std::vector<int64_t> &chunk);

template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<float> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<double> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int8_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint8_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int16_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint16_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int32_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint32_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int64_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint64_t> &value);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<char> &value);

template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<float> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<double> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int8_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint8_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int16_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint16_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int32_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint32_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<int64_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<uint64_t> &value,
                                                         std::vector<int64_t> &dims);
template MANTID_NEXUSCPP_DLL void File::writeUpdatedData(const string &name, vector<char> &value,
                                                         std::vector<int64_t> &dims);

template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<float> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<float> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<double> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<double> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int8_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int8_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint8_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint8_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int16_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int16_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint16_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint16_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int32_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int32_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint32_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint32_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int64_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<int64_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint64_t> &value,
                                                      const vector<int> &dims, const NXcompression comp,
                                                      const vector<int> &bufsize);
template MANTID_NEXUSCPP_DLL void File::writeCompData(const string &name, const vector<uint64_t> &value,
                                                      const vector<int64_t> &dims, const NXcompression comp,
                                                      const vector<int64_t> &bufsize);

template MANTID_NEXUSCPP_DLL vector<float> *File::getData();
template MANTID_NEXUSCPP_DLL vector<double> *File::getData();
template MANTID_NEXUSCPP_DLL vector<int8_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<uint8_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<int16_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<uint16_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<int32_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<uint32_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<int64_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<uint64_t> *File::getData();
template MANTID_NEXUSCPP_DLL vector<char> *File::getData();

template MANTID_NEXUSCPP_DLL void File::getData(vector<float> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<double> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<int8_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<uint8_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<int16_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<uint16_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<int32_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<uint32_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<int64_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<uint64_t> &data);
template MANTID_NEXUSCPP_DLL void File::getData(vector<char> &data);

template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<float> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<double> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<int8_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<uint8_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<int16_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<uint16_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<int32_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<uint32_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<int64_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<uint64_t> &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, vector<char> &data);

template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, float &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, double &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, int8_t &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, uint8_t &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, int16_t &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, uint16_t &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, int32_t &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, uint32_t &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, int64_t &data);
template MANTID_NEXUSCPP_DLL void File::readData(const std::string &dataName, uint64_t &data);

template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<float> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<double> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int8_t> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint8_t> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int16_t> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint16_t> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int32_t> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint32_t> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int64_t> &data, int start, int size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint64_t> &data, int start, int size);

template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<float> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<double> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int8_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint8_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int16_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint16_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int32_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint32_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<int64_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);
template MANTID_NEXUSCPP_DLL void File::putSlab(const std::vector<uint64_t> &data, const std::vector<int64_t> &start,
                                                const std::vector<int64_t> &size);

template MANTID_NEXUSCPP_DLL void File::getAttr(const std::string &name, double &value);
template MANTID_NEXUSCPP_DLL void File::getAttr(const std::string &name, int &value);

template MANTID_NEXUSCPP_DLL void File::malloc(int *&data, const Info &info);
template MANTID_NEXUSCPP_DLL void File::malloc(float *&data, const Info &info);
template MANTID_NEXUSCPP_DLL void File::malloc(double *&data, const Info &info);

template MANTID_NEXUSCPP_DLL void File::free(int *&data);
template MANTID_NEXUSCPP_DLL void File::free(float *&data);
template MANTID_NEXUSCPP_DLL void File::free(double *&data);
