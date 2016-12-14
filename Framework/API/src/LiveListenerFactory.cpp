#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include <Poco/Net/SocketAddress.h>

using boost::shared_ptr;

namespace Mantid {
namespace API {
namespace {
/// static logger
Kernel::Logger g_log("LiveListenerFactory");
}

/**
 * Creates an instance of the appropriate listener for the given instrument,
 * and establishes the connection to the data acquisition.
 *
 * @param instrumentName The name of the instrument to 'listen to' (Note that
 *                       the argument has different semantics to the base class
 *                       create method).
 * @param connect        Whether to connect the listener to the data stream for
 *                       the given instrument.
 * @param properties     Property manager to copy property values to the
 *                       listener if it has any.
 * @param listenerConnectionName Name of LiveListenerInfo connection to use.
 * @return A shared pointer to the created ILiveListener implementation
 * @throws Exception::NotFoundError If the requested listener is not registered
 * @throws std::runtime_error If unable to connect to the listener at the
 *                            configured address.
 */
boost::shared_ptr<ILiveListener> LiveListenerFactoryImpl::create(
    const std::string &instrumentName, bool connect,
    const Kernel::IPropertyManager *properties,
    const std::string &listenerConnectionName) const {
  try {
    // Look up LiveListenerInfo based on given instrument and listener names
    auto inst = Kernel::ConfigService::Instance().getInstrument(instrumentName);
    auto info = inst.liveListenerInfo(listenerConnectionName);

    // Defer creation logic to other create overload
    return create(info, connect, properties);

  } catch (Kernel::Exception::NotFoundError &) {
    // Could not determine LiveListenerInfo for instrumentName
    // Attempt to interpret instrumentName as listener class name instead, to
    // support legacy usage in unit tests.
    Kernel::LiveListenerInfo info(instrumentName);
    return create(info, connect, properties);
  }
}

/**
 * Creates an instance of a specific LiveListener based on the given
 * LiveListenerInfo instance. The only required data from LiveListenerInfo is
 * the listener class name. If connect is set to true, a valid address is also
 * required.
 *
 * @param info       LiveListenerInfo based on which to create the LiveListener
 * @param connect    Whether to connect the listener to the data stream for the
 *                   given instrument.
 * @param properties Property manager to copy property values to the listener
 *                   if it has any.
 * @return A shared pointer to the created ILiveListener implementation
 */
boost::shared_ptr<ILiveListener> LiveListenerFactoryImpl::create(
    const Kernel::LiveListenerInfo &info, bool connect,
    const Kernel::IPropertyManager *properties) const {

  ILiveListener_sptr listener =
      Kernel::DynamicFactory<ILiveListener>::create(info.listener());

  // Give LiveListener additional properties if provided
  if (properties) {
    listener->updatePropertyValues(*properties);
  }

  if (connect) {
    try {
      Poco::Net::SocketAddress address;

      // To allow listener::connect to be called even when no address is given
      if (!info.address().empty())
        address = Poco::Net::SocketAddress(info.address());

      // If we can't connect, throw an exception to be handled below
      if (!listener->connect(address)) {
        throw Poco::Exception("Connection attempt failed.");
      }
    }
    // The Poco SocketAddress can throw all manner of exceptions if the address
    // string it gets is badly formed, or it can't successfully look up the
    // server given, or .......
    // Just catch the base class exception
    catch (Poco::Exception &pocoEx) {
      std::stringstream ss;
      ss << "Unable to connect listener [" << info.listener() << "] to ["
         << info.address() << "]: " << pocoEx.what();
      g_log.debug(ss.str());
      throw std::runtime_error(ss.str());
    }
  }

  // If we get to here, it's all good!
  return listener;
}

/**
 * Override the DynamicFactory::createUnwrapped() method. We don't want it used
 * here. Making it private will prevent most accidental usage, though of course
 * this could be called through a DynamicFactory pointer or reference.
 *
 * @param className Argument that's ignored
 * @returns Never
 * @throws Exception::NotImplementedError every time!
 */
ILiveListener *
LiveListenerFactoryImpl::createUnwrapped(const std::string &className) const {
  UNUSED_ARG(className)
  throw Kernel::Exception::NotImplementedError(
      "Don't use this method - use the safe one!!!");
}

} // namespace Mantid
} // namespace API
