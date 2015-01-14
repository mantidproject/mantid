#ifndef MANTID_SINQ_POLDIFITPEAKS1D2_H_
#define MANTID_SINQ_POLDIFITPEAKS1D2_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace Mantid {
namespace Poldi {
/// Helper class for refining peaks with overlapping ranges
class RefinedRange {
public:
  RefinedRange(const PoldiPeak_sptr &peak, double fwhmMultiples);
  RefinedRange(double xStart, double xEnd,
               const std::vector<PoldiPeak_sptr> &peaks);
  RefinedRange(const RefinedRange &other);

  double getWidth() const;

  bool operator<(const RefinedRange &other) const;

  bool overlaps(const RefinedRange &other) const;
  bool overlaps(const RefinedRange &other, double fraction) const;
  bool contains(const RefinedRange &other) const;

  double getOverlapFraction(const RefinedRange &other) const;

  void merge(const RefinedRange &other);

  const std::vector<PoldiPeak_sptr> getPeaks() const { return m_peaks; }

  double getXStart() const { return m_xStart; }
  double getXEnd() const { return m_xEnd; }

private:
  void setRangeBorders(double start, double end);

  std::vector<PoldiPeak_sptr> m_peaks;
  double m_xStart;
  double m_xEnd;
  double m_width;
};

typedef boost::shared_ptr<RefinedRange> RefinedRange_sptr;

bool operator<(const RefinedRange_sptr &lhs, const RefinedRange_sptr &rhs);

/** PoldiFitPeaks1D2 :

  PoldiFitPeaks1D2 fits multiple peaks to POLDI auto-correlation data.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 17/03/2014

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
class MANTID_SINQ_DLL PoldiFitPeaks1D2 : public API::Algorithm {
public:
  PoldiFitPeaks1D2();
  virtual ~PoldiFitPeaks1D2();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "PoldiPeakFit1D fits peak profiles to POLDI auto-correlation data.";
  }

  virtual int version() const;
  virtual const std::string category() const;

protected:
  PoldiPeakCollection_sptr fitPeaks(const PoldiPeakCollection_sptr &peaks);

  int getBestChebyshevPolynomialDegree(
      const DataObjects::Workspace2D_sptr &dataWorkspace,
      const RefinedRange_sptr &range);

  PoldiPeakCollection_sptr
  getReducedPeakCollection(const PoldiPeakCollection_sptr &peaks) const;

  bool peakIsAcceptable(const PoldiPeak_sptr &peak) const;
  void setPeakFunction(const std::string &peakFunction);

  PoldiPeakCollection_sptr getInitializedPeakCollection(
      const DataObjects::TableWorkspace_sptr &peakTable) const;

  std::vector<RefinedRange_sptr>
  getRefinedRanges(const PoldiPeakCollection_sptr &peaks) const;

  std::vector<RefinedRange_sptr>
  getReducedRanges(const std::vector<RefinedRange_sptr> &ranges) const;

  API::IFunction_sptr getRangeProfile(const RefinedRange_sptr &range,
                                      int n) const;

  API::IFunction_sptr getPeakProfile(const PoldiPeak_sptr &poldiPeak) const;

  void
  setValuesFromProfileFunction(PoldiPeak_sptr poldiPeak,
                               const API::IFunction_sptr &fittedFunction) const;

  double getFwhmWidthRelation(API::IPeakFunction_sptr peakFunction) const;

  API::IAlgorithm_sptr
  getFitAlgorithm(const DataObjects::Workspace2D_sptr &dataWorkspace,
                  const RefinedRange_sptr &range, int n);

  PoldiPeakCollection_sptr m_peaks;
  std::string m_profileTemplate;

  API::WorkspaceGroup_sptr m_fitplots;

  double m_fwhmMultiples;

private:
  void init();
  void exec();
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIFITPEAKS1D2_H_ */
