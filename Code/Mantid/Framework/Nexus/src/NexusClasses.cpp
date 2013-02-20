//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/NexusClasses.h"
#include "MantidKernel/Exception.h"
#include <cstdio>
#include <nexus/NeXusException.hpp>

namespace Mantid
{
namespace NeXus
{

  

std::vector<std::string> NXAttributes::names()const
{
    std::vector<std::string> out;
    std::map<std::string,std::string>::const_iterator it = m_values.begin();
    for(;it!=m_values.end();++it)
        out.push_back(it->first);
    return out;
}

std::vector<std::string> NXAttributes::values()const
{
    std::vector<std::string> out;
    std::map<std::string,std::string>::const_iterator it = m_values.begin();
    for(;it!=m_values.end();++it)
        out.push_back(it->second);
    return out;
}

/**  Returns the value of an attribute
 *   @param name :: The name of the attribute
 *   @return The value of the attribute if it exists or an empty string otherwise
 */
std::string NXAttributes::operator()(const std::string& name)const
{
    std::map<std::string,std::string>::const_iterator it = m_values.find(name);
    if (it == m_values.end()) return "";
    return it->second;
}

/**  Sets the value of the attribute.
 *   @param name :: The name of the attribute
 *   @param value :: The new value of the attribute
 */
void NXAttributes::set(const std::string &name, const std::string &value)
{
    m_values[name] = value;
}

/**  Sets the value of the attribute as a double.
 *   @param name :: The name of the attribute
 *   @param value :: The new value of the attribute
 */
void NXAttributes::set(const std::string &name, double value)
{
    std::ostringstream ostr;
    ostr << value;
    m_values[name] = ostr.str();
}


//---------------------------------------------------------
//          NXObject methods
//---------------------------------------------------------

/**  NXObject constructor.
 *   @param fileID :: The Nexus file id
 *   @param parent :: The parent Nexus class. In terms of HDF it is the group containing the object.
 *   @param name :: The name of the object relative to its parent
 */
NXObject::NXObject(boost::shared_ptr< ::NeXus::File> handle,const NXClass* parent,const std::string& name):
  m_handle(handle),m_open(false)
{
    if (parent && !name.empty())
    {
        m_path = parent->path() + "/" + name;
    }
}

std::string NXObject::name()const
{
    size_t i = m_path.find_last_of('/');
    if (i == std::string::npos)
        return m_path;
    else
        return m_path.substr(i+1,m_path.size()-i-1);
}

/**  Reads in attributes
 */
void NXObject::getAttributes()
{
  std::vector< ::NeXus::AttrInfo> attrInfos =  m_handle->getAttrInfos();
  for (auto info = attrInfos.begin(); info != attrInfos.end(); ++info)
  {
    std::string avalue("");
    switch(info->type)
    {
    case (::NeXus::CHAR):
    {
      avalue = m_handle->getStrAttr(*info);
      break;
    }
    case(::NeXus::INT16):
    {
      avalue = boost::lexical_cast<std::string>(m_handle->getAttr<int16_t>(*info));
      break;
    }
    case(::NeXus::INT32):
    {
      avalue = boost::lexical_cast<std::string>(m_handle->getAttr<int32_t>(*info));
      break;
    }
    case(::NeXus::UINT16):
    {
      avalue = boost::lexical_cast<std::string>(m_handle->getAttr<uint16_t>(*info));
      break;
    }
    default:
    {
      // intentionally ignore
    }
    }
    if (! avalue.empty())
      attributes.set(info->name, avalue);
  }
}
//---------------------------------------------------------
//          NXClass methods
//---------------------------------------------------------

NXClass::NXClass(const NXClass& parent, const std::string& name):
  NXObject(parent.m_handle,&parent,name)
{
    clear();
}

void NXClass::readAllInfo()
{
    clear();
    std::map<std::string, std::string> entries = m_handle->getEntries();
    for (auto entry = entries.begin(); entry != entries.end(); ++entry)
    {
      if (entry->second == "SDS")
      {
        m_handle->openData(entry->first);
        ::NeXus::Info nxinfo = m_handle->getInfo();
        m_handle->closeData();

        NXInfo data_info;
        data_info.nxname = entry->first;
        data_info.rank = static_cast<int>(nxinfo.dims.size());
        for (int i = 0; i < data_info.rank; i++)
          data_info.dims[i] = static_cast<int>(nxinfo.dims[i]);
        data_info.type = nxinfo.type;
        data_info.stat = NX_OK;

        m_datasets->push_back(data_info);
      }
      else if (boost::algorithm::starts_with(entry->second, "NX")
               || boost::algorithm::starts_with(entry->second, "IX"))
      {
        NXClassInfo info;
        info.nxname = entry->first;
        info.nxclass = entry->second;
        m_groups->push_back(info);
      }
    }
}

bool NXClass::isValid(const std::string & path) const
{
  try{
    m_handle->openGroupPath(path);
    m_handle->closeGroup();
    return true;
  }
  catch (::NeXus::Exception &e)
  {
    return false;
  }
}

void NXClass::open()
{
  m_handle->openGroupPath(m_path);
  m_open = true;
  readAllInfo();
}

/** It is fast, but the parent of this class must be open at
 * the time of calling. openNXClass uses open() (the slow one). To open calss using openLocal() do:
 *    NXTheClass class(parent,name);
 *    class.openLocal();
 *    // work with class
 *    class.close();
 * @param nxclass :: The NX class name. If empty NX_class() will be used
 * @return true if OK
 */
void NXClass::openLocal(const std::string& nxclass)
{
  std::string className = nxclass.empty()? NX_class() : nxclass;
  m_handle->openGroup(name(), className);
  m_open = true;
  readAllInfo();
}

void NXClass::close()
{
    m_handle->closeGroup();
    m_open = false;
}

void NXClass::clear()
{
    m_groups.reset(new std::vector<NXClassInfo>);
    m_datasets.reset(new std::vector<NXInfo>);
}

std::string NXClass::getString(const std::string& name)const
{
    NXChar buff = openNXChar(name);
    try
    {
      buff.load();
      return std::string(buff(),buff.dim0());
    }
    catch(std::runtime_error &)
    {
        // deals with reading uninitialized/empty data
        return std::string();
    }
}

double NXClass::getDouble(const std::string& name)const
{
    NXDouble number = openNXDouble(name);
    number.load();
    return *number();
}

float NXClass::getFloat(const std::string& name)const
{
    NXFloat number = openNXFloat(name);
    number.load();
    return *number();
}

int NXClass::getInt(const std::string& name)const
{
    NXInt number = openNXInt(name);
    number.load();
    return *number();
}
/** Returns whether an individual group (or group) is present
*  @param query :: the class name to search for
*  @return true if the name is found and false otherwise
*/
bool NXClass::containsGroup(const std::string & query) const
{
  std::vector<NXClassInfo>::const_iterator end = m_groups->end();
  for(std::vector<NXClassInfo>::const_iterator i=m_groups->begin(); i!=end;++i)
  {
    if ( i->nxname == query )
    {
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
NXInfo NXClass::getDataSetInfo(const std::string& name)const
{
    NXInfo info;
    for(std::vector<NXInfo>::const_iterator it=datasets().begin();it!=datasets().end();++it)
    {
        if (it->nxname == name) return *it;
    }
    info.stat = NX_ERROR;
    return info;
}

/**
  * Returns whether an individual dataset is present.
  */
bool NXClass::containsDataSet(const std::string & query) const
{
  return getDataSetInfo( query ).stat != NX_ERROR;
}

//---------------------------------------------------------
//          NXNote methods
//---------------------------------------------------------

std::string NXNote::author()
{
    if (!m_author_ok)
    {
        NXChar aut = openNXChar("author");
        aut.load();
        m_author = std::string(aut(),aut.dim0());
        m_author_ok = true;
    }
    return m_author;
}

std::vector< std::string >& NXNote::data()
{
    if (!m_data_ok)
    {
      m_handle->openData("data");
      ::NeXus::Info info = m_handle->getInfo();
      int n = static_cast<int>(info.dims[0]);
      char* buffer = new char[n];
      m_data.clear();
      try
      {
        m_handle->getData(buffer);
      }
      catch (::NeXus::Exception &e)
      {
        delete[] buffer;
        return m_data;
      }
      m_handle->closeData();

      std::istringstream istr(std::string(buffer,n));
      delete[] buffer;

      std::string line;
      while(getline(istr,line))
      {
        m_data.push_back(line);
      }

      m_data_ok = true;
    }
    return m_data;
}

std::string NXNote::description()
{
    if (!m_description_ok)
    {
        NXChar str = openNXChar("description");
        str.load();
        m_description = std::string(str(),str.dim0());
        m_description_ok = true;
    }
    return m_description;
}

std::vector<char>& NXBinary::binary()
{
    if (!m_data_ok)
    {
      m_handle->openData("data");
      ::NeXus::Info info = m_handle->getInfo();
      m_binary.resize(info.dims[0]);
      m_handle->getData(&m_binary[0]);
      m_handle->closeData();
    }
    return m_binary;
}

//---------------------------------------------------------
//          NXRoot methods
//---------------------------------------------------------

NXRoot::NXRoot(boost::shared_ptr< ::NeXus::File> handle)
{
  m_handle = handle;
  readAllInfo();
}

/**  Constructor. On creation opens the Nexus file for reading only.
 *   @param fname :: The file name to open
 */
NXRoot::NXRoot(const std::string& fname)
    :m_filename(fname)
{
    m_handle = boost::make_shared< ::NeXus::File >(m_filename, NXACC_READ);
    readAllInfo();
}

/**  Constructor.
 *   Creates a new Nexus file. The first root entry will be also created.
 *   @param fname :: The file name to create
 *   @param entry :: The name of the first entry in the new file
 */
NXRoot::NXRoot(const std::string& fname,const std::string& entry)
    :m_filename(fname)
{
    (void)entry;
    // Open NeXus file
    m_handle = boost::make_shared< ::NeXus::File >(m_filename, NXACC_CREATE5);
}

NXRoot::~NXRoot()
{
  // file will close when shared pointer goes out of scope
}

bool NXRoot::isStandard()const
{
    return true;
}

/**
 * Open the first NXentry in the file.
 */
NXEntry NXRoot::openFirstEntry()
{
  if (groups().empty())
  {
    throw std::runtime_error("NeXus file has no entries");
  }
  for(std::vector<NXClassInfo>::const_iterator grp = groups().begin(); grp != groups().end(); ++grp)
  {
    if (grp->nxclass == "NXentry")
    {
      return openEntry(grp->nxname);
    }
  }
  throw std::runtime_error("NeXus file has no entries");
}

//---------------------------------------------------------
//          NXDataSet methods
//---------------------------------------------------------

/**  Constructor.
 *   @param parent :: The parent Nexus class. In terms of HDF it is the group containing the dataset.
 *   @param name :: The name of the dataset relative to its parent
 */
NXDataSet::NXDataSet(const NXClass& parent,const std::string& name)
    :NXObject(parent.m_handle,&parent,name)
{
  size_t i = name.find_last_of('/');
  if (i == std::string::npos)
    m_info.nxname = name;
  else if (name.empty() || i == name.size()-1)
    throw std::runtime_error("Improper dataset name "+name);
  else
    m_info.nxname = name.substr(i+1);
}

// Opens the data set. Does not read in any data. Call load(...) to load the data
void NXDataSet::open()
{
  size_t i = m_path.find_last_of('/');
  if (i == std::string::npos || i == 0) return; // we are in the root group, assume it is open
  std::string group_path = m_path.substr(0,i);
  m_handle->openPath(group_path);
  m_handle->openData(name());
  ::NeXus::Info nxinfo = m_handle->getInfo();
  m_info.rank = static_cast<int>(nxinfo.dims.size());
  for (int i = 0; i < m_info.rank; ++i)
    m_info.dims[i] = static_cast<int>(nxinfo.dims[i]);
  m_info.type = nxinfo.type;

  getAttributes();
  m_handle->closeData();
}

void NXDataSet::openLocal()
{
  m_handle->openData(name());
  ::NeXus::Info nxinfo = m_handle->getInfo();
  m_info.rank = static_cast<int>(nxinfo.dims.size());
  for (int i = 0; i < m_info.rank; ++i)
    m_info.dims[i] = static_cast<int>(nxinfo.dims[i]);
  m_info.type = nxinfo.type;
  getAttributes();
  m_handle->closeData();
}

/**
 * The size of the first dimension of data
 * @returns An integer indicating the size of the dimension.
 * @throws out_of_range error if requested on an object of rank 0
 */
int NXDataSet::dim0() const
{
  if( m_info.rank == 0 )
  {
    throw std::out_of_range("NXDataSet::dim0() - Requested dimension greater than rank.");
  }
  return m_info.dims[0];
}

/**
 * The size of the second dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 2
 */
int NXDataSet::dim1() const
{
  if( m_info.rank < 2 )
  {
    throw std::out_of_range("NXDataSet::dim1() - Requested dimension greater than rank.");
  }
  return m_info.dims[1];
}

/**
 * The size of the third dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 3
 */
int NXDataSet::dim2() const
{
  if( m_info.rank < 3 )
  {
    throw std::out_of_range("NXDataSet::dim2() - Requested dimension greater than rank.");
  }
  return m_info.dims[2];
}

/**
 * The size of the fourth dimension of data
 * @returns An integer indicating the size of the dimension
 * @throws out_of_range error if requested on an object of rank < 4
 */
int NXDataSet::dim3() const
{
  if( m_info.rank < 4 )
  {
    throw std::out_of_range("NXDataSet::dim3() - Requested dimension greater than rank.");
  }
  return m_info.dims[3];
}

/**  Wrapper to the NXgetdata.
 *   @param data :: The pointer to the buffer accepting the data from the file.
 *   @throw runtime_error if the operation fails.
 */
void NXDataSet::getData(void* data)
{
  m_handle->openData(name());
  m_handle->getData(data);
  m_handle->closeData();
}

/**  Wrapper to the NXgetslab.
 *   @param data :: The pointer to the buffer accepting the data from the file.
 *   @param start :: The array of starting indeces to read in from the file. The size of the array must be equal to 
 *          the rank of the data.
 *   @param size :: The array of numbers of data elements to read along each dimenstion.
 *          The number of dimensions (the size of the array) must be equal to the rank of the data.
 *   @throw runtime_error if the operation fails.
 */
void NXDataSet::getSlab(void* data, int start[], int size[])
{
  m_handle->openData(name());
  std::vector<int64_t> startVec(start, start+m_info.rank);
  std::vector<int64_t> sizeVec(size, size+m_info.rank);
  m_handle->getSlab(data, startVec, sizeVec);
  m_handle->closeData();
}


//---------------------------------------------------------
//          NXData methods
//---------------------------------------------------------

NXData::NXData(const NXClass& parent,const std::string& name):NXMainClass(parent,name)
{
}

//---------------------------------------------------------
//          NXLog methods
//---------------------------------------------------------

/** Creates a property wrapper around the log entry
 * @returns A valid property pointer or NULL
 */
Kernel::Property * NXLog::createProperty()
{
  NXInfo vinfo = getDataSetInfo("time");
  if( vinfo.stat == NX_ERROR )
  {
    return createSingleValueProperty();
  }
  else
  {
    return createTimeSeries();
  }
}

/** Creates a single value property of the log
 * @returns A pointer to a newly created property wrapped around the log entry
 */
Kernel::Property* NXLog::createSingleValueProperty()
{
  const std::string valAttr("value");
  NXInfo vinfo = getDataSetInfo(valAttr);
  Kernel::Property *prop;
  int nxType = vinfo.type;
  if( nxType == NX_FLOAT64 )
  {
    prop = new Kernel::PropertyWithValue<double>(name(), getDouble(valAttr));
  }
  else if( nxType == NX_INT32 )
  {
    prop = new Kernel::PropertyWithValue<int>(name(), getInt(valAttr));
  }
  else if( nxType == NX_CHAR )
  {
    prop = new Kernel::PropertyWithValue<std::string>(name(), getString(valAttr));
  }
  else if( nxType == NX_UINT8 )
  {
    NXDataSetTyped<unsigned char> value(*this, valAttr);
    value.load();
    bool state  = (value[0] == 0 ) ? false : true;
    prop = new Kernel::PropertyWithValue<bool>(name(), state);
  }
  else
  {
    prop = NULL;
  }

  return prop;
}

/** createTimeSeries
 * Create a TimeSeries property form the records of the NXLog group. Times are in dataset "time"
 * and the values are in dataset "value"
 * @param start_time :: If the "time" dataset does not have the "start" attribute sets the
 *   start time for the series.
 * @param new_name :: If not empty it is used as the TimeSeries property name
 *   @return The property or NULL
 */
Kernel::Property* NXLog::createTimeSeries(const std::string& start_time,const std::string& new_name)
{
  const std::string & logName = new_name.empty()? name(): new_name;
  NXInfo vinfo = getDataSetInfo("time");
  if( vinfo.type == NX_FLOAT64)
  {
    NXDouble times(*this,"time");
    times.openLocal();
    times.load();
    std::string units = times.attributes("units");
    if (units == "minutes")
    {
      std::transform(times(),times()+times.dim0(),times(),std::bind2nd(std::multiplies<double>(),60));
    }
    else if (!units.empty() && units.substr(0,6) != "second")
    {
      return NULL;
    }
    return parseTimeSeries(logName, times, start_time);
  }
  else if( vinfo.type == NX_FLOAT32 )
  {
    NXFloat times(*this,"time");
    times.openLocal();
    times.load();
    std::string units = times.attributes("units");
    if (units == "minutes")
    {
      std::transform(times(),times()+times.dim0(),times(),std::bind2nd(std::multiplies<float>(),60));
    }
    else if (!units.empty() && units.substr(0,6) != "second")
    {
      return NULL;
    }
    return parseTimeSeries(logName, times, start_time);
  }

  return NULL;
}



} // namespace DataHandling
} // namespace Mantid
