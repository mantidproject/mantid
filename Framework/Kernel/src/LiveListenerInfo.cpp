//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/LiveListenerInfo.h"

#include <iosfwd>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"

#include <Poco/DOM/Element.h>


namespace Mantid {
namespace Kernel {
namespace {
// static logger object
Logger g_log("InstrumentInfo");
}

/**
 * Construct from Facility Info XML.
 *
 * @param elem :: The Poco::XML::Element to read the data from
 */
LiveListenerInfo::LiveListenerInfo(const Poco::XML::Element *elem) {
  m_name = elem->getAttribute("name");
  if (m_name.empty()) {
    // throw std::runtime_error("Listener name is not defined");
    g_log.error() << "Listener connection name is not defined. "
                  << "This listener will not be selectable.\n";
  }

  m_address = elem->getAttribute("address");
  if (m_address.empty()) {
    // throw std::runtime_error("Listener address is not defined");
    g_log.error() << "Listener address for " << m_name << " is not defined\n";
  }

  m_listener = elem->getAttribute("listener");
  if (m_listener.empty()) {
    // m_listener = inst->facility().liveListener();
    g_log.error() << "Listener class for " << m_name << " is not defined\n";
  }
}

/**
 * Construct manually.
 *
 * @param listener Class name of specific listener to use
 * @param address Address which listener should use to connect
 * @param name Name designator for this listener connection info
 */
LiveListenerInfo::LiveListenerInfo(const std::string &listener,
                                   const std::string &address,
                                   const std::string &name)
    : m_name(name), m_address(address), m_listener(listener) {}

bool LiveListenerInfo::operator==(const LiveListenerInfo &rhs) const {
  return (this->address() == rhs.address() &&
          this->listener() == rhs.listener());
}

const std::string &LiveListenerInfo::name() const { return m_name; }

const std::string &LiveListenerInfo::address() const { return m_address; }

const std::string &LiveListenerInfo::listener() const { return m_listener; }

//-------------------------------------------------------------------------
// Non-member functions
//-------------------------------------------------------------------------
/**
 * Prints the listener to the stream
 * @param buffer :: A reference to an output stream
 * @param listener :: A reference to a LiveListenerInfo object
 * @return A reference to the stream written to
 */
std::ostream &operator<<(std::ostream &buffer,
                         const LiveListenerInfo &listener) {
  buffer << listener.name() << "(" << listener.address() << ", "
         << listener.listener() << ")";
  return buffer;
}

} // namespace Kernel
} // namespace Mantid
