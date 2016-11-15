//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/LiveListenerInfo.h"

#include <iosfwd>

#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"

#include <Poco/DOM/Element.h>

//#include <algorithm>
//
//#include "MantidKernel/Strings.h"
//
//#include <boost/lexical_cast.hpp>
//
//#include <Poco/AutoPtr.h>
//#include <Poco/DOM/NodeList.h>
//#include <Poco/DOM/Text.h>


namespace Mantid {
namespace Kernel {
namespace {
// static logger object
Logger g_log("InstrumentInfo");
}


/**
 * Constructor.
 *
 * @param inst :: InstrumentInfo for the instrument this listener belongs to
 * @param elem :: The Poco::XML::Element to read the data from
 */
LiveListenerInfo::LiveListenerInfo(const InstrumentInfo *inst,
                                   const Poco::XML::Element *elem)
    : m_instrument(inst)
{
  m_name = elem->getAttribute("name");
  if (m_name.empty()) {
    //throw std::runtime_error("Listener name is not defined");
    g_log.warning() << "Listener name is not defined\n";
  }

  m_address = elem->getAttribute("address");
  if (m_address.empty()) {
    //throw std::runtime_error("Listener address is not defined");
    g_log.warning() << "Listener address for " << m_name << " is not defined\n";
  }

  m_listener = elem->getAttribute("listener");
  if (m_listener.empty()) {
    m_listener = inst->facility().liveListener();
  }
}

const std::string &LiveListenerInfo::name() const
{
  return m_name;
}

const std::string &LiveListenerInfo::address() const
{
  return m_address;
}

const std::string &LiveListenerInfo::listener() const
{
  return m_listener;
}

const InstrumentInfo &LiveListenerInfo::instrument() const
{
  return *m_instrument;
}


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
  buffer << listener.name() << "("
         << listener.address() << ", "
         << listener.listener() << ")";
  return buffer;
}

} // namespace Kernel
} // namespace Mantid
