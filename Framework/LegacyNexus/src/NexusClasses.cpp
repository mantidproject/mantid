// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLegacyNexus/NexusClasses.h"
#include "MantidLegacyNexus/NeXusException.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <utility>

namespace Mantid::LegacyNexus {

namespace {
void readDataSetInfo(const NXhandle fileID, NXInfo &info, const std::string &name) {
  std::array<int64_t, NX_MAXRANK> dims{};
  info.stat = NXgetinfo64(fileID, &info.rank, dims.data(), &info.type);
  if (info.stat != NXstatus::NX_OK)
    throw Exception("Failed to read information for dataset " + name, info.stat);
  if (info.rank > 4)
    throw Exception("Dataset " + name + " has unsupported rank greater than 4", NXstatus::NX_ERROR);
  for (int i = 0; i < info.rank; ++i) {
    if (dims[static_cast<size_t>(i)] < 0 || dims[static_cast<size_t>(i)] > std::numeric_limits<int>::max())
      throw Exception("Dataset " + name + " has a dimension outside the supported range", NXstatus::NX_ERROR);
    info.dims[i] = static_cast<int>(dims[static_cast<size_t>(i)]);
  }
}
} // namespace

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

//---------------------------------------------------------
//          NXObject methods
//---------------------------------------------------------

/**  NXObject constructor.
 *   @param fileID :: The Nexus file id
 *   @param parent :: The parent Nexus class. In terms of HDF it is the group
 * containing the object.
 *   @param name :: The name of the object relative to its parent
 */
NXObject::NXObject(const NXhandle fileID, const NXClass *parent, const std::string &name)
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
  NXname pName;
  NXnumtype iType;
  int iLength;
  int rank;
  std::array<int, NX_MAXRANK> dims{};
  std::vector<char> buff(128);

  NXstatus status;
  while ((status = NXgetnextattra(m_fileID, pName, &rank, dims.data(), &iType)) == NXstatus::NX_OK) {
    if (rank > 1) { // mantid only supports single value attributes
      throw std::runtime_error("Encountered attribute with multi-dimensional array value");
    }
    iLength = dims[0]; // to clarify things
    if (iType != NXnumtype::CHAR && iLength != 1) {
      throw std::runtime_error("Encountered attribute with array value");
    }

    switch (iType) {
    case NXnumtype::CHAR: {
      if (iLength >= 0 && static_cast<size_t>(iLength + 1) > buff.size()) {
        buff.resize(static_cast<size_t>(iLength + 1));
      }
      int nz = iLength + 1;
      if (NXgetattr(m_fileID, pName, buff.data(), &nz, &iType) != NXstatus::NX_OK)
        throw Exception("Failed to read attribute " + std::string(pName), NXstatus::NX_ERROR);
      buff[static_cast<size_t>(iLength)] = '\0';
      attributes.set(pName, buff.data());
      break;
    }
    case NXnumtype::INT16: {
      short int value{};
      if (NXgetattr(m_fileID, pName, &value, &iLength, &iType) != NXstatus::NX_OK)
        throw Exception("Failed to read attribute " + std::string(pName), NXstatus::NX_ERROR);
      sprintf(buff.data(), "%i", value);
      attributes.set(pName, buff.data());
      break;
    }
    case NXnumtype::INT32: {
      int value{};
      if (NXgetattr(m_fileID, pName, &value, &iLength, &iType) != NXstatus::NX_OK)
        throw Exception("Failed to read attribute " + std::string(pName), NXstatus::NX_ERROR);
      sprintf(buff.data(), "%i", value);
      attributes.set(pName, buff.data());
      break;
    }
    case NXnumtype::UINT16: {
      short unsigned int value{};
      if (NXgetattr(m_fileID, pName, &value, &iLength, &iType) != NXstatus::NX_OK)
        throw Exception("Failed to read attribute " + std::string(pName), NXstatus::NX_ERROR);
      sprintf(buff.data(), "%u", value);
      attributes.set(pName, buff.data());
      break;
    }
    default:
      break;
    }
  }
  if (status == NXstatus::NX_ERROR)
    throw Exception("Failed to enumerate attributes", status);
}
//---------------------------------------------------------
//          NXClass methods
//---------------------------------------------------------

NXClass::NXClass(const NXClass &parent, const std::string &name) : NXObject(parent.m_fileID, &parent, name) { clear(); }

NXClassInfo NXClass::getNextEntry() {
  NXClassInfo res;
  char nxname[NX_MAXNAMELEN], nxclass[NX_MAXNAMELEN];
  res.stat = NXgetnextentry(m_fileID, nxname, nxclass, &res.datatype);
  if (res) // Check if previous call was successful
  {
    res.nxname = nxname;
    res.nxclass = nxclass;
  }
  return res;
}

