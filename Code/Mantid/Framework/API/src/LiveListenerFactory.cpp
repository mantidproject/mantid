#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include <Poco/Net/SocketAddress.h>

using boost::shared_ptr;

namespace Mantid {
namespace API {
namespace {
/// static logger
Kernel::Logger g_log("LiveListenerFactory");
}

LiveListenerFactoryImpl::LiveListenerFactoryImpl()
    : Kernel::DynamicFactory<ILiveListener>() {}

LiveListenerFactoryImpl::~LiveListenerFactoryImpl() {}

/** Creates an instance of the appropriate listener for the given instrument,
 * and establishes the
 *  connection to the data acquisition.
 *  @param instrumentName The name of the instrument to 'listen to' (Note that
 * the argument has
 *                        different semantics to the base class create method).
 *  @param connect        Whether to connect the listener to the data stream for
 * the given instrument.
 *  @param properties     Property manager to copy property values to the
 * listener if it has any.
 *  @returns A shared pointer to the created ILiveListener implementation
 *  @throws Exception::NotFoundError If the requested listener is not registered
 *  @throws std::runtime_error If unable to connect to the listener at the
 * configured address
 */
boost::shared_ptr<ILiveListener> LiveListenerFactoryImpl::create(
    const std::string &instrumentName, bool connect,
    const Kernel::IPropertyManager *properties) const {
  ILiveListener_sptr listener;
  // See if we know about the instrument with the given name
  try {
    Kernel::InstrumentInfo inst =
        Kernel::ConfigService::Instance().getInstrument(instrumentName);
    listener =
        Kernel::DynamicFactory<ILiveListener>::create(inst.liveListener());
    // set the properties
    if (properties) {
      listener->updatePropertyValues(*properties);
    }
    if (connect &&
        !listener->connect(Poco::Net::SocketAddress(inst.liveDataAddress()))) {
      // If we can't connect, log and throw an exception
      std::stringstream ss;
      ss << "Unable to connect listener " << listener->name() << " to "
         << inst.liveDataAddress();
      g_log.debug(ss.str());
      throw std::runtime_error(ss.str());
    }
  } catch (Kernel::Exception::NotFoundError &) {
    // If we get to here either we don't know of the instrument name given, or
    // the the live
    // listener class given for the instrument is not known.
    // During development, and for testing, we allow the direct passing in of a
    // listener name
    //   - so try to create that. Will throw if it doesn't exist - let that
    //   exception get out
    listener = Kernel::DynamicFactory<ILiveListener>::create(instrumentName);
    if (connect)
      listener->connect(Poco::Net::SocketAddress()); // Dummy argument for now
  }
  // The Poco SocketAddress can throw all manner of exceptions if the address
  // string it gets is
  // badly formed, or it can't successfully look up the server given, or .......
  // Just catch the base class exception
  catch (Poco::Exception &pocoEx) {
    std::stringstream ss;
    ss << "Unable to connect listener " << listener->name() << " to "
       << instrumentName << ": " << pocoEx.what();
    g_log.debug(ss.str());
    throw std::runtime_error(ss.str());
  }

  // If we get to here, it's all good!
  return listener;
}

/** Tries to connect to the named instrument and returns an indicator of success
 * or failure.
 *  Useful for clients that want to just check whether it's possible to connect
 * to a live stream
 *  but not maintain and use the connection (e.g. for enabling or disabling some
 * GUI button).
 *  @param instrumentName The name of the instrument to connect to
 *  @return True if able to connect, false otherwise
 */
bool LiveListenerFactoryImpl::checkConnection(
    const std::string &instrumentName) const {
  try {
    // Create the live listener (which will try to connect).
    // Don't capture the returned listener - just let it die.
    create(instrumentName, true);
    // If we get to here we have connected successfully.
    return true;
  } catch (std::runtime_error &) {
    // We couldn't connect. Return false.
    return false;
  }
}

/** Override the DynamicFactory::createUnwrapped() method. We don't want it used
 * here.
 *  Making it private will prevent most accidental usage, though of course this
 * could
 *  be called through a DynamicFactory pointer or reference.
 *  @param className Argument that's ignored
 *  @returns Never
 *  @throws Exception::NotImplementedError every time!
 */
ILiveListener *
LiveListenerFactoryImpl::createUnwrapped(const std::string &className) const {
  UNUSED_ARG(className)
  throw Kernel::Exception::NotImplementedError(
      "Don't use this method - use the safe one!!!");
}
} // namespace Mantid
} // namespace API
