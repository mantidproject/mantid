#ifndef MANTID_SINQ_POLDIDETECTORFACTORY_H_
#define MANTID_SINQ_POLDIDETECTORFACTORY_H_

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "boost/date_time/gregorian/gregorian.hpp"

namespace Mantid {
namespace Poldi {
/** PoldiDetectorFactory :
 *
 *Simple factory

  @author Michael Wedel, Paul Scherrer Institut - SINQ
  @date 07/02/2014

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

class MANTID_SINQ_DLL PoldiDetectorFactory {
public:
  PoldiDetectorFactory();
  virtual ~PoldiDetectorFactory() {}

  virtual PoldiAbstractDetector *createDetector(std::string detectorType);
  virtual PoldiAbstractDetector *
  createDetector(boost::gregorian::date experimentDate);

protected:
  boost::gregorian::date m_newDetectorDate;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIDETECTORFACTORY_H_ */
