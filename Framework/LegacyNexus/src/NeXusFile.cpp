#include <cstring>
// REMOVE
#include "MantidLegacyNexus/NeXusException.hpp"
#include "MantidLegacyNexus/NeXusFile.hpp"
#include <iostream>
#include <numeric>
#include <sstream>
#include <typeinfo>

using namespace Mantid::LegacyNexus;
using std::map;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;

/**
 * \file NeXusFile.cpp
 * The implementation of the NeXus C++ API
 */

namespace { // anonymous namespace to keep it in the file
const std::string NULL_STR("NULL");
} // end of anonymous namespace

namespace Mantid::LegacyNexus {

// catch for undefined types
template <typename NumT> NXnumtype getType(NumT const number) {
  stringstream msg;
  msg << "NeXus::getType() does not know type of " << typeid(number).name();
  throw Exception(msg.str());
}

template <> MANTID_LEGACYNEXUS_DLL NXnumtype getType(char const) { return NXnumtype::CHAR; }

// template specialisations for types we know
template <> MANTID_LEGACYNEXUS_DLL NXnumtype getType(float const) { return NXnumtype::FLOAT32; }

template <> MANTID_LEGACYNEXUS_DLL NXnumtype getType(int32_t const) { return NXnumtype::INT32; }

} // namespace Mantid::LegacyNexus

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

namespace Mantid::LegacyNexus {
File::File(const string &filename, const NXaccess access) : m_close_handle(true) {
  this->initOpenFile(filename, access);
}

void File::initOpenFile(const string &filename, const NXaccess access) {
  if (filename.empty()) {
    throw Exception("Filename specified is empty constructor");
  }

  NXstatus status = NXopen(filename.c_str(), access, &(this->m_file_id));
  if (status != NXstatus::NX_OK) {
    stringstream msg;
    msg << "NXopen(" << filename << ", " << access << ") failed";
    throw Exception(msg.str(), status);
  }
}

File::~File() {
  if (m_close_handle && m_file_id != NULL) {
    NXstatus status = NXclose(&(this->m_file_id));
    this->m_file_id = NULL;
    if (status != NXstatus::NX_OK) {
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
    if (status != NXstatus::NX_OK) {
      throw Exception("NXclose failed", status);
    }
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
  if (status != NXstatus::NX_OK) {
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
  if (status != NXstatus::NX_OK) {
    stringstream msg;
    msg << "NXopenpath(" << path << ") failed";
    throw Exception(msg.str(), status);
  }
}

std::string File::getPath() {
  std::string path = NXgetpath(this->m_file_id);
  return path;
}

void File::closeGroup() {
  NXstatus status = NXclosegroup(this->m_file_id);
  if (status != NXstatus::NX_OK) {
    throw Exception("NXclosegroup failed", status);
  }
}

void File::openData(const string &name) {
  if (name.empty()) {
    throw Exception("Supplied empty name to openData");
  }
  NXstatus status = NXopendata(this->m_file_id, name.c_str());
  if (status != NXstatus::NX_OK) {
    throw Exception("NXopendata(" + name + ") failed", status);
  }
}

void File::closeData() {
  NXstatus status = NXclosedata(this->m_file_id);
  if (status != NXstatus::NX_OK) {
    throw Exception("NXclosedata() failed", status);
  }
}

void File::getData(void *data) {
  if (data == NULL) {
    throw Exception("Supplied null pointer to getData");
  }
  NXstatus status = NXgetdata(this->m_file_id, data);
  if (status != NXstatus::NX_OK) {
    throw Exception("NXgetdata failed", status);
  }
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

string File::getStrData() {
  string res;
  Info info = this->getInfo();
  if (info.type != NXnumtype::CHAR) {
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
  NXnumtype type;
  int rank;
  NXstatus status = NXgetinfo64(this->m_file_id, &rank, dims, &type);
  if (status != NXstatus::NX_OK) {
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
  NXname name, class_name;
  NXnumtype datatype;

  NXstatus status = NXgetnextentry(this->m_file_id, name, class_name, &datatype);
  if (status == NXstatus::NX_OK) {
    string str_name(name);
    string str_class(class_name);
    return pair<string, string>(str_name, str_class);
  } else if (status == NXstatus::NX_EOD) {
    return EOD_ENTRY; // TODO return the correct thing
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
    if (temp == EOD_ENTRY) { // TODO this needs to be changed when getNextEntry is fixed
      break;
    } else {
      result.insert(temp);
    }
  }
}

AttrInfo File::getNextAttr() {
  // string & name, int & length, NXnumtype type) {
  NXname name;
  NXnumtype type;

  int rank;
  int dim[NX_MAXRANK];
  NXstatus status = NXgetnextattra(this->m_file_id, name, &rank, dim, &type);
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
    throw Exception("getNextAttr failed", NXstatus::NX_ERROR);

  } else if (status == NXstatus::NX_EOD) {
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
  NXnumtype type = info.type;
  if (length < 0) {
    length = static_cast<int>(info.length);
  }
  NXstatus status = NXgetattr(this->m_file_id, name, data, &length, &type);
  if (status != NXstatus::NX_OK) {
    throw Exception("NXgetattr(" + info.name + ") failed", status);
  }
  if (type != info.type) {
    stringstream msg;
    msg << "NXgetattr(" << info.name << ") changed type [" << info.type << "->" << type << "]";
    throw Exception(msg.str());
  }
  // char attributes are always NULL terminated and so may change length
  if (static_cast<unsigned>(length) != info.length && type != NXnumtype::CHAR) {
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

template <> MANTID_LEGACYNEXUS_DLL void File::getAttr(const std::string &name, std::string &value) {
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

void File::initGroupDir() {
  NXstatus status = NXinitgroupdir(this->m_file_id);
  if (status != NXstatus::NX_OK) {
    throw Exception("NXinitgroupdir failed", status);
  }
}

void File::initAttrDir() {
  NXstatus status = NXinitattrdir(this->m_file_id);
  if (status != NXstatus::NX_OK) {
    throw Exception("NXinitattrdir failed", status);
  }
}

} // namespace Mantid::LegacyNexus

std::ostream &operator<<(std::ostream &stm, const Mantid::LegacyNexus::NXstatus status) {
  return stm << static_cast<int>(status);
}
std::ostream &operator<<(std::ostream &stm, const Mantid::LegacyNexus::NXnumtype type) {
  return stm << static_cast<int>(type);
}

/* ---------------------------------------------------------------- */
/* Concrete instantiations of template definitions.                 */
/* ---------------------------------------------------------------- */

template MANTID_LEGACYNEXUS_DLL void File::getData(vector<float> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<double> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<int8_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<uint8_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<int16_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<uint16_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<int32_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<uint32_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<int64_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<uint64_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::getData(vector<char> &data);

template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<float> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<double> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<int8_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<uint8_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<int16_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<uint16_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<int32_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<uint32_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<int64_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<uint64_t> &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, vector<char> &data);

template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, float &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, double &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, int8_t &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, uint8_t &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, int16_t &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, uint16_t &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, int32_t &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, uint32_t &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, int64_t &data);
template MANTID_LEGACYNEXUS_DLL void File::readData(const std::string &dataName, uint64_t &data);

template MANTID_LEGACYNEXUS_DLL void File::getAttr(const std::string &name, double &value);
template MANTID_LEGACYNEXUS_DLL void File::getAttr(const std::string &name, int &value);
