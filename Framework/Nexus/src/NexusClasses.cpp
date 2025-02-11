// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexus/NexusClasses.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <memory>
#include <utility>

namespace Mantid::NeXus {

static NXDimArray dimArray(::NeXus::DimVector xd) {
  return NXDimArray({static_cast<DimSize>(xd[0]), static_cast<DimSize>(xd[1]), static_cast<DimSize>(xd[2]),
                     static_cast<DimSize>(xd[3])});
}

NXInfo::NXInfo(::NeXus::Info const &info, std::string const &name)
    : nxname(name), rank(info.dims.size()), dims(dimArray(info.dims)), type(info.type), stat(NXstatus::NX_OK) {}

std::vector<std::string> NXAttributes::names() const {
  std::vector<std::string> out;
  out.reserve(m_values.size());
  std::transform(m_values.cbegin(), m_values.cend(), std::back_inserter(out),
                 [](const auto &value) { return value.first; });
  return out;
}

std::vector<std::string> NXAttributes::values() const {
  std::vector<std::string> out;
  out.reserve(m_values.size());
  std::transform(m_values.cbegin(), m_values.cend(), std::back_inserter(out),
                 [](const auto &value) { return value.second; });
  return out;
}

/**  Returns the value of an attribute
 *   @param name :: The name of the attribute
 *   @return The value of the attribute if it exists or an empty string
 * otherwise
 */
std::string NXAttributes::operator()(const std::string &name) const {
  auto it = m_values.find(name);
  if (it == m_values.end())
    return "";
  return it->second;
}

/**  Sets the value of the attribute.
 *   @param name :: The name of the attribute
 *   @param value :: The new value of the attribute
 */
void NXAttributes::set(const std::string &name, const std::string &value) { m_values[name] = value; }

/**  Sets the value of the attribute from typed value.
 *   @param name :: The name of the attribute
 *   @param value :: The new value of the attribute
 */
template <typename T> void NXAttributes::set(const std::string &name, T value) {
  std::ostringstream ostr;
  ostr << value;
  m_values[name] = ostr.str();
}

//---------------------------------------------------------
//          NXObject methods
//---------------------------------------------------------

/**  NXObject private default constructor
 */
NXObject::NXObject() : m_fileID(nullptr), m_open(false) {}

/**  NXObject constructor.
 *   @param fileID :: The Nexus file id
 *   @param parent :: The parent Nexus class. In terms of HDF it is the group
 * containing the object.
 *   @param name :: The name of the object relative to its parent
 */
NXObject::NXObject(::NeXus::File *fileID, NXClass const *parent, const std::string &name)
    : m_fileID(fileID), m_open(false) {
  if (parent && !name.empty()) {
    m_path = parent->path() + "/" + name;
  }
}

std::string NXObject::name() const {
  size_t i = m_path.find_last_of('/');
  if (i == std::string::npos)
    return m_path;
  else
    return m_path.substr(i + 1, m_path.size() - i - 1);
}

/**  Reads in attributes
 */
void NXObject::getAttributes() {
  std::vector<char> buff(128);
  for (::NeXus::AttrInfo const &ainfo : m_fileID->getAttrInfos()) {
    if (ainfo.type != NXnumtype::CHAR && ainfo.length != 1) {
      throw std::runtime_error("Encountered attribute with array value");
    }

    switch (ainfo.type) {
    case NXnumtype::CHAR: {
      attributes.set(ainfo.name, m_fileID->getStrAttr(ainfo));
      break;
    }
    case NXnumtype::INT16: {
      attributes.set(ainfo.name, m_fileID->getAttr<short int>(ainfo));
      break;
    }
    case NXnumtype::INT32: {
      attributes.set(ainfo.name, m_fileID->getAttr<int>(ainfo));
      break;
    }
    case NXnumtype::UINT16: {
      attributes.set(ainfo.name, m_fileID->getAttr<short unsigned int>(ainfo));
      break;
    }
    default:
      break;
    }
  };
}
//---------------------------------------------------------
//          NXClass methods
//---------------------------------------------------------

NXClass::NXClass(const NXClass &parent, const std::string &name) : NXObject(parent.m_fileID, &parent, name) { clear(); }

void NXClass::readAllInfo() {
  clear();
  for (auto const &entry : m_fileID->getEntries()) {
    if (entry.second == "SDS") {
      m_fileID->openData(entry.first);
      NXInfo info(m_fileID->getInfo(), entry.first);
      m_fileID->closeData();
      m_datasets->emplace_back(info);
    } else if (entry.second.substr(0, 2) == "NX" || entry.second.substr(0, 2) == "IX") {
      m_groups->emplace_back(NXClassInfo(entry));
    }
  }
  reset();
}

bool NXClass::isValid(const std::string &path) const {
  try {
    m_fileID->openGroupPath(path);
    m_fileID->closeGroup();
    return true;
  } catch (::NeXus::Exception const &) {
    return false;
  }
}

void NXClass::open() {
  m_fileID->openGroupPath(m_path);
  m_open = true;
  readAllInfo();
}

/** It is fast, but the parent of this class must be open at
 * the time of calling. openNXClass uses open() (the slow one). To open class
 * using openLocal() do:
 *    NXTheClass class(parent,name);
 *    class.openLocal();
 *    // work with class
 *    class.close();
 * @param nxclass :: The NX class name. If empty NX_class() will be used
 * @return true if OK
 */
bool NXClass::openLocal(const std::string &nxclass) {
  std::string className = nxclass.empty() ? NX_class() : nxclass;
  try {
    m_fileID->openGroup(name(), className);
  } catch (::NeXus::Exception const &) {
    return false;
  }
  m_open = true;
  readAllInfo();
  return true;
}

void NXClass::close() {
  try {
    m_fileID->closeGroup();
  } catch (::NeXus::Exception const &) {
    throw std::runtime_error("Cannot close group " + name() + " of class " + NX_class() + " (trying to close path " +
                             m_path + ")");
  }
  m_open = false;
}

void NXClass::reset() { m_fileID->initGroupDir(); }

void NXClass::clear() {
  m_groups.reset(new std::vector<NXClassInfo>);
  m_datasets.reset(new std::vector<NXInfo>);
}

std::string NXClass::getString(const std::string &name) const {
  NXChar buff = openNXChar(name);
  try {
    buff.load();
    return std::string(buff(), buff.dim0());
  } catch (std::runtime_error &) {
    // deals with reading uninitialized/empty data
    return std::string();
  }
}

double NXClass::getDouble(const std::string &name) const {
  NXDouble number = openNXDouble(name);
  number.load();
  return *number();
}

float NXClass::getFloat(const std::string &name) const {
  NXFloat number = openNXFloat(name);
  number.load();
  return *number();
}

int NXClass::getInt(const std::string &name) const {
  NXInt number = openNXInt(name);
  number.load();
  return *number();
}
/** Returns whether an individual group (or group) is present
 *  @param query :: the class name to search for
 *  @return true if the name is found and false otherwise
 */
bool NXClass::containsGroup(const std::string &query) const {
  return std::any_of(m_groups->cbegin(), m_groups->cend(),
                     [&query](const auto &group) { return group.nxname == query; });
}

/**
 *  Returns NXInfo for a dataset
 *  @param name :: The name of the dataset
 *  @return NXInfo::stat is set to NXstatus::NX_ERROR if the dataset does not exist
 */
NXInfo NXClass::getDataSetInfo(const std::string &name) const {
  const auto it = std::find_if(datasets().cbegin(), datasets().cend(),
                               [&name](const auto &dataset) { return dataset.nxname == name; });
  if (it != datasets().cend()) {
    return *it;
  }
  NXInfo info;
  info.stat = NXstatus::NX_ERROR;
  return info;
}

/**
 * Returns whether an individual dataset is present.
 */
bool NXClass::containsDataSet(const std::string &query) const {
  return getDataSetInfo(query).stat != NXstatus::NX_ERROR;
}

//---------------------------------------------------------
//          NXRoot methods
//---------------------------------------------------------

/**  Constructor. On creation opens the Nexus file for reading only.
 *   @param fname :: The file name to open
 */
NXRoot::NXRoot(std::string fname) : m_filename(std::move(fname)) {
  // Open NeXus file
  try {
    m_fileID = new ::NeXus::File(m_filename, NXACC_READ);
  } catch (::NeXus::Exception const &e) {
    std::cout << "NXRoot: Error loading " << m_filename << "\" in read mode: " << e.what() << "\n";
    throw Kernel::Exception::FileError("Unable to open File:", m_filename);
  }
  readAllInfo();
}

/**  Constructor.
 *   Creates a new Nexus file. The first root entry will be also created.
 *   @param fname :: The file name to create
 *   @param entry :: The name of the first entry in the new file
 */
NXRoot::NXRoot(std::string fname, const std::string &) : m_filename(std::move(fname)) {
  // Open NeXus file
  try {
    m_fileID = new ::NeXus::File(m_filename, NXACC_CREATE5);
  } catch (::NeXus::Exception const &) {
    throw Kernel::Exception::FileError("Unable to open File:", m_filename);
  }
}

NXRoot::~NXRoot() { m_fileID->close(); }

bool NXRoot::isStandard() const { return true; }

/**
 * Open the first NXentry in the file.
 */
NXEntry NXRoot::openFirstEntry() {
  if (groups().empty()) {
    throw std::runtime_error("NeXus file has no entries");
  }
  const auto it =
      std::find_if(groups().cbegin(), groups().cend(), [](const auto &group) { return group.nxclass == "NXentry"; });
  if (it != groups().cend()) {
    return openEntry(it->nxname);
  }
  throw std::runtime_error("NeXus file has no entries");
}

//---------------------------------------------------------
//          NXDataSet methods
//---------------------------------------------------------

/**  Constructor.
 *   @param parent :: The parent Nexus class. In terms of HDF it is the group
 * containing the dataset.
 *   @param name :: The name of the dataset relative to its parent
 */
NXDataSet::NXDataSet(const NXClass &parent, const std::string &name) : NXObject(parent.m_fileID, &parent, name) {
  size_t i = name.find_last_of('/');
  if (i == std::string::npos)
    m_info.nxname = name;
  else if (name.empty() || i == name.size() - 1)
    throw std::runtime_error("Improper dataset name " + name);
  else
    m_info.nxname = name.substr(i + 1);
}

// Opens the data set. Does not read in any data. Call load(...) to load the
// data
void NXDataSet::open() {
  size_t i = m_path.find_last_of('/');
  if (i == std::string::npos || i == 0)
    return; // we are in the root group, assume it is open
  std::string group_path = m_path.substr(0, i);
  m_fileID->openPath(group_path);
  m_fileID->openData(name());
  m_info = NXInfo(m_fileID->getInfo(), name());
  getAttributes();
  m_fileID->closeData();
}

void NXDataSet::openLocal() {
  m_fileID->openData(name());
  m_info = NXInfo(m_fileID->getInfo(), name());
  getAttributes();
  m_fileID->closeData();
}

/**
 * The size of the first dimension of data
 * @returns An integer indicating the size of the dimension.
 * @throws out_of_range error if requested on an object of rank 0
 */
int NXDataSet::dim0() const {
  if (m_info.rank == 0UL) {
    throw std::out_of_range("NXDataSet::dim0() - Requested dimension greater than rank.");
  }
  return static_cast<int>(m_info.dims[0]);
}

/**
 * The size of the second dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 2
 */
int NXDataSet::dim1() const {
  if (m_info.rank < 2UL) {
    throw std::out_of_range("NXDataSet::dim1() - Requested dimension greater than rank.");
  }
  return static_cast<int>(m_info.dims[1]);
}

/**
 * The size of the third dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 3
 */
int NXDataSet::dim2() const {
  if (m_info.rank < 3UL) {
    throw std::out_of_range("NXDataSet::dim2() - Requested dimension greater than rank.");
  }
  return static_cast<int>(m_info.dims[2]);
}

/**
 * The size of the fourth dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 4
 */
int NXDataSet::dim3() const {
  if (m_info.rank < 4UL) {
    throw std::out_of_range("NXDataSet::dim3() - Requested dimension greater than rank.");
  }
  return static_cast<int>(m_info.dims[3]);
}

/**  Wrapper to the NXgetdata.
 *   @param data :: The pointer to the buffer accepting the data from the file.
 *   @throw runtime_error if the operation fails.
 */
void NXDataSet::getData(void *data) {
  m_fileID->openData(name());
  m_fileID->getData(data);
  m_fileID->closeData();
}

/**  Wrapper to the NXgetslab.
 *   @param data :: The pointer to the buffer accepting the data from the file.
 *   @param start :: The array of starting indeces to read in from the file. The
 * size of the array must be equal to
 *          the rank of the data.
 *   @param size :: The array of numbers of data elements to read along each
 * dimenstion.
 *          The number of dimensions (the size of the array) must be equal to
 * the rank of the data.
 *   @throw runtime_error if the operation fails.
 */
void NXDataSet::getSlab(void *data, NXDimArray const &start, NXDimArray const &size) {
  std::vector<::NeXus::DimSize> vstart(start.cbegin(), start.cend());
  std::vector<::NeXus::DimSize> vsize(size.cbegin(), size.cend());
  m_fileID->openData(name());
  m_fileID->getSlab(data, vstart, vsize);
  m_fileID->closeData();
}

//---------------------------------------------------------
//          NXData methods
//---------------------------------------------------------

NXData::NXData(const NXClass &parent, const std::string &name) : NXMainClass(parent, name) {}

//---------------------------------------------------------
//          NXLog methods
//---------------------------------------------------------

/** Creates a property wrapper around the log entry
 * @returns A valid property pointer or NULL
 */
Kernel::Property *NXLog::createProperty() {
  NXInfo vinfo = getDataSetInfo("time");
  if (vinfo.stat == NXstatus::NX_ERROR) {
    return createSingleValueProperty();
  } else {
    return createTimeSeries();
  }
}

/** Creates a single value property of the log
 * @returns A pointer to a newly created property wrapped around the log entry
 */
Kernel::Property *NXLog::createSingleValueProperty() {
  const std::string valAttr("value");
  NXInfo vinfo = getDataSetInfo(valAttr);
  Kernel::Property *prop;
  NXnumtype nxType = vinfo.type;
  if (nxType == NXnumtype::FLOAT64) {
    prop = new Kernel::PropertyWithValue<double>(name(), getDouble(valAttr));
  } else if (nxType == NXnumtype::INT32) {
    prop = new Kernel::PropertyWithValue<int>(name(), getInt(valAttr));
  } else if (nxType == NXnumtype::CHAR) {
    prop = new Kernel::PropertyWithValue<std::string>(name(), getString(valAttr));
  } else if (nxType == NXnumtype::UINT8) {
    NXDataSetTyped<unsigned char> value(*this, valAttr);
    value.load();
    bool state = value[0] != 0;
    prop = new Kernel::PropertyWithValue<bool>(name(), state);
  } else {
    prop = nullptr;
  }

  return prop;
}

/** createTimeSeries
 * Create a TimeSeries property form the records of the NXLog group. Times are
 * in dataset "time"
 * and the values are in dataset "value"
 * @param start_time :: If the "time" dataset does not have the "start"
 * attribute sets the
 *   start time for the series.
 * @param new_name :: If not empty it is used as the TimeSeries property name
 *   @return The property or NULL
 */
Kernel::Property *NXLog::createTimeSeries(const std::string &start_time, const std::string &new_name) {
  const std::string &logName = new_name.empty() ? name() : new_name;
  NXInfo vinfo = getDataSetInfo("time");
  if (vinfo.type == NXnumtype::FLOAT64) {
    NXDouble times(*this, "time");
    times.openLocal();
    times.load();
    std::string units = times.attributes("units");
    if (units == "minutes") {
      using std::placeholders::_1;
      std::transform(times(), times() + times.dim0(), times(), std::bind(std::multiplies<double>(), _1, 60));
    } else if (!units.empty() && units.substr(0, 6) != "second") {
      return nullptr;
    }
    return parseTimeSeries(logName, times, start_time);
  } else if (vinfo.type == NXnumtype::FLOAT32) {
    NXFloat times(*this, "time");
    times.openLocal();
    times.load();
    std::string units = times.attributes("units");
    if (units == "minutes") {
      std::for_each(times(), times() + times.dim0(), [](float &val) { val *= 60.0f; });
    } else if (!units.empty() && units.substr(0, 6) != "second") {
      return nullptr;
    }
    return parseTimeSeries(logName, times, start_time);
  }

  return nullptr;
}

} // namespace Mantid::NeXus
