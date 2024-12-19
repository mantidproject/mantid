// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid {
namespace Poldi {
/// Helper class for refining peaks with overlapping ranges
class MANTID_SINQ_DLL RefinedRange {
public:
  RefinedRange(const PoldiPeak_sptr &peak, double fwhmMultiples);
  RefinedRange(double xStart, double xEnd, std::vector<PoldiPeak_sptr> peaks);
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

using RefinedRange_sptr = std::shared_ptr<RefinedRange>;

bool MANTID_SINQ_DLL operator<(const RefinedRange_sptr &lhs, const RefinedRange_sptr &rhs);

/** PoldiFitPeaks1D2 :

  PoldiFitPeaks1D2 fits multiple peaks to POLDI auto-correlation data.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 17/03/2014
*/
class MANTID_SINQ_DLL PoldiFitPeaks1D2 : public API::Algorithm {
public:
  PoldiFitPeaks1D2();
  ~PoldiFitPeaks1D2() = default;
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "PoldiPeakFit1D fits peak profiles to POLDI auto-correlation data.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"PoldiFitPeaks2D"}; }
  const std::string category() const override;

protected:
  PoldiPeakCollection_sptr fitPeaks(const PoldiPeakCollection_sptr &peaks);

  int getBestChebyshevPolynomialDegree(const DataObjects::Workspace2D_sptr &dataWorkspace,
                                       const RefinedRange_sptr &range);

  PoldiPeakCollection_sptr getReducedPeakCollection(const PoldiPeakCollection_sptr &peaks) const;

  bool peakIsAcceptable(const PoldiPeak_sptr &peak) const;
  void setPeakFunction(const std::string &peakFunction);

  PoldiPeakCollection_sptr getInitializedPeakCollection(const DataObjects::TableWorkspace_sptr &peakTable) const;

  std::vector<RefinedRange_sptr> getRefinedRanges(const PoldiPeakCollection_sptr &peaks) const;

  std::vector<RefinedRange_sptr> getReducedRanges(const std::vector<RefinedRange_sptr> &ranges) const;

  API::IFunction_sptr getRangeProfile(const RefinedRange_sptr &range, int n) const;

  API::IFunction_sptr getPeakProfile(const PoldiPeak_sptr &poldiPeak) const;

  void setValuesFromProfileFunction(const PoldiPeak_sptr &poldiPeak, const API::IFunction_sptr &fittedFunction) const;

  double getFwhmWidthRelation(const API::IPeakFunction_sptr &peakFunction) const;

  API::IAlgorithm_sptr getFitAlgorithm(const DataObjects::Workspace2D_sptr &dataWorkspace,
                                       const RefinedRange_sptr &range, int n);

  PoldiPeakCollection_sptr m_peaks;
  std::string m_profileTemplate;

  API::WorkspaceGroup_sptr m_fitplots;

  double m_fwhmMultiples;
  double m_maxRelativeFwhm;

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid
