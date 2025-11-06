// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/LiveListenerInfo.h"

#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"

#include <Poco/DOM/Element.h>

#include <ostream>
#include <utility>

namespace Mantid::Kernel {
namespace {
// static logger object
Logger g_log("InstrumentInfo");
} // namespace

/**
 * Construct from Facility Info XML.
 *
 * @param inst Pointer to InstrumentInfo that this LiveListenerInfo belongs to
 * @param elem The Poco::XML::Element to read the data from
 */
LiveListenerInfo::LiveListenerInfo(InstrumentInfo const *inst, Poco::XML::Element const *elem)
    : m_name(elem->getAttribute("name")), m_address(elem->getAttribute("address")),
      m_listener(elem->getAttribute("listener")) {

  if (m_name.empty())
    g_log.error() << "Listener connection name for " << inst->name()
                  << "is not defined. This listener will not be selectable.\n";
  if (m_address.empty())
    g_log.error() << "Listener address for " << inst->name() << " is not defined.\n";
  if (m_listener.empty())
    g_log.error() << "Listener class for " << inst->name() << " is not defined.\n";
}

/**
 * Construct manually.
 *
 * @param listener Class name of specific listener to use
 * @param address Address which listener should use to connect
 * @param name Name designator for this listener connection info
 */
LiveListenerInfo::LiveListenerInfo(std::string listener, std::string address, std::string name)
    : m_name(std::move(name)), m_address(std::move(address)), m_listener(std::move(listener)) {}

bool LiveListenerInfo::operator==(const LiveListenerInfo &rhs) const {
  return (this->address() == rhs.address() && this->listener() == rhs.listener());
}

const std::string &LiveListenerInfo::name() const { return m_name; }

const std::string &LiveListenerInfo::address() const { return m_address; }

const std::string &LiveListenerInfo::listener() const { return m_listener; }

//-------------------------------------------------------------------------
// Non-member functions
//-------------------------------------------------------------------------
/**
 * Prints the listener to the stream.
 *
 * @param buffer :: A reference to an output stream
 * @param listener :: A reference to a LiveListenerInfo object
 * @return A reference to the stream written to
 */
std::ostream &operator<<(std::ostream &buffer, const LiveListenerInfo &listener) {
  buffer << listener.name() << "(" << listener.address() << ", " << listener.listener() << ")";
  return buffer;
}

} // namespace Mantid::Kernel