void NXClass::readAllInfo() {
  clear();
  NXClassInfo info;
  while ((info = getNextEntry())) {
    if (info.nxclass == "SDS") {
      NXInfo data_info;
      if (NXopendata(m_fileID, info.nxname.c_str()) != NXstatus::NX_OK)
        throw Exception("Failed to open dataset " + info.nxname, NXstatus::NX_ERROR);

      try {
        readDataSetInfo(m_fileID, data_info, info.nxname);
      } catch (...) {
        NXclosedata(m_fileID);
        throw;
      }
      const NXstatus closeStatus = NXclosedata(m_fileID);
      if (closeStatus != NXstatus::NX_OK)
        throw Exception("Failed to close dataset " + info.nxname, closeStatus);
      data_info.nxname = info.nxname;
      m_datasets->emplace_back(data_info);
    } else if (info.nxclass.substr(0, 2) == "NX" || info.nxclass.substr(0, 2) == "IX") {
      m_groups->emplace_back(info);
    }
  }
  reset();
}

void NXClass::open() {
  if (NXopengrouppath(m_fileID, m_path.c_str()) == NXstatus::NX_ERROR) {

    throw std::runtime_error("Cannot open group " + name() + " of class " + NX_class() + " (trying to open path " +
                             m_path + ")");
  }
  //}
  m_open = true;
  readAllInfo();
}

void NXClass::reset() { NXinitgroupdir(m_fileID); }

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
  NXstatus stat = NXopen(m_filename.c_str(), NXACC_READ, &m_fileID);
  if (stat == NXstatus::NX_ERROR) {
    std::cout << "NXRoot: Error loading " << m_filename;
    throw Exception("Unable to open File: " + m_filename);
  }
  readAllInfo();
}

NXRoot::~NXRoot() { NXclose(&m_fileID); }

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
  if (NXopenpath(m_fileID, group_path.c_str()) == NXstatus::NX_ERROR) {
    throw std::runtime_error("Cannot open dataset " + m_path);
  }
  if (NXopendata(m_fileID, name().c_str()) != NXstatus::NX_OK) {
    throw std::runtime_error("Error opening data in group \"" + name() + "\"");
  }

  try {
    readDataSetInfo(m_fileID, m_info, name());
    getAttributes();
  } catch (...) {
    NXclosedata(m_fileID);
    throw;
  }
  if (NXclosedata(m_fileID) != NXstatus::NX_OK)
    throw Exception("Failed to close dataset " + name(), NXstatus::NX_ERROR);
}

void NXDataSet::openLocal() {
  if (NXopendata(m_fileID, name().c_str()) != NXstatus::NX_OK) {
    throw std::runtime_error("Error opening data in group \"" + name() + "\"");
  }
  try {
    readDataSetInfo(m_fileID, m_info, name());
    getAttributes();
  } catch (...) {
    NXclosedata(m_fileID);
    throw;
  }
  if (NXclosedata(m_fileID) != NXstatus::NX_OK)
    throw Exception("Failed to close dataset " + name(), NXstatus::NX_ERROR);
}

/**
 * The size of the first dimension of data
 * @returns An integer indicating the size of the dimension.
 * @throws out_of_range error if requested on an object of rank 0
 */
int NXDataSet::dim0() const {
  if (m_info.rank == 0) {
    throw std::out_of_range("NXDataSet::dim0() - Requested dimension greater than rank.");
  }
  return m_info.dims[0];
}

/**
 * The size of the second dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 2
 */
int NXDataSet::dim1() const {
  if (m_info.rank < 2) {
    throw std::out_of_range("NXDataSet::dim1() - Requested dimension greater than rank.");
  }
  return m_info.dims[1];
}

/**
 * The size of the third dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 3
 */
int NXDataSet::dim2() const {
  if (m_info.rank < 3) {
    throw std::out_of_range("NXDataSet::dim2() - Requested dimension greater than rank.");
  }
  return m_info.dims[2];
}

/**
 * The size of the fourth dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 4
 */
int NXDataSet::dim3() const {
  if (m_info.rank < 4) {
    throw std::out_of_range("NXDataSet::dim3() - Requested dimension greater than rank.");
  }
  return m_info.dims[3];
}

/**  Wrapper to the NXgetdata.
 *   @param data :: The pointer to the buffer accepting the data from the file.
 *   @throw runtime_error if the operation fails.
 */
void NXDataSet::getData(void *data) {
  NXopendata(m_fileID, name().c_str());
  if (NXgetdata(m_fileID, data) != NXstatus::NX_OK)
    throw std::runtime_error("Cannot read data from NeXus file");
  NXclosedata(m_fileID);
}

//---------------------------------------------------------
//          NXData methods
//---------------------------------------------------------

NXData::NXData(const NXClass &parent, const std::string &name) : NXClass(parent, name) {}

} // namespace Mantid::LegacyNexus
