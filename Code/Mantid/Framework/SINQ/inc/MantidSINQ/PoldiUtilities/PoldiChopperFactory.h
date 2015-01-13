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

  Copyright © 2014 PSI-MSS

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_SINQ_DLL PoldiChopperFactory {
public:
  PoldiChopperFactory() {}
  virtual ~PoldiChopperFactory() {}

  virtual PoldiAbstractChopper *createChopper(std::string chopperType);
};

} // namespace SINQ
} // namespace Mantid

#endif /* MANTID_SINQ_POLDICHOPPERFACTORY_H_ */
