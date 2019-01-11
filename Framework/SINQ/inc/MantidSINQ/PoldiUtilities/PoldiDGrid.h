// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDIDGRID_H
#define MANTID_SINQ_POLDIDGRID_H

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"

#include <vector>

namespace Mantid {
namespace Poldi {

/** PoldiDGrid :
 *
  Helper class for Poldi routines that work with a grid of d-spacings
  that results from detector geometry and wavelength limits.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 24/03/2014
*/

class MANTID_SINQ_DLL PoldiDGrid {
public:
  PoldiDGrid(
      boost::shared_ptr<PoldiAbstractDetector> detector =
          boost::shared_ptr<PoldiAbstractDetector>(),
      boost::shared_ptr<PoldiAbstractChopper> chopper =
          boost::shared_ptr<PoldiAbstractChopper>(),
      double deltaT = 0.0,
      std::pair<double, double> wavelengthRange = std::pair<double, double>());
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
} // namespace Poldi
} // namespace Mantid

#endif // MANTID_SINQ_POLDIDGRID_H
