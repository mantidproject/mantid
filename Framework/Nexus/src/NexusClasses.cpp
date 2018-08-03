//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace NeXus {

std::vector<std::string> NXAttributes::names() const {
  std::vector<std::string> out;
  out.reserve(m_values.size());
  for (const auto &value : m_values)
    out.push_back(value.first);
  return out;
}

std::vector<std::string> NXAttributes::values() const {
  std::vector<std::string> out;
  out.reserve(m_values.size());
  for (const auto &value : m_values)
    out.push_back(value.second);
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
void NXAttributes::set(const std::string &name, const std::string &value) {
  m_values[name] = value;
}

/**  Sets the value of the attribute as a double.
 *   @param name :: The name of the attribute
 *   @param value :: The new value of the attribute
 */
void NXAttributes::set(const std::string &name, double value) {
  std::ostringstream ostr;
  ostr << value;
  m_values[name] = ostr.str();
}

//---------------------------------------------------------
//          NXObject methods
//---------------------------------------------------------

/**  NXObject constructor.
 *   @param fileID :: The Nexus file id
 *   @param parent :: The parent Nexus class. In terms of HDF it is the group
 * containing the object.
 *   @param name :: The name of the object relative to its parent
 */
NXObject::NXObject(const NXhandle fileID, const NXClass *parent,
                   const std::string &name)
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
  int iLength, iType;
#ifndef NEXUS43
  int rank;
  int dims[4];
#endif
  int nbuff = 127;
  boost::shared_array<char> buff(new char[nbuff + 1]);

#ifdef NEXUS43
  while (NXgetnextattr(m_fileID, pName, &iLength, &iType) != NX_EOD) {
#else
  while (NXgetnextattra(m_fileID, pName, &rank, dims, &iType) != NX_EOD) {
#endif
// std::cerr<<"--------------------------\n";
// std::cerr<<"name="<<path()<<'\n';
// std::cerr<<pName<<' ' <<iLength<<' '<<iType<<'\n';
#ifndef NEXUS43
    if (rank > 1) { // mantid only supports single value attributes
      throw std::runtime_error(
          "Encountered attribute with multi-dimensional array value");
    }
    iLength = dims[0]; // to clarify things
    if (iType != NX_CHAR && iLength != 1) {
      throw std::runtime_error("Encountered attribute with array value");
    }
#endif

    switch (iType) {
    case NX_CHAR: {
      if (iLength > nbuff + 1) {
        nbuff = iLength;
        buff.reset(new char[nbuff + 1]);
      }
      int nz = iLength + 1;
      NXgetattr(m_fileID, pName, buff.get(), &nz, &iType);
      attributes.set(pName, buff.get());
      // std::cerr<<"value="<<buff.get()<<'\n';
      break;
    }
    case NX_INT16: {
      short int value;
      NXgetattr(m_fileID, pName, &value, &iLength, &iType);
      sprintf(buff.get(), "%i", value);
      attributes.set(pName, buff.get());
      break;
    }
    case NX_INT32: {
      int value;
      NXgetattr(m_fileID, pName, &value, &iLength, &iType);
      sprintf(buff.get(), "%i", value);
      attributes.set(pName, buff.get());
      break;
    }
    case NX_UINT16: {
      short unsigned int value;
      NXgetattr(m_fileID, pName, &value, &iLength, &iType);
      sprintf(buff.get(), "%u", value);
      attributes.set(pName, buff.get());
      break;
    }
    }
  };
}
//---------------------------------------------------------
//          NXClass methods
//---------------------------------------------------------

NXClass::NXClass(const NXClass &parent, const std::string &name)
    : NXObject(parent.m_fileID, &parent, name) {
  clear();
}

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
      NXopendata(m_fileID, info.nxname.c_str());
      data_info.stat =
          NXgetinfo(m_fileID, &data_info.rank, data_info.dims, &data_info.type);
      NXclosedata(m_fileID);
      data_info.nxname = info.nxname;
      m_datasets->push_back(data_info);
    } else if (info.nxclass.substr(0, 2) == "NX" ||
               info.nxclass.substr(0, 2) == "IX") {
      m_groups->push_back(info);
    }
    // std::cerr<<'!'<<info.nxname<<'\n';
  }
  reset();
}

bool NXClass::isValid(const std::string &path) const {
  if (NXopengrouppath(m_fileID, path.c_str()) == NX_OK) {
    NXclosegroup(m_fileID);
    return true;
  } else
    return false;
}

void NXClass::open() {
  // if (NX_ERROR == NXopengroup(m_fileID,name().c_str(),NX_class().c_str()))
  //{
  if (NX_ERROR == NXopengrouppath(m_fileID, m_path.c_str())) {

    throw std::runtime_error("Cannot open group " + name() + " of class " +
                             NX_class() + " (trying to open path " + m_path +
                             ")");
  }
  //}
  m_open = true;
  readAllInfo();
}

