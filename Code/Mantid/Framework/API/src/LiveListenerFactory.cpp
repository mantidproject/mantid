#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/Logger.h"
#include <Poco/Net/SocketAddress.h>

using boost::shared_ptr;

namespace Mantid
{
namespace API
{

  LiveListenerFactoryImpl::LiveListenerFactoryImpl() : Kernel::DynamicFactory<ILiveListener>(),
      m_log(Kernel::Logger::get("LiveListenerFactory"))
  {
  }

  LiveListenerFactoryImpl::~LiveListenerFactoryImpl()
  {
  }
  
  /** Creates an instance of the appropriate listener for the given instrument, and establishes the
   *  connection to the data acquisition.
   *  @param instrumentName The name of the instrument to 'listen to' (Note that the argument has
   *                        different semantics to the overridden base class create method).
   *  @returns A shared pointer to the created ILiveListener implementation
   *  @throws Exception::NotFoundError if the requested listener is not registered
   */
  boost::shared_ptr<ILiveListener> LiveListenerFactoryImpl::create(const std::string& instrumentName) const
  {
    /* Later, this will look into the facilities.xml file to determine which listener should be created
       for the specified instrument, as well as the connection parameters.
       For now, clients should just supply the class name as 'usual'.
     */
    shared_ptr<ILiveListener> listener = Kernel::DynamicFactory<ILiveListener>::create(instrumentName);
    listener->connect(Poco::Net::SocketAddress());  // Dummy argument for now
    // TODO: Handle connection failure
    return listener;
  }

  /** Override the DynamicFactory::createUnwrapped() method. We don't want it used here.
   *  Making it private will prevent most accidental usage, though of course this could
   *  be called through a DynamicFactory pointer or reference.
   *  @param className Argument that's ignored
   *  @returns Never
   *  @throws Exception::NotImplementedError every time!
   */
  ILiveListener* LiveListenerFactoryImpl::createUnwrapped(const std::string& className) const
  {
    UNUSED_ARG(className)
    throw Kernel::Exception::NotImplementedError("Don't use this method - use the safe one!!!");
  }
} // namespace Mantid
} // namespace API
