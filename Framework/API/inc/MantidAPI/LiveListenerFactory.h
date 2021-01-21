// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/* Used to register unit classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 *
 * The second operation that this macro performs is to provide the definition
 * of the unitID method for the concrete unit.
 */
#define DECLARE_LISTENER(classname)                                                                                    \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_listener_##classname(                                                    \
      ((Mantid::API::LiveListenerFactory::Instance().subscribe<classname>(#classname)), 0));                           \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ILiveListener.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/LiveListenerInfo.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
class IAlgorithm;
/** The factory for creating instances of ILiveListener implementations.
 */
class MANTID_API_DLL LiveListenerFactoryImpl : public Kernel::DynamicFactory<ILiveListener> {
public:
  std::shared_ptr<ILiveListener> create(const std::string &instrumentName, bool connect = false,
                                        const API::IAlgorithm *callingAlgorithm = nullptr,
                                        const std::string &listenerConnectionName = "") const;

  std::shared_ptr<ILiveListener> create(const Kernel::LiveListenerInfo &info, bool connect = false,
                                        const API::IAlgorithm *callingAlgorithm = nullptr) const;

  LiveListenerFactoryImpl(const LiveListenerFactoryImpl &) = delete;
  LiveListenerFactoryImpl &operator=(const LiveListenerFactoryImpl &) = delete;

private:
  friend struct Kernel::CreateUsingNew<LiveListenerFactoryImpl>;

  /// Private constructor for singleton class
  LiveListenerFactoryImpl() = default;

  /// Private destructor
  ~LiveListenerFactoryImpl() override = default;

  // Unhide the base class method to avoid a warning, but make private.
  using Kernel::DynamicFactory<ILiveListener>::create;
  /// Override the DynamicFactory::createUnwrapped() method. We don't want it
  /// used here.
  ILiveListener *createUnwrapped(const std::string &className) const override;
};

using LiveListenerFactory = Kernel::SingletonHolder<LiveListenerFactoryImpl>;

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Kernel::SingletonHolder<Mantid::API::LiveListenerFactoryImpl>;
}
} // namespace Mantid
