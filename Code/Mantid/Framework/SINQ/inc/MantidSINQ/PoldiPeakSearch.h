#ifndef MANTID_SINQ_POLDIPEAKSEARCH_H_
#define MANTID_SINQ_POLDIPEAKSEARCH_H_

#include "MantidSINQ/DllConfig.h"

#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V2D.h"
#include "MantidAPI/Algorithm.h"

#include "MantidDataObjects/Workspace2D.h"

#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid {
namespace Poldi {
/** PoldiPeakSearch :

  This Algorithm performs a peak search in POLDI auto-correlation data. It's a
  direct
  translation from the original POLDI analysis software.

  @author Michael Wedel, Paul Scherrer Institut - SINQ
  @date 28/02/2014

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

class MANTID_SINQ_DLL PoldiPeakSearch : public API::Algorithm {
public:
  PoldiPeakSearch();
  virtual ~PoldiPeakSearch() {}

  virtual int version() const { return 1; }
  virtual const std::string name() const { return "PoldiPeakSearch"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "This algorithm finds the peaks in a POLDI auto-correlation "
           "spectrum.";
  }

  virtual const std::string category() const { return "SINQ\\Poldi"; }

protected:
  MantidVec getNeighborSums(MantidVec correlationCounts) const;

  std::list<MantidVec::const_iterator>
  findPeaks(MantidVec::const_iterator begin, MantidVec::const_iterator end);
  std::list<MantidVec::const_iterator>
  findPeaksRecursive(MantidVec::const_iterator begin,
                     MantidVec::const_iterator end) const;

  std::list<MantidVec::const_iterator> mapPeakPositionsToCorrelationData(
      std::list<MantidVec::const_iterator> peakPositions,
      MantidVec::const_iterator baseDataStart,
      MantidVec::const_iterator originalDataStart) const;

  UncertainValue
  getBackgroundWithSigma(std::list<MantidVec::const_iterator> peakPositions,
                         const MantidVec &correlationCounts) const;
  MantidVec getBackground(std::list<MantidVec::const_iterator> peakPositions,
                          const MantidVec &correlationCounts) const;
  bool distanceToPeaksGreaterThanMinimum(
      std::list<MantidVec::const_iterator> peakPositions,
      MantidVec::const_iterator point) const;
  size_t getNumberOfBackgroundPoints(
      std::list<MantidVec::const_iterator> peakPositions,
      const MantidVec &correlationCounts) const;

  double getMedianFromSortedVector(MantidVec::const_iterator begin,
                                   MantidVec::const_iterator end) const;
  double getSn(MantidVec::const_iterator begin,
               MantidVec::const_iterator end) const;

  double
  minimumPeakHeightFromBackground(UncertainValue backgroundWithSigma) const;

  double getTransformedCenter(double value,
                              const Kernel::Unit_sptr &unit) const;
  std::vector<PoldiPeak_sptr>
  getPeaks(const MantidVec::const_iterator &baseListStart,
           const MantidVec::const_iterator &baseListEnd,
           std::list<MantidVec::const_iterator> peakPositions,
           const MantidVec &xData, const Kernel::Unit_sptr &unit) const;

  double getFWHMEstimate(const MantidVec::const_iterator &baseListStart,
                         const MantidVec::const_iterator &baseListEnd,
                         MantidVec::const_iterator peakPosition,
                         const MantidVec &xData) const;

  void setErrorsOnWorkspace(DataObjects::Workspace2D_sptr correlationWorkspace,
                            double error) const;

  void setMinimumDistance(int newMinimumDistance);
  void setMinimumPeakHeight(double newMinimumPeakHeight);
  void setMaximumPeakNumber(int newMaximumPeakNumber);

  static bool vectorElementGreaterThan(MantidVec::const_iterator first,
                                       MantidVec::const_iterator second);
  bool isLessThanMinimum(PoldiPeak_sptr peak);

  int m_minimumDistance;
  int m_doubleMinimumDistance;
  double m_minimumPeakHeight;
  int m_maximumPeakNumber;

  PoldiPeakCollection_sptr m_peaks;

private:
  void init();
  void exec();
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIPEAKSEARCH_H_ */
