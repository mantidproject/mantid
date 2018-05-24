#ifndef MANTID_API_LIVELISTENERFACTORYIMPL_H_
#define MANTID_API_LIVELISTENERFACTORYIMPL_H_

/* Used to register unit classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 *
 * The second operation that this macro performs is to provide the definition
 * of the unitID method for the concrete unit.
 */
#define DECLARE_LISTENER(classname)                                            \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_listener_##classname(            \
      ((Mantid::API::LiveListenerFactory::Instance().subscribe<classname>(     \
           #classname)),                                                       \
       0));                                                                    \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/ILiveListener.h"
#include "MantidKernel/LiveListenerInfo.h"

namespace Mantid {
namespace API {
class IAlgorithm;
/** The factory for creating instances of ILiveListener implementations.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
class MANTID_API_DLL LiveListenerFactoryImpl
    : public Kernel::DynamicFactory<ILiveListener> {
public:
  boost::shared_ptr<ILiveListener>
  create(const std::string &instrumentName, bool connect = false,
         const API::IAlgorithm *callingAlgorithm = nullptr,
         const std::string &listenerConnectionName = "") const;

  boost::shared_ptr<ILiveListener>
  create(const Kernel::LiveListenerInfo &info, bool connect = false,
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
EXTERN_MANTID_API template class MANTID_API_DLL
    Kernel::SingletonHolder<Mantid::API::LiveListenerFactoryImpl>;
}
}

#endif /* MANTID_API_LIVELISTENERFACTORYIMPL_H_ */
