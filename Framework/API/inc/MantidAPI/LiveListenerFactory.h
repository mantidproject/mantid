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

namespace Mantid {
namespace API {
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
  create(const std::string &instrumentName, bool connect,
         const Kernel::IPropertyManager *props = NULL) const;
  bool checkConnection(const std::string &instrumentName) const;

private:
  friend struct Kernel::CreateUsingNew<LiveListenerFactoryImpl>;

  /// Private constructor for singleton class
  LiveListenerFactoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  LiveListenerFactoryImpl(const LiveListenerFactoryImpl &);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  LiveListenerFactoryImpl &operator=(const LiveListenerFactoryImpl &);
  /// Private destructor
  virtual ~LiveListenerFactoryImpl();

  // Unhide the base class method to avoid a warning, but make private.
  using Kernel::DynamicFactory<ILiveListener>::create;
  /// Override the DynamicFactory::createUnwrapped() method. We don't want it
  /// used here.
  ILiveListener *createUnwrapped(const std::string &className) const;
};

/// Forward declaration of a specialisation of SingletonHolder (needed for
/// dllexport/dllimport).
#ifdef _WIN32
// this breaks new namespace declaration rules; need to find a better fix
template class MANTID_API_DLL Kernel::SingletonHolder<LiveListenerFactoryImpl>;
#endif /* _WIN32 */
/// The specialisation of the SingletonHolder class that holds the
/// LiveListenerFactory
typedef Kernel::SingletonHolder<LiveListenerFactoryImpl> LiveListenerFactory;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_LIVELISTENERFACTORYIMPL_H_ */
