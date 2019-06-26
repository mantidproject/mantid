// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_REMOTEJOBMANAGERFACTORY_H_
#define MANTID_API_REMOTEJOBMANAGERFACTORY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IRemoteJobManager.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
/**
The RemoteJobManagerFactory handles the creation of remote job
managers specialised for different types of compute resources (for
different underlying job schedulers, web services, front-ends,
etc.). Through the create method of this class a shared pointer to a
remote job manager object can be obtained for a particular compute
resource.

The remote job managers built by this factory know how to start and
stop jobs, upload/download files, etc. for the compute resource
specified when creating the job manager (as long as the compute
resource is found for the current facility in the facilities
definition file).

Remote job manager classes must be registered/subscribe using the
macro DECLARE_REMOTEJOBMANAGER (the same way you use DECLARE_ALGORITHM
for algorithms and remote algorithms).

As the algorithm, workspace and other factories in Mantid, this
factory is implemented as a singleton class. Typical usages:

Mantid::API::IRemoteJob|Manager_sptr jobManager =
    Mantid::API::RemoteJobManagerFactory::Instance().create("Fermi");
*/
class MANTID_API_DLL RemoteJobManagerFactoryImpl
    : public Kernel::DynamicFactory<IRemoteJobManager> {
public:
  /// Create a remote job manager that will know how to use the
  /// underlying mechanism that suits the compute resource passed
  IRemoteJobManager_sptr
  create(const std::string &computeResourceName) const override;

  /// alternative (lower level) create where the specific type of
  /// manager and base URL are directly given
  IRemoteJobManager_sptr create(const std::string baseURL,
                                const std::string jobManagerType) const;

private:
  /// So that the singleton can be created (cons/destructor are private)
  friend struct Mantid::Kernel::CreateUsingNew<RemoteJobManagerFactoryImpl>;

  /// Private Constructor for singleton class
  RemoteJobManagerFactoryImpl();
  /// Disallow copy construction
  RemoteJobManagerFactoryImpl(const RemoteJobManagerFactoryImpl &);
  /// Disallow assignment
  RemoteJobManagerFactoryImpl &operator=(const RemoteJobManagerFactoryImpl &);
  ~RemoteJobManagerFactoryImpl() override = default;

  // Unhide the inherited create method but make it private
  using Kernel::DynamicFactory<IRemoteJobManager>::create;
};

// The factory is just a specialisation of SingletonHolder
using RemoteJobManagerFactory =
    Mantid::Kernel::SingletonHolder<RemoteJobManagerFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<Mantid::API::RemoteJobManagerFactoryImpl>;
}
} // namespace Mantid

/* Macro to register (remote job manager) classes into the factory. As
 * with the equivalent macros of the workspace factory or the
 * algorithm factory, this creates a global object in an anonymous
 * namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the
 * factory's subscribe method.
 *
 * You need to use this in every remote job manager. For example:
 * DECLARE_REMOTEJOBMANAGER(MantidWebServiceAPI)
 */
#define DECLARE_REMOTEJOBMANAGER(classname)                                    \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_job_manager_##classname(         \
      ((Mantid::API::RemoteJobManagerFactory::Instance().subscribe<classname>( \
           #classname)),                                                       \
       0));                                                                    \
  }

#endif // MANTID_API_REMOTEJOBMANAGERFACTORY_H_