/** It is fast, but the parent of this class must be open at
 * the time of calling. openNXClass uses open() (the slow one). To open calss
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
  if (NX_ERROR == NXopengroup(m_fileID, name().c_str(), className.c_str())) {
    // It would be nice if this worked
    // if (NX_ERROR == NXopengrouppath(m_fileID,m_path.c_str()))
    //{
    //  throw std::runtime_error("Cannot open group "+m_path+" of class
    //  "+NX_class());
    //}
    return false;
  }
  m_open = true;
  readAllInfo();
  return true;
}

void NXClass::close() {
  if (NX_ERROR == NXclosegroup(m_fileID)) {
    throw std::runtime_error("Cannot close group " + name() + " of class " +
                             NX_class() + " (trying to close path " + m_path +
                             ")");
  }
  m_open = false;
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
  std::vector<NXClassInfo>::const_iterator end = m_groups->end();
  for (std::vector<NXClassInfo>::const_iterator i = m_groups->begin(); i != end;
       ++i) {
    if (i->nxname == query) {
      return true;
    }
  }
  return false;
}

/**
  *  Returns NXInfo for a dataset
  *  @param name :: The name of the dataset
  *  @return NXInfo::stat is set to NX_ERROR if the dataset does not exist
  */
NXInfo NXClass::getDataSetInfo(const std::string &name) const {
  NXInfo info;
  for (std::vector<NXInfo>::const_iterator it = datasets().begin();
       it != datasets().end(); ++it) {
    if (it->nxname == name)
      return *it;
  }
  info.stat = NX_ERROR;
  return info;
}

/**
  * Returns whether an individual dataset is present.
  */
bool NXClass::containsDataSet(const std::string &query) const {
  return getDataSetInfo(query).stat != NX_ERROR;
}

//---------------------------------------------------------
//          NXNote methods
//---------------------------------------------------------

std::string NXNote::author() {
  if (!m_author_ok) {
    NXChar aut = openNXChar("author");
    aut.load();
    m_author = std::string(aut(), aut.dim0());
    m_author_ok = true;
  }
  return m_author;
}

std::vector<std::string> &NXNote::data() {
  if (!m_data_ok) {
    int rank;
    int dims[4];
    int type;
    NXopendata(m_fileID, "data");
    NXgetinfo(m_fileID, &rank, dims, &type);
    int n = dims[0];
    auto buffer = new char[n];
    NXstatus stat = NXgetdata(m_fileID, buffer);
    NXclosedata(m_fileID);
    m_data.clear();
    if (stat == NX_ERROR) {
      delete[] buffer;
      return m_data;
    }
    std::istringstream istr(std::string(buffer, n));
    delete[] buffer;

    std::string line;
    while (getline(istr, line)) {
      m_data.push_back(line);
    }

    m_data_ok = true;
  }
  return m_data;
}

std::string NXNote::description() {
  if (!m_description_ok) {
    NXChar str = openNXChar("description");
    str.load();
    m_description = std::string(str(), str.dim0());
    m_description_ok = true;
  }
  return m_description;
}

std::vector<char> &NXBinary::binary() {
  if (!m_data_ok) {
    int rank;
    int dims[4];
    int type;
    NXopendata(m_fileID, "data");
    NXgetinfo(m_fileID, &rank, dims, &type);
    int n = dims[0];
    m_binary.resize(n);
    NXstatus stat = NXgetdata(m_fileID, &m_binary[0]);
    (void)stat; // Avoid unused variable compiler warning
    NXclosedata(m_fileID);
  }
  return m_binary;
}

//---------------------------------------------------------
//          NXRoot methods
//---------------------------------------------------------

/**  Constructor. On creation opens the Nexus file for reading only.
 *   @param fname :: The file name to open
 */
NXRoot::NXRoot(const std::string &fname) : m_filename(fname) {
  // Open NeXus file
  NXstatus stat = NXopen(m_filename.c_str(), NXACC_READ, &m_fileID);
  if (stat == NX_ERROR) {
    std::cout << "NXRoot: Error loading " << m_filename;
    throw Kernel::Exception::FileError("Unable to open File:", m_filename);
  }
  readAllInfo();
}

/**  Constructor.
 *   Creates a new Nexus file. The first root entry will be also created.
 *   @param fname :: The file name to create
 *   @param entry :: The name of the first entry in the new file
 */
NXRoot::NXRoot(const std::string &fname, const std::string &entry)
    : m_filename(fname) {
  (void)entry;
  // Open NeXus file
  NXstatus stat = NXopen(m_filename.c_str(), NXACC_CREATE5, &m_fileID);
  if (stat == NX_ERROR) {
    throw Kernel::Exception::FileError("Unable to open File:", m_filename);
  }
}

NXRoot::~NXRoot() { NXclose(&m_fileID); }

bool NXRoot::isStandard() const { return true; }

/**
 * Open the first NXentry in the file.
 */
