#ifndef MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDS_H_
#define MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDS_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid {
namespace Poldi {

/** IndexCandidatePair :

    IndexCandidatePair is a small helper struct that holds a pointer
    to a measured peak and a pointer to a candidate that may be a
    suitable candidate for indexing. It also calculates a score
    for this pair (see wiki for details).
  */
struct MANTID_SINQ_DLL IndexCandidatePair {
  /// Default constructor
  IndexCandidatePair()
      : observed(), candidate(), positionMatch(0.0),
        candidateCollectionIndex(0) {}

  IndexCandidatePair(const PoldiPeak_sptr &measuredPeak,
                     const PoldiPeak_sptr &candidatePeak, size_t index);

  /// Comparison operator, position matches are compared.
  bool operator<(const IndexCandidatePair &other) const {
    return positionMatch < other.positionMatch;
  }

  PoldiPeak_sptr observed;
  PoldiPeak_sptr candidate;
  double positionMatch;
  size_t candidateCollectionIndex;
};

/** PoldiIndexKnownCompounds :

    Algorithm that assigns Miller indices to measured peaks using
    reflections of known structures present in the sample.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 23/09/2014

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
class MANTID_SINQ_DLL PoldiIndexKnownCompounds : public API::Algorithm {
public:
  PoldiIndexKnownCompounds();
  virtual ~PoldiIndexKnownCompounds();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

  virtual std::map<std::string, std::string> validateInputs();

  void setMeasuredPeaks(const PoldiPeakCollection_sptr &measuredPeaks);
  void setExpectedPhases(
      const std::vector<PoldiPeakCollection_sptr> &expectedPhases);
  void setExpectedPhaseNames(const std::vector<std::string> &phaseNames);

  void initializeUnindexedPeaks();
  void initializeIndexedPeaks(
      const std::vector<PoldiPeakCollection_sptr> &expectedPhases);

  static double fwhmToSigma(double fwhm);
  static double sigmaToFwhm(double sigma);

protected:
  // Workspace and name-handling
  std::vector<API::Workspace_sptr>
  getWorkspaces(const std::vector<std::string> &workspaceNames) const;
  std::vector<PoldiPeakCollection_sptr>
  getPeakCollections(const std::vector<API::Workspace_sptr> &workspaces) const;

  std::vector<std::string>
  getWorkspaceNames(const std::vector<API::Workspace_sptr> &workspaces) const;

  // Input vector checks
  std::vector<double> reshapeVector(const std::vector<double> &vector,
                                    size_t size) const;

  std::vector<double> getContributions(size_t size) const;
  std::vector<double>
  getNormalizedContributions(const std::vector<double> &contributions) const;

  void scaleIntensityEstimates(
      const std::vector<PoldiPeakCollection_sptr> &peakCollections,
      const std::vector<double> &normalizedContributions) const;
  void scaleIntensityEstimates(const PoldiPeakCollection_sptr &peakCollection,
                               double contribution) const;

  void scaleToExperimentalValues(
      const std::vector<PoldiPeakCollection_sptr> &peakCollections,
      const PoldiPeakCollection_sptr &measuredPeaks) const;

  double
  getMaximumIntensity(const PoldiPeakCollection_sptr &peakCollection) const;
  size_t getMaximumIntensityPeakIndex(
      const PoldiPeakCollection_sptr &peakCollection) const;

  std::vector<double> getTolerances(size_t size) const;
  void assignFwhmEstimates(
      const std::vector<PoldiPeakCollection_sptr> &peakCollections,
      const std::vector<double> &tolerances) const;
  void assignFwhmEstimates(const PoldiPeakCollection_sptr &peakCollection,
                           double tolerance) const;

  // Indexing algorithm
  void
  indexPeaks(const PoldiPeakCollection_sptr &measured,
             const std::vector<PoldiPeakCollection_sptr> &knownCompoundPeaks);

  std::vector<IndexCandidatePair> getAllIndexCandidatePairs(
      const PoldiPeakCollection_sptr &measured,
      const std::vector<PoldiPeakCollection_sptr> &knownCompoundPeaks);
  std::vector<IndexCandidatePair> getIndexCandidatePairs(
      const PoldiPeak_sptr &peak,
      const std::vector<PoldiPeakCollection_sptr> &candidateCollections) const;
  bool isCandidate(const PoldiPeak_sptr &measuredPeak,
                   const PoldiPeak_sptr &possibleCandidate) const;
  void collectUnindexedPeak(const PoldiPeak_sptr &unindexedPeak);

  void assignCandidates(const std::vector<IndexCandidatePair> &candidates);
  bool inPeakSet(const std::set<PoldiPeak_sptr> &peakSet,
                 const PoldiPeak_sptr &peak) const;
  void assignPeakIndex(const IndexCandidatePair &candidate);

  // Finalization
  PoldiPeakCollection_sptr
  getIntensitySortedPeakCollection(const PoldiPeakCollection_sptr &peaks) const;

  void assignCrystalStructureParameters(
      PoldiPeakCollection_sptr &indexedPeaks,
      const PoldiPeakCollection_sptr &phasePeaks) const;

  PoldiPeakCollection_sptr m_measuredPeaks;
  std::vector<PoldiPeakCollection_sptr> m_expectedPhases;
  std::vector<std::string> m_phaseNames;

  PoldiPeakCollection_sptr m_unindexedPeaks;
  std::vector<PoldiPeakCollection_sptr> m_indexedPeaks;

private:
  void init();
  void exec();
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDS_H_ */
