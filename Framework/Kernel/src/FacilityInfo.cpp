// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <algorithm>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

#include <MantidKernel/StringTokenizer.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

using Poco::XML::Element;

namespace Mantid {
namespace Kernel {

namespace {
/// static logger
Logger g_log("FacilityInfo");
} // namespace

/** Constructor.
 * @param elem :: The Poco::XML::Element to read the data from
 * @throw std::runtime_error if name or file extensions are not defined
 */
FacilityInfo::FacilityInfo(const Poco::XML::Element *elem)
    : m_catalogs(elem), m_name(elem->getAttribute("name")), m_timezone(),
      m_zeroPadding(0), m_delimiter(), m_extensions(), m_archiveSearch(),
      m_instruments(), m_noFilePrefix(), m_multiFileLimit(100),
      m_computeResources() {
  if (m_name.empty()) {
    g_log.error("Facility name is not defined");
    throw std::runtime_error("Facility name is not defined");
  }

  // Fill the various fields from the XML
  fillZeroPadding(elem);
  fillDelimiter(elem);
  fillExtensions(elem);
  fillArchiveNames(elem);
  fillTimezone(elem);
  fillComputeResources(elem);
  fillNoFilePrefix(elem);
  fillMultiFileLimit(elem);
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

/// Called from constructor to fill the noFilePrefix flag
void FacilityInfo::fillNoFilePrefix(const Poco::XML::Element *elem) {
  std::string noFilePrefixStr = elem->getAttribute("nofileprefix");
  m_noFilePrefix = (noFilePrefixStr == "True");
}

/// Called from constructor to fill the multifile limit
void FacilityInfo::fillMultiFileLimit(const Poco::XML::Element *elem) {
  const std::string multiFileLimitStr = elem->getAttribute("multifilelimit");
  if (!multiFileLimitStr.empty()) {
    size_t limit;
    if (Mantid::Kernel::Strings::convert(multiFileLimitStr, limit)) {
      m_multiFileLimit = limit;
    }
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
  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer exts(extsStr, ",",
                 tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (const auto &ext : exts) {
    addExtension(ext);
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

void FacilityInfo::fillTimezone(const Poco::XML::Element *elem) {
  Poco::AutoPtr<Poco::XML::NodeList> pNL_timezones =
      elem->getElementsByTagName("timezone");
  if (pNL_timezones->length() == 0) {
    g_log.notice() << "No timezone specified for " << m_name
                   << " in Facilities.xml\n";
  } else if (pNL_timezones->length() == 1) {
    pNL_timezones = pNL_timezones->item(0)->childNodes();
    if (pNL_timezones->length() > 0) {
      Poco::XML::Text *txt =
          dynamic_cast<Poco::XML::Text *>(pNL_timezones->item(0));
      m_timezone = txt->getData();
    }
  } else {
    throw std::runtime_error("Facility " + m_name +
                             " has more than one timezone specified");
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

/// Called from constructor to fill compute resources map
void FacilityInfo::fillComputeResources(const Poco::XML::Element *elem) {
  Poco::AutoPtr<Poco::XML::NodeList> pNL_compute =
      elem->getElementsByTagName("computeResource");
  unsigned long n = pNL_compute->length();
  for (unsigned long i = 0; i < n; i++) {
    Poco::XML::Element *elem =
        dynamic_cast<Poco::XML::Element *>(pNL_compute->item(i));

    if (elem) {
      try {
        ComputeResourceInfo cr(this, elem);
        m_computeResInfos.push_back(cr);

        g_log.debug() << "Compute resource found: " << cr << '\n';
      } catch (...) { // next resource...
      }

      std::string name = elem->getAttribute("name");
      // TODO: this is a bit of duplicate effort at the moment, until
      // RemoteJobManager goes away from here (then this would be
      // removed), see header for details.
      m_computeResources.emplace(name,
                                 boost::make_shared<RemoteJobManager>(elem));
    }
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
                  << iName << ".\n";
    if (iName.empty()) {
      return m_instruments.front();
    }
  }

  auto instrument = std::find_if(m_instruments.cbegin(), m_instruments.cend(),
                                 [&iName](const auto &inst) {
                                   return boost::iequals(inst.name(), iName);
                                 });

  // if unsuccessful try lookup by short name
  if (instrument == m_instruments.cend()) {
    instrument = std::find_if(m_instruments.cbegin(), m_instruments.cend(),
                              [&iName](const auto &inst) {
                                return boost::iequals(inst.shortName(), iName);
                              });
  }

  if (instrument != m_instruments.cend()) {
    g_log.debug() << "Instrument '" << iName << "' found as "
                  << instrument->name() << " at " << name() << ".\n";
    return *instrument;
  }

  g_log.debug("Instrument " + iName + " not found in facility " + name());
  throw Exception::NotFoundError("FacilityInfo", iName);
}

/**
 * Get the vector of available compute resources
 * @return vector of ComputeResourInfo for the current facility
 */
std::vector<ComputeResourceInfo> FacilityInfo::computeResInfos() const {
  return m_computeResInfos;
}

/**
 * Returns a list of instruments of given technique
 * @param tech :: Technique name
 * @return a list of instrument information objects
 */
std::vector<InstrumentInfo>
FacilityInfo::instruments(const std::string &tech) const {
  std::vector<InstrumentInfo> out;
  auto it = m_instruments.begin();
  for (; it != m_instruments.end(); ++it) {
    if (it->techniques().count(tech)) {
      out.push_back(*it);
    }
  }
  return out;
}

/**
 * Returns a vector of the names of the available compute resources
 * @return vector of strings of the compute resource names
 */
std::vector<std::string> FacilityInfo::computeResources() const {
  std::vector<std::string> names;
  auto it = m_computeResources.begin();
  while (it != m_computeResources.end()) {
    names.push_back((*it).first);
    ++it;
  }

  return names;
}

/**
 * Get a compute resource by name
 *
 * @param name Name as specified in the facilities definition file
 *
 * @return the named compute resource
 *
 * @throws NotFoundError if the resource is not found/available.
 */
const ComputeResourceInfo &
FacilityInfo::computeResource(const std::string &name) const {
  if (name.empty()) {
    g_log.debug("Cannot find a compute resource without name "
                "(empty).");
    throw Exception::NotFoundError("FacilityInfo, empty compute resource name",
                                   name);
  }

  auto it = m_computeResInfos.begin();
  for (; it != m_computeResInfos.end(); ++it) {
    if (it->name() == name) {
      g_log.debug() << "Compute resource '" << name << "' found at facility "
                    << this->name() << ".\n";
      return *it;
    }
  }

  g_log.debug() << "Could not find requested compute resource: " << name
                << " in facility " << this->name() << ".\n";
  throw Exception::NotFoundError(
      "FacilityInfo, missing compute resource, it does not seem to be defined "
      "in the facility '" +
          this->name() + "' - ",
      name);
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
    return boost::shared_ptr<RemoteJobManager>(); // TODO: should we throw an
                                                  // exception instead??
  }
  return (*it).second;
}

} // namespace Kernel
} // namespace Mantid