NXEntry NXRoot::openFirstEntry() {
  if (groups().empty()) {
    throw std::runtime_error("NeXus file has no entries");
  }
  for (std::vector<NXClassInfo>::const_iterator grp = groups().begin();
       grp != groups().end(); ++grp) {
    if (grp->nxclass == "NXentry") {
      return openEntry(grp->nxname);
    }
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
NXDataSet::NXDataSet(const NXClass &parent, const std::string &name)
    : NXObject(parent.m_fileID, &parent, name) {
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
  if (NX_ERROR == NXopenpath(m_fileID, group_path.c_str())) {
    throw std::runtime_error("Cannot open dataset " + m_path);
  }
  if (NXopendata(m_fileID, name().c_str()) != NX_OK) {
    throw std::runtime_error("Error opening data in group \"" + name() + "\"");
  }

  if (NXgetinfo(m_fileID, &m_info.rank, m_info.dims, &m_info.type) != NX_OK) {
    throw std::runtime_error("Error retrieving information for " + name() +
                             " group");
  }

  getAttributes();
  NXclosedata(m_fileID);
}

void NXDataSet::openLocal() {
  if (NXopendata(m_fileID, name().c_str()) != NX_OK) {
    throw std::runtime_error("Error opening data in group \"" + name() + "\"");
  }
  if (NXgetinfo(m_fileID, &m_info.rank, m_info.dims, &m_info.type) != NX_OK) {
    throw std::runtime_error("Error retrieving information for " + name() +
                             " group");
  }
  getAttributes();
  NXclosedata(m_fileID);
}

/**
 * The size of the first dimension of data
 * @returns An integer indicating the size of the dimension.
 * @throws out_of_range error if requested on an object of rank 0
 */
int NXDataSet::dim0() const {
  if (m_info.rank == 0) {
    throw std::out_of_range(
        "NXDataSet::dim0() - Requested dimension greater than rank.");
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
    throw std::out_of_range(
        "NXDataSet::dim1() - Requested dimension greater than rank.");
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
    throw std::out_of_range(
        "NXDataSet::dim2() - Requested dimension greater than rank.");
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
    throw std::out_of_range(
        "NXDataSet::dim3() - Requested dimension greater than rank.");
  }
  return m_info.dims[3];
}

/**  Wrapper to the NXgetdata.
 *   @param data :: The pointer to the buffer accepting the data from the file.
 *   @throw runtime_error if the operation fails.
 */
void NXDataSet::getData(void *data) {
  NXopendata(m_fileID, name().c_str());
  if (NXgetdata(m_fileID, data) != NX_OK)
    throw std::runtime_error("Cannot read data from NeXus file");
  NXclosedata(m_fileID);
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
void NXDataSet::getSlab(void *data, int start[], int size[]) {
  NXopendata(m_fileID, name().c_str());
  if (NXgetslab(m_fileID, data, start, size) != NX_OK)
    throw std::runtime_error("Cannot read data slab from NeXus file");
  NXclosedata(m_fileID);
}

//---------------------------------------------------------
//          NXData methods
//---------------------------------------------------------

NXData::NXData(const NXClass &parent, const std::string &name)
    : NXMainClass(parent, name) {}

//---------------------------------------------------------
//          NXLog methods
//---------------------------------------------------------

/** Creates a property wrapper around the log entry
 * @returns A valid property pointer or NULL
 */
Kernel::Property *NXLog::createProperty() {
  NXInfo vinfo = getDataSetInfo("time");
  if (vinfo.stat == NX_ERROR) {
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
  int nxType = vinfo.type;
  if (nxType == NX_FLOAT64) {
    prop = new Kernel::PropertyWithValue<double>(name(), getDouble(valAttr));
  } else if (nxType == NX_INT32) {
    prop = new Kernel::PropertyWithValue<int>(name(), getInt(valAttr));
  } else if (nxType == NX_CHAR) {
    prop =
        new Kernel::PropertyWithValue<std::string>(name(), getString(valAttr));
  } else if (nxType == NX_UINT8) {
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
Kernel::Property *NXLog::createTimeSeries(const std::string &start_time,
                                          const std::string &new_name) {
  const std::string &logName = new_name.empty() ? name() : new_name;
  NXInfo vinfo = getDataSetInfo("time");
  if (vinfo.type == NX_FLOAT64) {
    NXDouble times(*this, "time");
    times.openLocal();
    times.load();
    std::string units = times.attributes("units");
    if (units == "minutes") {
      std::transform(times(), times() + times.dim0(), times(),
                     std::bind2nd(std::multiplies<double>(), 60));
    } else if (!units.empty() && units.substr(0, 6) != "second") {
      return nullptr;
    }
    return parseTimeSeries(logName, times, start_time);
  } else if (vinfo.type == NX_FLOAT32) {
    NXFloat times(*this, "time");
    times.openLocal();
    times.load();
    std::string units = times.attributes("units");
    if (units == "minutes") {
      std::for_each(times(), times() + times.dim0(),
                    [](float &val) { val *= 60.0f; });
    } else if (!units.empty() && units.substr(0, 6) != "second") {
      return nullptr;
    }
    return parseTimeSeries(logName, times, start_time);
  }

  return nullptr;
}

} // namespace DataHandling
} // namespace Mantid
