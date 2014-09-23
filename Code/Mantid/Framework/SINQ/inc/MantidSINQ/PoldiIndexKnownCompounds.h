#ifndef MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDS_H_
#define MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDS_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid
{
namespace Poldi
{

struct PeakCandidate
{
    PeakCandidate() :
        unindexed(),
        candidate(),
        score(0.0),
        candidateCollection(0) { }

    PeakCandidate(const PoldiPeak_sptr &unindexedPeak, const PoldiPeak_sptr &candidatePeak, size_t index) :
        unindexed(unindexedPeak),
        candidate(candidatePeak),
        candidateCollection(index)
    {
        if(!unindexedPeak || !candidatePeak) {
            throw std::invalid_argument("Cannot construct candidate from invalid peaks.");
        }

        double fwhm = candidate->fwhm(PoldiPeak::AbsoluteD);

        if(fwhm <= 0.0) {
            throw std::range_error("FWHM of candidate peak is zero or less - aborting.");
        }

        double peakD = unindexed->d();
        double sigma = fwhm / (2.0 * sqrt(2.0 * log(2.0)));
        double difference = (peakD - candidate->d()) / sigma;
        score = candidate->intensity() / (sigma * sqrt(2.0 * M_PI)) * exp(-0.5*difference*difference);
    }

    bool operator <(const PeakCandidate &other) const
    {
        return score < other.score;
    }

    PoldiPeak_sptr unindexed;
    PoldiPeak_sptr candidate;
    double score;
    size_t candidateCollection;
};

/** PoldiIndexKnownCompounds : TODO: DESCRIPTION

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class MANTID_SINQ_DLL PoldiIndexKnownCompounds  : public API::Algorithm
{
public:
    PoldiIndexKnownCompounds();
    virtual ~PoldiIndexKnownCompounds();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;

    virtual std::map<std::string, std::string> validateInputs();

    void setMeasuredPeaks(const PoldiPeakCollection_sptr &measuredPeaks);
    void setExpectedPhases(const std::vector<PoldiPeakCollection_sptr> &expectedPhases);
    void setExpectedPhaseNames(const std::vector<std::string> &phaseNames);

    void initializeUnindexedPeaks();
    void initializeIndexedPeaks(const std::vector<PoldiPeakCollection_sptr> &expectedPhases);

protected:
    std::vector<API::Workspace_sptr> getWorkspaces(const std::vector<std::string> &workspaceNames) const;
    std::vector<std::string> getWorkspaceNames(const std::vector<API::Workspace_sptr> &workspaces);
    std::vector<PoldiPeakCollection_sptr> getPeakCollections(const std::vector<API::Workspace_sptr> &workspaces) const;

    std::vector<double> getTolerances(size_t size) const;
    std::vector<double> getContributions(size_t size) const;
    std::vector<double> reshapeVector(const std::vector<double> &vector, size_t size) const;

    std::vector<double> getNormalizedContributions(const std::vector<double> &contributions) const;

    void assignIntensityEstimates(const std::vector<PoldiPeakCollection_sptr> &peakCollections, const std::vector<double> &normalizedContributions) const;
    void assignIntensityEstimates(const PoldiPeakCollection_sptr &peakCollection, double contribution) const;

    double getMultiplicity(const PoldiPeakCollection_sptr &peakCollection, const Kernel::V3D &hkl) const;

    void assignFwhmEstimates(const std::vector<PoldiPeakCollection_sptr> &peakCollections, const std::vector<double> &tolerances) const;
    void assignFwhmEstimates(const PoldiPeakCollection_sptr &peakCollection, double tolerance) const;


    void indexPeaks(const PoldiPeakCollection_sptr &unindexed, const std::vector<PoldiPeakCollection_sptr> &knownCompounds);
    void assignCandidates(const std::vector<PeakCandidate> &candidates);

    std::vector<PeakCandidate> getPeakCandidates(const PoldiPeak_sptr &peak, const std::vector<PoldiPeakCollection_sptr> &candidateCollections) const;
    bool isCandidate(const PoldiPeak_sptr &measuredPeak, const PoldiPeak_sptr &possibleCandidate) const;

    std::vector<PeakCandidate> getAllCandidates(const PoldiPeakCollection_sptr &unindexed, const std::vector<PoldiPeakCollection_sptr> &knownCompounds);

    void collectUnindexedPeak(const PoldiPeak_sptr &unindexedPeak);

    bool inPeakSet(const std::set<PoldiPeak_sptr> &peakSet, const PoldiPeak_sptr &peak) const;
    double fwhmToSigma(double fwhm) const;
    double sigmaToFwhm(double sigma) const;

    void assignPeakIndex(const PeakCandidate &candidate);


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

#endif  /* MANTID_SINQ_POLDIINDEXKNOWNCOMPOUNDS_H_ */
