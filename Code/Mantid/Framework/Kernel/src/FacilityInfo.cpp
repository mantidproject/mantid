//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/StringTokenizer.h>

#include <algorithm>
#include <iostream>

namespace Mantid
{
namespace Kernel
{

Logger& FacilityInfo::g_log(Logger::get("FacilityInfo"));

/** Constructor.
  * @param elem :: The Poco::XML::Element to read the data from
  * @throw std::runtime_error if name or file extensions are not defined
  */
FacilityInfo::FacilityInfo(const Poco::XML::Element* elem) : 
  m_name(elem->getAttribute("name")), m_zeroPadding(0), m_delimiter(), m_extensions(),
  m_soapEndPoint(), m_archiveSearch(), m_instruments(), m_catalogName(), m_liveListener()
{
  if (m_name.empty())
  {
    g_log.error("Facility name is not defined");
    throw std::runtime_error("Facility name is not defined");
  }

  // Fill the various fields from the XML
  fillZeroPadding(elem);
  fillDelimiter(elem);
  fillExtensions(elem);
  fillSoapEndPoint(elem);
  fillArchiveNames(elem);
  fillCatalogName(elem);
  fillInstruments(elem);
}

/// Called from constructor to fill zero padding field
void FacilityInfo::fillZeroPadding(const Poco::XML::Element* elem)
{
  std::string paddingStr = elem->getAttribute("zeropadding");
  if ( paddingStr.empty() || !Mantid::Kernel::Strings::convert(paddingStr,m_zeroPadding) )
  {
    m_zeroPadding = 0;
  }
}

/// Called from constructor to fill default delimiter
void FacilityInfo::fillDelimiter(const Poco::XML::Element* elem)
{
  // The string to separate the instrument name and the run number.
  m_delimiter = elem->getAttribute("delimiter");
}

/// Called from constructor to fill file extensions
void FacilityInfo::fillExtensions(const Poco::XML::Element* elem)
{
  std::string extsStr = elem->getAttribute("FileExtensions");
  if (extsStr.empty())
  {
    g_log.error("No file extensions defined");
    throw std::runtime_error("No file extensions defined");
  }
  typedef Poco::StringTokenizer tokenizer;
  tokenizer exts(extsStr, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (tokenizer::Iterator it = exts.begin(); it != exts.end(); ++it)
  {
    addExtension(*it);
  }
}

/**
  * Add new extension. Adds both a lowercase and uppercase version
  * @param ext :: File extension, including the dot, e.g. ".nxs" or ".raw"
  */
void FacilityInfo::addExtension(const std::string& ext)
{
  std::string casedExt(ext);
  std::transform(ext.begin(), ext.end(), casedExt.begin(), tolower);
  std::vector<std::string>::iterator it = std::find(m_extensions.begin(),m_extensions.end(),casedExt);
  if (it == m_extensions.end())
  {
    m_extensions.push_back(casedExt);
    std::transform(ext.begin(), ext.end(), casedExt.begin(), toupper);
    m_extensions.push_back(casedExt);
  }
}

/// Called from constructor to fill ICAT soap end point
void FacilityInfo::fillSoapEndPoint(const Poco::XML::Element* elem)
{
  Poco::XML::NodeList* pNL_soapEndPoint = elem->getElementsByTagName("soapEndPoint");

  if (pNL_soapEndPoint->length() > 1)
  {
    pNL_soapEndPoint->release();
    g_log.error("Facility must have only one soapEndPoint tag");
    throw std::runtime_error("Facility must have only one csoapEndPoint tag");
  }

  Poco::XML::Element* endpoint = dynamic_cast<Poco::XML::Element*>(pNL_soapEndPoint->item(0));
  if(!endpoint->getAttribute("url").empty())
  {
    m_soapEndPoint= endpoint->getAttribute("url");
  }
  pNL_soapEndPoint->release();
}

/// Called from constructor to fill archive interface names
void FacilityInfo::fillArchiveNames(const Poco::XML::Element* elem)
{
  Poco::XML::NodeList* pNL_archives = elem->getElementsByTagName("archive");
  if (pNL_archives->length() > 1)
  {
    g_log.error("Facility must have only one archive tag");
    throw std::runtime_error("Facility must have only one archive tag");
  }
  else if (pNL_archives->length() == 1)
  {
    Poco::XML::NodeList* pNL_interfaces = elem->getElementsByTagName("archiveSearch");
    for (unsigned int i = 0; i < pNL_interfaces->length(); ++i)
    {
      Poco::XML::Element* elem = dynamic_cast<Poco::XML::Element*>(pNL_interfaces->item(i));
      std::string plugin = elem->getAttribute("plugin");
      if (!plugin.empty())
      {
        m_archiveSearch.push_back(plugin);
      }
    }
    pNL_interfaces->release();
  }
  pNL_archives->release();
}

/// Called from constructor to fill catalog name
void FacilityInfo::fillCatalogName(const Poco::XML::Element* elem)
{
  Poco::XML::NodeList* pNL_catalogs = elem->getElementsByTagName("catalog");

  if (pNL_catalogs->length() > 1)
  {
    g_log.error("Facility must have only one catalog tag");
    throw std::runtime_error("Facility must have only one catalog tag");
  }

  Poco::XML::Element* catalog = dynamic_cast<Poco::XML::Element*>(pNL_catalogs->item(0));
  if(!catalog->getAttribute("name").empty())
  {
    m_catalogName= catalog->getAttribute("name");
  }

  pNL_catalogs->release();
}

/// Called from constructor to fill instrument list
void FacilityInfo::fillInstruments(const Poco::XML::Element* elem)
{
  Poco::XML::NodeList* pNL_instrument = elem->getElementsByTagName("instrument");
  unsigned long n = pNL_instrument->length();
  m_instruments.reserve(n);

  for (unsigned long i = 0; i < n; ++i)
  {
    Poco::XML::Element* elem = dynamic_cast<Poco::XML::Element*>(pNL_instrument->item(i));
    if (elem)
    {
      try
      {
        InstrumentInfo instr(this,elem);
        m_instruments.push_back(instr);
      }
      catch(...)
      {/*skip this instrument*/}
    }
  }
  pNL_instrument->release();

  if (m_instruments.empty())
  {
    throw std::runtime_error("Facility "+m_name+" does not have any instruments;");
  }
}

/**
  * Returns instrument with given name
  * @param  iName Instrument name
  * @return the instrument information object
  * @throw NotFoundError if iName was not found
  */
const InstrumentInfo & FacilityInfo::instrument(const std::string& iName)const
{
  std::string iname;
  if (iName.empty())
  {
    iname = ConfigService::Instance().getString("default.instrument");
    g_log.debug() << "Blank instrument specified, using default instrument of " << iname << "." << std::endl;
    if (iname.empty())
    {
      return m_instruments.front();
    }
  }
  else
  {
    iname = iName;
  }
  // All of our instrument names are upper case
  std::transform(iname.begin(), iname.end(), iname.begin(), toupper);
  std::vector<InstrumentInfo>::const_iterator it = m_instruments.begin();
  for(;it != m_instruments.end(); ++it)
  {
    if (it->name() == iname)
    {
      g_log.debug() << "Instrument '" << iName << "' found as " << it->name() << " at " << name() << "." << std::endl;
      return *it;
    }
  }

  // if unsuccessful try shortname
  for(it = m_instruments.begin(); it != m_instruments.end(); ++it)
  {
    if (it->shortName() == iname)
    {
      g_log.debug() << "Instrument '" << iName << "' found as " << it->name() << " at " << name() << "." << std::endl;
      return *it;
    }
  }
  g_log.debug("Instrument "+iName+" not found in facility "+name());
  throw Exception::NotFoundError("FacilityInfo",iname);
}

/**
  * Returns a list of instruments of given technique
  * @param tech :: Technique name
  * @return a list of instrument information objects
  */
std::vector<InstrumentInfo> FacilityInfo::instruments(const std::string& tech)const
{
  std::vector<InstrumentInfo> out;
  std::vector<InstrumentInfo>::const_iterator it = m_instruments.begin();
  for(;it != m_instruments.end(); ++it)
  {
    if (it->techniques().count(tech))
    {
      out.push_back(*it);
    }
  }
  return out;
}

} // namespace Kernel
} // namespace Mantid
