//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <algorithm>
#include <iostream>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/StringTokenizer.h>

using Poco::XML::Element;

namespace Mantid {
namespace Kernel {

namespace {
/// static logger
Logger g_log("FacilityInfo");
}

/** Constructor.
  * @param elem :: The Poco::XML::Element to read the data from
  * @throw std::runtime_error if name or file extensions are not defined
  */
FacilityInfo::FacilityInfo(const Poco::XML::Element *elem)
    : m_catalogs(elem), m_name(elem->getAttribute("name")), m_zeroPadding(0),
      m_delimiter(), m_extensions(), m_archiveSearch(), m_instruments(),
      m_liveListener(), m_computeResources() {
  if (m_name.empty()) {
    g_log.error("Facility name is not defined");
    throw std::runtime_error("Facility name is not defined");
  }

  // Fill the various fields from the XML
  fillZeroPadding(elem);
  fillDelimiter(elem);
  fillExtensions(elem);
  fillArchiveNames(elem);
  fillLiveListener(elem);
  fillComputeResources(elem);
  fillInstruments(elem); // Make sure this is last as it picks up some defaults
                         // that are set above
}

/// Called from constructor to fill zero padding field
void FacilityInfo::fillZeroPadding(const Poco::XML::Element *elem) {
  std::string paddingStr = elem->getAttribute("zeropadding");
  if (paddingStr.empty() ||
      !Mantid::Kernel::Strings::convert(paddingStr, m_zeroPadding)) {
    m_zeroPadding = 0;
  }
}

/// Called from constructor to fill default delimiter
void FacilityInfo::fillDelimiter(const Poco::XML::Element *elem) {
  // The string to separate the instrument name and the run number.
  m_delimiter = elem->getAttribute("delimiter");
}

/// Called from constructor to fill file extensions
void FacilityInfo::fillExtensions(const Poco::XML::Element *elem) {
  std::string extsStr = elem->getAttribute("FileExtensions");
  if (extsStr.empty()) {
    g_log.error("No file extensions defined");
    throw std::runtime_error("No file extensions defined");
  }
  typedef Poco::StringTokenizer tokenizer;
  tokenizer exts(extsStr, ",",
                 tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (tokenizer::Iterator it = exts.begin(); it != exts.end(); ++it) {
    addExtension(*it);
  }
}

/**
  * Add new extension. Adds both a lowercase and uppercase version
  * @param ext :: File extension, including the dot, e.g. ".nxs" or ".raw"
  */
void FacilityInfo::addExtension(const std::string &ext) {
  auto it = std::find(m_extensions.begin(), m_extensions.end(), ext);
  if (it == m_extensions.end())
    m_extensions.push_back(ext);
}

/// Called from constructor to fill archive interface names
void FacilityInfo::fillArchiveNames(const Poco::XML::Element *elem) {
  Poco::AutoPtr<Poco::XML::NodeList> pNL_archives =
      elem->getElementsByTagName("archive");
  if (pNL_archives->length() > 1) {
    g_log.error("Facility must have only one archive tag");
    throw std::runtime_error("Facility must have only one archive tag");
  } else if (pNL_archives->length() == 1) {
    Poco::AutoPtr<Poco::XML::NodeList> pNL_interfaces =
        elem->getElementsByTagName("archiveSearch");
    for (unsigned int i = 0; i < pNL_interfaces->length(); ++i) {
      Poco::XML::Element *elem =
          dynamic_cast<Poco::XML::Element *>(pNL_interfaces->item(i));
      std::string plugin = elem->getAttribute("plugin");
      if (!plugin.empty()) {
        m_archiveSearch.push_back(plugin);
      }
    }
  }
}

/// Called from constructor to fill instrument list
void FacilityInfo::fillInstruments(const Poco::XML::Element *elem) {
  Poco::AutoPtr<Poco::XML::NodeList> pNL_instrument =
      elem->getElementsByTagName("instrument");
  unsigned long n = pNL_instrument->length();
  m_instruments.reserve(n);

  for (unsigned long i = 0; i < n; ++i) {
    Poco::XML::Element *elem =
        dynamic_cast<Poco::XML::Element *>(pNL_instrument->item(i));
    if (elem) {
      try {
        InstrumentInfo instr(this, elem);
        m_instruments.push_back(instr);
      } catch (...) { /*skip this instrument*/
      }
    }
  }

  if (m_instruments.empty()) {
    throw std::runtime_error("Facility " + m_name +
                             " does not have any instruments;");
  }
}

/// Called from constructor to fill live listener name
void FacilityInfo::fillLiveListener(const Poco::XML::Element *elem) {
  // Get the first livedata element (will be NULL if there's none)
  Element *live = elem->getChildElement("livedata");
  if (live) {
    // Get the name of the listener - empty string will be returned if missing
    m_liveListener = live->getAttribute("listener");
  }
}

/// Called from constructor to fill compute resources map
void FacilityInfo::fillComputeResources(const Poco::XML::Element *elem) {
  Poco::AutoPtr<Poco::XML::NodeList> pNL_compute =
      elem->getElementsByTagName("computeResource");
  unsigned long n = pNL_compute->length();
  for (unsigned long i = 0; i < n; i++) {
    Poco::XML::Element *elem =
        dynamic_cast<Poco::XML::Element *>(pNL_compute->item(i));
    std::string name = elem->getAttribute("name");

    m_computeResources.insert(std::make_pair(
        name, boost::shared_ptr<RemoteJobManager>(new RemoteJobManager(elem))));
  }
}

/**
  * Returns instrument with given name
  * @param  iName Instrument name
  * @return the instrument information object
  * @throw NotFoundError if iName was not found
  */
const InstrumentInfo &FacilityInfo::instrument(std::string iName) const {
  if (iName.empty()) {
    iName = ConfigService::Instance().getString("default.instrument");
    g_log.debug() << "Blank instrument specified, using default instrument of "
                  << iName << "." << std::endl;
    if (iName.empty()) {
      return m_instruments.front();
    }
  }

  std::vector<InstrumentInfo>::const_iterator it = m_instruments.begin();
  for (; it != m_instruments.end(); ++it) {
    if (boost::iequals(it->name(), iName)) // Case-insensitive search
    {
      g_log.debug() << "Instrument '" << iName << "' found as " << it->name()
                    << " at " << name() << "." << std::endl;
      return *it;
    }
  }

  // if unsuccessful try shortname
  for (it = m_instruments.begin(); it != m_instruments.end(); ++it) {
    if (boost::iequals(it->shortName(), iName)) // Case-insensitive search
    {
      g_log.debug() << "Instrument '" << iName << "' found as " << it->name()
                    << " at " << name() << "." << std::endl;
      return *it;
    }
  }

  g_log.debug("Instrument " + iName + " not found in facility " + name());
  throw Exception::NotFoundError("FacilityInfo", iName);
}

/**
  * Returns a list of instruments of given technique
  * @param tech :: Technique name
  * @return a list of instrument information objects
  */
std::vector<InstrumentInfo>
FacilityInfo::instruments(const std::string &tech) const {
  std::vector<InstrumentInfo> out;
  std::vector<InstrumentInfo>::const_iterator it = m_instruments.begin();
  for (; it != m_instruments.end(); ++it) {
    if (it->techniques().count(tech)) {
      out.push_back(*it);
    }
  }
  return out;
}

/**
  * Returns a vector of the available compute resources
  * @return vector of strings of the compute resource names
  */
std::vector<std::string> FacilityInfo::computeResources() const {
  std::vector<std::string> names;
  ComputeResourcesMap::const_iterator it = m_computeResources.begin();
  while (it != m_computeResources.end()) {
    names.push_back((*it).first);
    ++it;
  }

  return names;
}

/**
  * Returns a reference to the requested remote job manager
  * @param name :: Name of the cluster we want to submit jobs to
  * @return a shared pointer to the RemoteJobManager instance (or
  * Null if the name wasn't recognized)
  */
boost::shared_ptr<RemoteJobManager>
FacilityInfo::getRemoteJobManager(const std::string &name) const {
  auto it = m_computeResources.find(name);
  if (it == m_computeResources.end()) {
    return boost::shared_ptr<
        RemoteJobManager>(); // TODO: should we throw an exception instead??
  }
  return (*it).second;
}

} // namespace Kernel
} // namespace Mantid
