// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_ALGORITHMFACTORYOBSERVER_H_
#define MANTID_KERNEL_ALGORITHMFACTORYOBSERVER_H_

#include "MantidAPI/AlgorithmFactory.h"
#include <Poco/NObserver.h>

namespace Mantid {
namespace API {

/*
 * To use the AlgorithmFactoryObserver you will need to do a few things:
 *
 * 1. Inherit from this class in the class you wish to take effect on
 *
 * 2. Make sure that the effect you are attempting to observe has been added
 * to the AlgorithmFactory itself by using the public method in this
 * class, e.g. observeSubscribe
 *
 * 3. The last thing to actually have something take effect is by overriding
 * the relevant handle function e.g. when observing subscriptions override
 * subscribeHandle and anything done in that overridden method will happen
 * every time something is subscribed to AlgorithmFactory.
 *
 * This works in both C++ and Python, some functionality is limited in
 * python, but the handlers will all be called.
 */

class MANTID_API_DLL AlgorithmFactoryObserver {
public:
  AlgorithmFactoryObserver();
  virtual ~AlgorithmFactoryObserver();

  void observeUpdate(bool turnOn = true);

  virtual void updateHandle();

private:
  bool m_observingUpdate{false};

  void _updateHandle(AlgorithmFactoryUpdateNotification_ptr pNf);

  /// Poco::NObserver for SubscribeNotification.
  Poco::NObserver<AlgorithmFactoryObserver, AlgorithmFactoryUpdateNotification>
      m_updateObserver;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ALGORITHMFACTORYOBSERVER_H_*/