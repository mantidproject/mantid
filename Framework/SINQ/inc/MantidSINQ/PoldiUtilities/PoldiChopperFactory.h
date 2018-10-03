// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDICHOPPERFACTORY_H_
#define MANTID_SINQ_POLDICHOPPERFACTORY_H_

#include "MantidKernel/System.h"

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"

namespace Mantid {
namespace Poldi {

/** PoldiChopperFactory :
 *
  Factory for chopper objects for use with POLDI algorithms.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 10/02/2014
*/
class MANTID_SINQ_DLL PoldiChopperFactory {
public:
  virtual ~PoldiChopperFactory() = default;

  virtual PoldiAbstractChopper *createChopper(std::string chopperType);
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDICHOPPERFACTORY_H_ */
