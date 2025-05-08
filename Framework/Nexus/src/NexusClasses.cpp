// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexus/NexusClasses.h"
#include "MantidNexus/NeXusException.hpp"

#include <memory>
#include <utility>

using ::NeXus::dimsize_t;

namespace Mantid::NeXus {

static NXDimArray nxdimArray(::NeXus::DimVector xd) {
  NXDimArray ret{0};
  for (std::size_t i = 0; i < xd.size(); i++) {
    ret[i] = xd[i];
  }
  return ret;
}

NXInfo::NXInfo(::NeXus::Info const &info, std::string const &name)
    : nxname(name), rank(info.dims.size()), dims(nxdimArray(info.dims)), type(info.type), allGood(true) {}

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
  m_values[name] = std::to_string(value);
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

/**  NXObject constructor.
 *   @param fileID :: The Nexus file id
 *   @param parent :: The parent Nexus class. In terms of HDF it is the group
 * containing the object.
 *   @param name :: The name of the object relative to its parent
 */
NXObject::NXObject(std::shared_ptr<::NeXus::File> fileID, NXClass const *parent, const std::string &name)
    : m_fileID(fileID), m_open(false) {
  if (parent && !name.empty()) {
    m_path_parent = parent->path();
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
void NXDataSet::getAttributes() {
  std::vector<char> buff(128);
  for (::NeXus::AttrInfo const &ainfo : m_fileID->getAttrInfos()) {
    if (ainfo.type != NXnumtype::CHAR && ainfo.length != 1) {
      throw std::runtime_error("Encountered attribute with array value");
    }

    switch (ainfo.type) {
    case NXnumtype::CHAR: {
      attributes.set(ainfo.name, m_fileID->getAttr<std::string>(ainfo.name));
      break;
    }
    case NXnumtype::INT16: {
      attributes.set(ainfo.name, m_fileID->getAttr<int16_t>(ainfo.name));
      break;
    }
    case NXnumtype::UINT16: {
      attributes.set(ainfo.name, m_fileID->getAttr<uint16_t>(ainfo.name));
      break;
    }
    case NXnumtype::INT32: {
      attributes.set(ainfo.name, m_fileID->getAttr<int32_t>(ainfo.name));
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

NXClass::~NXClass() {
  if (m_open && (!m_path_parent.empty()))
    m_fileID->openPath(m_path_parent);
}

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
}

bool NXClass::isValid(const std::string &path) const {
  const auto previousPath = m_fileID->getPath();
  bool result = false;
  try {
    m_fileID->openGroupPath(path);
    result = true;
  } catch (::NeXus::Exception const &) {
    // assume the path didn't exist
    result = false;
  }
  m_fileID->openPath(previousPath);
  return result;
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

void NXClass::clear() {
  m_groups.reset(new std::vector<NXClassInfo>);
  m_datasets.reset(new std::vector<NXInfo>);
}

namespace {
/**
 * This assumes that the last "element" of the current path can overlap with the first "element" of the requested path
 * and removes duplication.
 */
std::pair<std::string, std::string> splitPath(const std::string &current, const std::string &requested) {
  // return early if it is already split
  if (requested.find("/") == std::string::npos)
    return std::pair<std::string, std::string>{current, requested};

  // decompose the two components to see if the inner element overlaps
  const auto currentLastPos = current.rfind("/");
  const auto currentLast = (currentLastPos == std::string::npos) ? std::string("") : current.substr(currentLastPos + 1);

  const auto requestedFirstPos = requested.find("/");
  const auto requestedFirst =
      (requestedFirstPos == std::string::npos) ? std::string("") : requested.substr(0, requestedFirstPos);

  // construct the effective path
  std::string fullPath;
  if (currentLast == requestedFirst) {
    fullPath = current + requested.substr(requestedFirstPos);
  } else {
    fullPath = current + "/" + requested;
  }

  // last / is the division between path and data
  const auto last = fullPath.rfind("/");
  return std::pair<std::string, std::string>{fullPath.substr(0, last), fullPath.substr(last + 1)};
}
} // namespace

std::string NXClass::getString(const std::string &name) const {
  const std::string oldPath = m_fileID->getPath();
  // split the input into group and name
  auto pathParts = splitPath(oldPath, name);

  std::string value;

  // open the containing group
  m_fileID->openPath(pathParts.first);

  // read the value
  try {
    m_fileID->readData(name, value);
  } catch (const ::NeXus::Exception &) {
    m_fileID->openPath(oldPath); // go back to original location
    throw;                       // rethrow the exception
  }
  m_fileID->openPath(oldPath); // go back to original location

  return value;
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

int32_t NXClass::getInt(const std::string &name) const {
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
 *  @return NXInfo::allGood is set to false if the dataset does not exist
 */
NXInfo NXClass::getDataSetInfo(const std::string &name) const {
  const auto it = std::find_if(datasets().cbegin(), datasets().cend(),
                               [&name](const auto &dataset) { return dataset.nxname == name; });
  NXInfo info;
  if (it != datasets().cend()) {
    info = *it;
  } else {
    info.allGood = false;
  }
  return info;
}

/**
 * Returns whether an individual dataset is present.
 */
bool NXClass::containsDataSet(const std::string &query) const { return getDataSetInfo(query).allGood; }

//---------------------------------------------------------
//          NXRoot methods
//---------------------------------------------------------

/**  Constructor. On creation opens the Nexus file for reading only.
 *   @param fname :: The file name to open
 */
NXRoot::NXRoot(std::string fname) : m_filename(std::move(fname)) {
  // Open NeXus file
  try {
    m_fileID = std::make_shared<::NeXus::File>(m_filename, NXACC_READ);
  } catch (::NeXus::Exception const &e) {
    std::cout << "NXRoot: Error loading " << m_filename << "\" in read mode: " << e.what() << "\n";
    throw;
  }
  readAllInfo();
}

/**  Constructor.
 *   Creates a new Nexus file. The first root entry will be also created.
 *   @param fname :: The file name to create
 *   @param entry :: The name of the first entry in the new file
 */
NXRoot::NXRoot(std::string fname, const std::string &entry) : m_filename(std::move(fname)) {
  UNUSED_ARG(entry);
  // Open NeXus file
  m_fileID = std::make_shared<::NeXus::File>(m_filename, NXACC_CREATE5);
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

// Opens the data set. Does not read in any data. Call load(...) to load the data
void NXDataSet::open() {
  size_t i = m_path.find_last_of('/');
  if (i == std::string::npos || i == 0)
    return; // we are in the root group, assume it is open
  std::string path_before = m_fileID->getPath();
  std::string group_path = m_path.substr(0, i);
  m_fileID->openPath(group_path);
  m_fileID->openData(name());
  m_info = NXInfo(m_fileID->getInfo(), name());
  getAttributes();
  // go back to where the file was before
  m_fileID->closeData();
  if (!path_before.empty())
    m_fileID->openPath(path_before);
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
nxdimsize_t NXDataSet::dim0() const {
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
nxdimsize_t NXDataSet::dim1() const {
  if (m_info.rank < 2) {
    throw std::out_of_range("NXDataSet::dim1() - Requested dimension greater than rank.");
  }
  return static_cast<int>(m_info.dims[1]);
}

/**
 * The size of the third dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 3
 */
nxdimsize_t NXDataSet::dim2() const {
  if (m_info.rank < 3) {
    throw std::out_of_range("NXDataSet::dim2() - Requested dimension greater than rank.");
  }
  return static_cast<int>(m_info.dims[2]);
}

/**
 * The size of the fourth dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 4
 */
nxdimsize_t NXDataSet::dim3() const {
  if (m_info.rank < 4) {
    throw std::out_of_range("NXDataSet::dim3() - Requested dimension greater than rank.");
  }
  return static_cast<int>(m_info.dims[3]);
}

//---------------------------------------------------------
//          NXData methods
//---------------------------------------------------------

NXData::NXData(const NXClass &parent, const std::string &name) : NXClass(parent, name) {}

} // namespace Mantid::NeXus
