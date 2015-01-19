#ifndef MANTID_SINQ_POLDIDGRID_H
#define MANTID_SINQ_POLDIDGRID_H

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"

#include <vector>

namespace Mantid {
namespace Poldi {

/** PoldiDGrid :
 *
  Helper class for Poldi routines that work with a grid of d-spacings
  that results from detector geometry and wavelength limits.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 24/03/2014

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

class MANTID_SINQ_DLL PoldiDGrid {
public:
  PoldiDGrid(boost::shared_ptr<PoldiAbstractDetector> detector =
                 boost::shared_ptr<PoldiAbstractDetector>(),
             boost::shared_ptr<PoldiAbstractChopper> chopper =
                 boost::shared_ptr<PoldiAbstractChopper>(),
             double deltaT = 0.0, std::pair<double, double> wavelengthRange =
                                      std::pair<double, double>());
  ~PoldiDGrid() {}

  void setDetector(boost::shared_ptr<PoldiAbstractDetector> newDetector);
  void setChopper(boost::shared_ptr<PoldiAbstractChopper> newChopper);
  void setDeltaT(double newDeltaT);
  void setWavelengthRange(std::pair<double, double> wavelengthRange);

  double deltaD();
  std::vector<double> grid();

protected:
  std::pair<int, int> calculateDRange();
  double calculateDeltaD();
  void createGrid();

  boost::shared_ptr<PoldiAbstractDetector> m_detector;
  boost::shared_ptr<PoldiAbstractChopper> m_chopper;
  double m_deltaT;
  std::pair<double, double> m_wavelengthRange;

  std::pair<int, int> m_dRangeAsMultiples;
  double m_deltaD;
  std::vector<double> m_dgrid;

  bool m_hasCachedCalculation;
};
}
}

#endif // MANTID_SINQ_POLDIDGRID_H
