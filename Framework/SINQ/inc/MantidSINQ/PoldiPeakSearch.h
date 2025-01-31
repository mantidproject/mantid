// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V2D.h"

#include "MantidDataObjects/Workspace2D.h"

#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"

namespace Mantid {
namespace HistogramData {
class HistogramY;
}
namespace Poldi {
/** PoldiPeakSearch :

  This Algorithm performs a peak search in POLDI auto-correlation data. It's a
  direct
  translation from the original POLDI analysis software.

  @author Michael Wedel, Paul Scherrer Institut - SINQ
  @date 28/02/2014
*/

class MANTID_SINQ_DLL PoldiPeakSearch : public API::Algorithm {
public:
  PoldiPeakSearch();
  virtual ~PoldiPeakSearch() = default;
  int version() const override { return 1; }
  const std::string name() const override { return "PoldiPeakSearch"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm finds the peaks in a POLDI auto-correlation "
           "spectrum.";
  }

  const std::string category() const override { return "SINQ\\Poldi"; }

protected:
  MantidVec getNeighborSums(const HistogramData::HistogramY &correlationCounts) const;

  std::list<MantidVec::const_iterator> findPeaks(MantidVec::const_iterator begin, MantidVec::const_iterator end);
  std::list<MantidVec::const_iterator> findPeaksRecursive(MantidVec::const_iterator begin,
                                                          MantidVec::const_iterator end) const;

  std::list<MantidVec::const_iterator>
  mapPeakPositionsToCorrelationData(const std::list<MantidVec::const_iterator> &peakPositions,
                                    MantidVec::const_iterator baseDataStart,
                                    MantidVec::const_iterator originalDataStart) const;

  UncertainValue getBackgroundWithSigma(const std::list<MantidVec::const_iterator> &peakPositions,
                                        const MantidVec &correlationCounts) const;
  MantidVec getBackground(const std::list<MantidVec::const_iterator> &peakPositions,
                          const MantidVec &correlationCounts) const;
  bool distanceToPeaksGreaterThanMinimum(std::list<MantidVec::const_iterator> peakPositions,
                                         MantidVec::const_iterator point) const;
  size_t getNumberOfBackgroundPoints(const std::list<MantidVec::const_iterator> &peakPositions,
                                     const MantidVec &correlationCounts) const;

  double getMedianFromSortedVector(MantidVec::const_iterator begin, MantidVec::const_iterator end) const;
  double getSn(MantidVec::const_iterator begin, MantidVec::const_iterator end) const;

  double minimumPeakHeightFromBackground(UncertainValue backgroundWithSigma) const;

  double getTransformedCenter(double value, const Kernel::Unit_sptr &unit) const;
  std::vector<PoldiPeak_sptr> getPeaks(const MantidVec::const_iterator &baseListStart,
                                       const MantidVec::const_iterator &baseListEnd,
                                       std::list<MantidVec::const_iterator> peakPositions, const MantidVec &xData,
                                       const Kernel::Unit_sptr &unit) const;

  double getFWHMEstimate(const MantidVec::const_iterator &baseListStart, const MantidVec::const_iterator &baseListEnd,
                         MantidVec::const_iterator peakPosition, const MantidVec &xData) const;

  void setErrorsOnWorkspace(const DataObjects::Workspace2D_sptr &correlationWorkspace, double error) const;

  void setMinimumDistance(int newMinimumDistance);
  void setMinimumPeakHeight(double newMinimumPeakHeight);
  void setMaximumPeakNumber(int newMaximumPeakNumber);

  static bool vectorElementGreaterThan(MantidVec::const_iterator first, MantidVec::const_iterator second);
  bool isLessThanMinimum(const PoldiPeak_sptr &peak);

  int m_minimumDistance;
  int m_doubleMinimumDistance;
  double m_minimumPeakHeight;
  int m_maximumPeakNumber;

  PoldiPeakCollection_sptr m_peaks;

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid
