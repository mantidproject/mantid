#include "MantidAPI/RemoteJobManagerFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {
namespace {
/// static logger object
Kernel::Logger g_log("RemoteJobManagerFactory");
}

/// Private constructor, singleton class
RemoteJobManagerFactoryImpl::RemoteJobManagerFactoryImpl()
    : Mantid::Kernel::DynamicFactory<IRemoteJobManager>() {
  g_log.debug() << "RemoteJobManager factory created." << std::endl;
}

/**
 * Private destructor, prevent client code from using this.
 */
RemoteJobManagerFactoryImpl::~RemoteJobManagerFactoryImpl() {}

/**
 * Create a remote algorithm with the underlying mechanism that suits
 * the compute resource passed.
 *
 * @param computeResourceName Name of a (remote) compute resource
 *
 * @throw std::invalid_argument If no resource is found by the name
 * given (compute resources are looked up in the facilities definition
 * (XML) file for the current facility.
 */
IRemoteJobManager_sptr RemoteJobManagerFactoryImpl::create(
    const std::string &computeResourceName) const {
  IRemoteJobManager_sptr jm;

  if (computeResourceName.empty())
    return jm;

  Mantid::Kernel::ComputeResourceInfo cr =
      Mantid::Kernel::ConfigService::Instance().getFacility().computeResource(
          computeResourceName);

  // this is the default. It could be "MantidWebServiceAPI", "LSF",
  // "SCARFLSF", "MOAB", etc.
  std::string type = "MantidWebServiceAPIJobManager";
  std::string fdfType = cr.remoteJobManagerType();
  if (!fdfType.empty())
    type = fdfType;
  return create(cr.baseURL(), type);
}

/**
 * Lower level create method that makes a remote algorithm given a
 * base URL and the type of remote job manager.
 *
 * @param baseURL URL where the resource is accessible
 *
 * @param jobManagerType Type/class that can handle this remote
 * compute resource (string names as used in the facilities definition
 * file, for example: MantidWebServiceAPIJobManager).
 *
 * @throw std::invalid_argument If there is an issue with the URL or
 * the type (for example the type is not recognized).
 */
Mantid::API::IRemoteJobManager_sptr
RemoteJobManagerFactoryImpl::create(const std::string baseURL,
                                    const std::string jobManagerType) const {
  Mantid::API::IRemoteJobManager_sptr jm;

  // use the inherited/generic create method
  try {
    jm = Mantid::Kernel::DynamicFactory<IRemoteJobManager>::create(
        jobManagerType);
  } catch (Kernel::Exception::NotFoundError &e) {
    throw Kernel::Exception::NotFoundError(
        "RemoteJobManagerFactory: failed to create a remote job manager of "
        "type (class) '" +
            jobManagerType + "' with base URL " + baseURL +
            ". Error description: ",
        jobManagerType);
  }

  return jm;
}

} // namespace API
} // Namespace Mantid
