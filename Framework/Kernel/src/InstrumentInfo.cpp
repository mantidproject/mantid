// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <algorithm>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

namespace Mantid::Kernel {
namespace {
// static logger object
Logger g_log("InstrumentInfo");
} // namespace

/** Constructor.
 * @param f :: The facility
 * @param elem :: The Poco::XML::Element to read the data from
 * @throw std::runtime_error if name or at least one technique are not defined
 */
InstrumentInfo::InstrumentInfo(const FacilityInfo *f, const Poco::XML::Element *elem)
    : m_facility(f), m_name(elem->getAttribute("name")), m_shortName(elem->getAttribute("shortname")),
      m_delimiter(elem->getAttribute("delimiter")) {

  if (m_delimiter.empty())
    m_delimiter = f->delimiter();
  if (m_shortName.empty())
    m_shortName = m_name;

  fillTechniques(elem);
  fillLiveData(elem);
  fillZeroPadding(elem);
}

/**
 * Equality operator. Two objects are considered equal if their names and short
 * names are the same.
 * @param rhs :: The object to compare this with
 * @returns True if the objects are considered equal, false otherwise
 */
bool InstrumentInfo::operator==(const InstrumentInfo &rhs) const {
  return (this->name() == rhs.name() && this->shortName() == rhs.shortName());
}

/// Returns the default delimiter between instrument name and run number
const std::string &InstrumentInfo::delimiter() const { return m_delimiter; }

/// Return the name of the instrument
const std::string InstrumentInfo::name() const { return m_name; }

/// Return the short name of the instrument
const std::string &InstrumentInfo::shortName() const { return m_shortName; }

/// Returns zero padding for this instrument
int InstrumentInfo::zeroPadding(unsigned int runNumber) const {
  if (m_zeroPadding.empty())
    return m_facility->zeroPadding();
  if (m_zeroPadding.size() == 1) {
    auto padding = m_zeroPadding.begin();
    if (runNumber >= padding->first)
      return getZeroPadding(padding);
    else
      return m_facility->zeroPadding();
  }
  auto last = m_zeroPadding.end();
  --last;
  for (auto it = m_zeroPadding.begin(); it != last; ++it) {
    auto next = it;
    ++next;
    if (runNumber >= it->first && runNumber < next->first) {
      return getZeroPadding(it);
    }
  }
  return getZeroPadding(last);
}

/**
 * Returns file prefix for this instrument and a run number.
 * @param runNumber :: A run number.
 */
std::string InstrumentInfo::filePrefix(unsigned int runNumber) const {
  if (m_zeroPadding.empty())
    return m_shortName;
  if (m_zeroPadding.size() == 1) {
    auto padding = m_zeroPadding.begin();
    if (runNumber >= padding->first)
      return getPrefix(padding);
    else
      return m_shortName;
  }
  auto last = m_zeroPadding.end();
  --last;
  for (auto it = m_zeroPadding.begin(); it != last; ++it) {
    auto next = it;
    ++next;
    if (runNumber >= it->first && runNumber < next->first) {
      return getPrefix(it);
    }
  }
  return getPrefix(last);
}

/// Returns the name of the live listener
std::string InstrumentInfo::liveListener(const std::string &name) const {
  if (!hasLiveListenerInfo())
    return "";

  return liveListenerInfo(name).listener();
}

/** Returns the host & port to connect to for a live data stream
 *  No guarantees are given that the provided string is well-formed and valid
 *    - the caller should check this themselves
 */
std::string InstrumentInfo::liveDataAddress(const std::string &name) const {
  if (!hasLiveListenerInfo())
    return "";

  return liveListenerInfo(name).address();
}

/**
 * Get LiveListenerInfo for specified connection (or default).
 *
 * @param name Name attribute of connection to return info on
 * @return Reference to LiveListenerInfo for specified connection
 * @throw std::runtime_error When no listeners, or name not found
 */
const LiveListenerInfo &InstrumentInfo::liveListenerInfo(std::string name) const {
  if (!hasLiveListenerInfo())
    throw std::runtime_error("Attempted to access live listener for " + m_name +
                             " instrument, which has no listeners.");

  // Default to specified default connection
  if (name.empty())
    name = m_defaultListener;

  // If no default connection specified, fallback to first connection
  if (name.empty())
    return m_listeners.front();

  // Name specified, find requested connection
  auto it = std::find_if(m_listeners.cbegin(), m_listeners.cend(),
                         [&name](const auto &listener) { return boost::iequals(listener.name(), name); });
  if (it != m_listeners.end())
    return *it;

  // The provided name was not valid / did not match any listeners
  throw std::runtime_error("Could not find connection " + name + " for instrument " + m_name);
}

bool InstrumentInfo::hasLiveListenerInfo() const { return !m_listeners.empty(); }

const std::vector<LiveListenerInfo> &InstrumentInfo::liveListenerInfoList() const { return m_listeners; }

/// Return list of techniques
const std::set<std::string> &InstrumentInfo::techniques() const { return m_technique; }

/// Return the facility
const FacilityInfo &InstrumentInfo::facility() const { return *m_facility; }

/// Called from constructor to fill zero padding
void InstrumentInfo::fillZeroPadding(const Poco::XML::Element *elem) {
  Poco::AutoPtr<Poco::XML::NodeList> pNL_zeropadding = elem->getElementsByTagName("zeropadding");
  unsigned long n = pNL_zeropadding->length();

  for (unsigned long i = 0; i < n; ++i) {
    auto elemenent = dynamic_cast<Poco::XML::Element *>(pNL_zeropadding->item(i));
    if (!elemenent)
      continue;
    // read the zero padding size
    if (!elemenent->hasAttribute("size")) {
      throw std::runtime_error("Zeropadding size is missing for instrument " + m_name);
    }
    auto &sizeStr = elemenent->getAttribute("size");
    int size = 0;
    if (!Mantid::Kernel::Strings::convert(sizeStr, size)) {
      throw std::runtime_error("Zeropadding size must be an integer value (instrument " + m_name + ")");
    }
    // read the start run number
    unsigned int startRunNumber = 0;
    if (!elemenent->hasAttribute("startRunNumber")) {
      if (!m_zeroPadding.empty()) {
        throw std::runtime_error("Zeropadding size is missing for instrument " + m_name);
      }
    } else {
      auto &startRunNumberStr = elemenent->getAttribute("startRunNumber");
      try {
        startRunNumber = boost::lexical_cast<unsigned int>(startRunNumberStr);
      } catch (...) {
        throw std::runtime_error("Zeropadding start run number must be an "
                                 "integer value (instrument " +
                                 m_name + ")");
      }
    }
    // read the file prefix
    std::string prefix = m_shortName;
    if (elemenent->hasAttribute("prefix")) {
      prefix = elemenent->getAttribute("prefix");
    }
    m_zeroPadding[startRunNumber] = std::make_pair(prefix, size);
  }

  if (m_zeroPadding.empty()) {
    m_zeroPadding[0] = std::make_pair(m_shortName, m_facility->zeroPadding());
  }
}

/// Called from constructor to fill live listener name
void InstrumentInfo::fillTechniques(const Poco::XML::Element *elem) {
  Poco::AutoPtr<Poco::XML::NodeList> pNL_technique = elem->getElementsByTagName("technique");
  unsigned long n = pNL_technique->length();

  for (unsigned long i = 0; i < n; ++i) {
    Poco::AutoPtr<Poco::XML::NodeList> pNL = pNL_technique->item(i)->childNodes();
    if (pNL->length() > 0) {
      auto *txt = dynamic_cast<Poco::XML::Text *>(pNL->item(0));
      if (txt) {
        const std::string &tech = txt->getData();
        if (!tech.empty()) {
          m_technique.insert(tech);
        }
      }
    }
  }

  if (m_technique.empty()) {
    throw std::runtime_error("No technique is defined for instrument " + m_name);
  }
}

/// Called from constructor to fill live listener name
void InstrumentInfo::fillLiveData(const Poco::XML::Element *elem) {
  // See if we have a <livedata> element (will be NULL if there's none)
  Poco::XML::Element *live = elem->getChildElement("livedata");
  if (!live)
    return;

  // Load default connection name attribute
  m_defaultListener = live->getAttribute("default");

  // Get connections under <livedata>
  Poco::AutoPtr<Poco::XML::NodeList> connections = elem->getElementsByTagName("connection");

  // Load connection info for each child element
  for (unsigned long i = 0; i < connections->length(); ++i) {
    auto *conn = dynamic_cast<Poco::XML::Element *>(connections->item(i));
    try {
      m_listeners.emplace_back(this, conn);
    } catch (...) {
      g_log.error() << "Exception occurred while loading livedata for " << m_name
                    << " instrument. Skipping faulty connection.\n";
    }
  }

  // Get kafka topics under <livedata>
  Poco::AutoPtr<Poco::XML::NodeList> topics = elem->getElementsByTagName("topic");
  for (unsigned long i = 0; i < topics->length(); ++i) {
    auto *topic = dynamic_cast<Poco::XML::Element *>(topics->item(i));
    try {
      m_kafkaTopics.emplace_back(this, topic);
    } catch (...) {
      g_log.error() << "Exception occured while loading livedata for " << m_name
                    << " instrument. Skipping kafka topic.\n";
    }
  }
}

//-------------------------------------------------------------------------
// Non-member functions
//-------------------------------------------------------------------------
/**
 * Prints the instrument name to the stream
 * @param buffer :: A reference to an output stream
 * @param instrumentDescriptor :: A reference to an InstrumentInfo object
 * @return A reference to the stream written to
 */
std::ostream &operator<<(std::ostream &buffer, const InstrumentInfo &instrumentDescriptor) {
  buffer << instrumentDescriptor.name();
  return buffer;
}

} // namespace Mantid::Kernel
