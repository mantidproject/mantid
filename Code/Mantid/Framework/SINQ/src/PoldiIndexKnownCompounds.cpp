#include "MantidSINQ/PoldiIndexKnownCompounds.h"
#include "MantidSINQ/PoldiUtilities/PeakFunctionIntegrator.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidSINQ/PoldiUtilities/MillerIndicesIO.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FunctionFactory.h"

#include <boost/bind.hpp>
#include <numeric>

#include <boost/math/distributions/normal.hpp>

namespace Mantid
{
namespace Poldi
{

using Mantid::Kernel::Direction;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiIndexKnownCompounds)

/// Constructor of IndexCandidatePair, calculates score for given peak-pair.
IndexCandidatePair::IndexCandidatePair(const PoldiPeak_sptr &measuredPeak, const PoldiPeak_sptr &candidatePeak, size_t index) :
    observed(measuredPeak),
    candidate(candidatePeak),
    candidateCollectionIndex(index)
{
    if(!measuredPeak || !candidatePeak) {
        throw std::invalid_argument("Cannot construct candidate from invalid peaks.");
    }

    double fwhm = candidate->fwhm(PoldiPeak::AbsoluteD);

    if(fwhm <= 0.0) {
        throw std::range_error("FWHM of candidate peak is zero or less - aborting.");
    }

    double peakD = observed->d();
    double sigmaD = PoldiIndexKnownCompounds::fwhmToSigma(fwhm);
    double differenceD = fabs(peakD - candidate->d());

    /* Assume a normal distribution of distances around 0.0, with the given
     * sigma (corresponds to tolerance). Then, calculate the probability
     * of finding a peak that is even closer. The complement of that is the
     * "match". The closer the measured position is to the calculated, the
     * lower the probability of finding a peak that is even closer, thus
     * increasing the peak's "match".
     */
    boost::math::normal positionDistribution(0.0, sigmaD);

    // Multiplication by 2.0, because difference is absolute.
    double complementOfCloserPeak = boost::math::cdf(complement(positionDistribution, differenceD)) * 2.0;

    // Scale by calculated intensity to take into account minority phases etc.
    double candidateIntensity = candidatePeak->intensity();
    positionMatch = complementOfCloserPeak * candidateIntensity;
}

/// Default constructor
PoldiIndexKnownCompounds::PoldiIndexKnownCompounds()
{
}

/// Destructor
PoldiIndexKnownCompounds::~PoldiIndexKnownCompounds()
{
}

/// Returns the algorithm name
const std::string PoldiIndexKnownCompounds::name() const
{
    return "PoldiIndexKnownCompounds";
}

/// Algorithm's version for identification. @see Algorithm::version
int PoldiIndexKnownCompounds::version() const { return 1;}

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiIndexKnownCompounds::category() const { return "SINQ\\Poldi";}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiIndexKnownCompounds::summary() const { return "Index POLDI peaks using known compounds.";}

/** Validates user input once all values have been set.
 *
 *  This method checks that the vectors supplied in Tolerances and ScatteringContributions
 *  have the same length as the number of workspaces supplied to CompoundWorkspaces or 1.
 *
 *  @return map that contains problem descriptions.
 */
std::map<std::string, std::string> PoldiIndexKnownCompounds::validateInputs()
{
    std::map<std::string, std::string> errorMap;

    const std::vector<Workspace_sptr> compoundWorkspaces = getWorkspaces(getProperty("CompoundWorkspaces"));

    const std::vector<double> tolerances = getProperty("Tolerances");
    if(tolerances.size() > 1 && tolerances.size() != compoundWorkspaces.size()) {
        errorMap["Tolerance"] = std::string("Number of Tolerances must be either 1 or equal to the number of CompoundWorkspaces.");
    }

    const std::vector<double> contributions = getProperty("ScatteringContributions");
    if(contributions.size() > 1 && contributions.size() != compoundWorkspaces.size()) {
        errorMap["ScatteringContributions"] = std::string("Number of ScatteringContributions must be either 1 or equal to the number of CompoundWorkspaces.");
    }

    return errorMap;
}

/// Sets measured peaks that will be used for indexing.
void PoldiIndexKnownCompounds::setMeasuredPeaks(const PoldiPeakCollection_sptr &measuredPeaks)
{
    m_measuredPeaks = measuredPeaks;
}

/// This method sets the peaks of all expected phases.
void PoldiIndexKnownCompounds::setExpectedPhases(const std::vector<PoldiPeakCollection_sptr> &expectedPhases)
{
    m_expectedPhases = expectedPhases;
}

/// In order to name output workspaces correctly, their names have to be stored.
void PoldiIndexKnownCompounds::setExpectedPhaseNames(const std::vector<std::string> &phaseNames)
{
    m_phaseNames = phaseNames;
}

/// Creates a new collection that will store peaks that cannot be indexed.
void PoldiIndexKnownCompounds::initializeUnindexedPeaks()
{
    m_unindexedPeaks = boost::make_shared<PoldiPeakCollection>();
}

/** Initializes peak collections for storing assigned peaks.
 *
 *  This method creates as many empty PoldiPeakCollections as expected phases are given as a parameter to this method.
 *  If a point group is stored in the workspace, it is transfered to the new workspace.
 *
 *  @param expectedPhases :: Vector of peak collections for expected phases.
 */
void PoldiIndexKnownCompounds::initializeIndexedPeaks(const std::vector<PoldiPeakCollection_sptr> &expectedPhases)
{
    if(!m_measuredPeaks) {
        throw std::runtime_error("Measured peaks need to be set first.");
    }

    m_indexedPeaks.clear();

    for(size_t i = 0; i < expectedPhases.size(); ++i) {
        PoldiPeakCollection_sptr newCollection = boost::make_shared<PoldiPeakCollection>(m_measuredPeaks->intensityType());
        newCollection->setPointGroup(expectedPhases[i]->pointGroup());
        newCollection->setProfileFunctionName(m_measuredPeaks->getProfileFunctionName());

        m_indexedPeaks.push_back(newCollection);
    }
}

/// Converts FWHM to sigma of a Gaussian
double PoldiIndexKnownCompounds::fwhmToSigma(double fwhm)
{
    return fwhm / (2.0 * sqrt(2.0 * log(2.0)));
}

/// Converts sigma of a Gaussian to FWHM
double PoldiIndexKnownCompounds::sigmaToFwhm(double sigma)
{
    return sigma * (2.0 * sqrt(2.0 * log(2.0)));
}

/** Recursively creates a list of Workspaces from a list of workspace names.
 *
 *  In order to be flexible and allow organization of input workspaces in nested hierarchies,
 *  this method recursively retrieves workspaces from groups. All workspaces are stored
 *  in one flat vector, which is returned at the end.
 *
 *  @param workspaceNames :: Vector with workspace names.
 *  @return Vector of workspaces.
 */
std::vector<Workspace_sptr> PoldiIndexKnownCompounds::getWorkspaces(const std::vector<std::string> &workspaceNames) const
{
    std::vector<Workspace_sptr> workspaces;

    for(auto it = workspaceNames.begin(); it != workspaceNames.end(); ++it) {
        Workspace_sptr currentWorkspace = AnalysisDataService::Instance().retrieveWS<Workspace>(*it);

        WorkspaceGroup_sptr groupTest = boost::dynamic_pointer_cast<WorkspaceGroup>(currentWorkspace);
        if(groupTest) {
            std::vector<Workspace_sptr> workspacesNextLevel = getWorkspaces(groupTest->getNames());

            workspaces.insert(workspaces.end(), workspacesNextLevel.begin(), workspacesNextLevel.end());
        } else {
            workspaces.insert(workspaces.end(), currentWorkspace);
        }
    }

    return workspaces;
}

/// Creates a PoldiPeakCollection from each workspace of the input vector. Throws std::invalid_argument if invalid workspaces are present.
std::vector<PoldiPeakCollection_sptr> PoldiIndexKnownCompounds::getPeakCollections(const std::vector<Workspace_sptr> &workspaces) const
{
    std::vector<PoldiPeakCollection_sptr> peakCollections;
    for(auto it = workspaces.begin(); it != workspaces.end(); ++it) {
        TableWorkspace_sptr tableWs = boost::dynamic_pointer_cast<TableWorkspace>(*it);

        if(!tableWs) {
            throw std::invalid_argument("Can only construct PoldiPeakCollection from a TableWorkspace.");
        }

        peakCollections.push_back(boost::make_shared<PoldiPeakCollection>(tableWs));
    }

    return peakCollections;
}

/// Maps a vector of workspaces to a vector of strings by extracting the name.
std::vector<std::string> PoldiIndexKnownCompounds::getWorkspaceNames(const std::vector<Workspace_sptr> &workspaces) const
{
    std::vector<std::string> names;

    for(auto it = workspaces.begin(); it != workspaces.end(); ++it) {
        names.push_back((*it)->getName());
    }

    return names;
}

/** Returns a vector with specified size based on an input vector.
 *
 *  This method "corrects" the size of a given input vector:
 *    - If the vector is empty or size is 0, the method throws an std::invalid_argument exception.
 *    - If the vector already has the correct size, the vector is returned unchanged.
 *    - If the vector contains more than "size" elements, the first "size" elements are returned.
 *    - If the vector is smaller than "size", it is expanded to "size", all new elements being equal to the last element of the input vector.
 *
 *  The method is used to make sure tolerance- and contribution-vectors have a length
 *  that matches the number of expected phases. If only 1 tolerance is supplied but 3 phases,
 *  this method creates a vector of 3 tolerance-values (all equal to the 1 value that was supplied).
 *
 *  @param vector :: Input vector of length n, n > 0.
 *  @param size :: Length of output vector m, m > 0.
 *
 *  @return Vector of length m, content depends on input vector.
 */
std::vector<double> PoldiIndexKnownCompounds::reshapeVector(const std::vector<double> &vector, size_t size) const
{
    if(vector.empty() || size == 0) {
        throw std::invalid_argument("Cannot process empty vector.");
    }

    if(vector.size() == size) {
        return vector;
    }

    if(vector.size() > size) {
        return std::vector<double>(vector.begin(), vector.begin() + size);
    }

    std::vector<double> returnVector(vector);
    returnVector.resize(size, vector.back());

    return returnVector;
}

/// Returns scattering contributions to be used for indexing. The actual size of the vector is determined by PoldiIndexKnownCompounds::reshapeVector.
std::vector<double> PoldiIndexKnownCompounds::getContributions(size_t size) const
{
    return reshapeVector(getProperty("ScatteringContributions"), size);
}

/** Creates a vector which contains elements with a sum equal to 1
 *
 *  This method takes a vector of numbers and normalizes them so that the sum of
 *  the output vector is 1. It throws an std::invalid_argument exception if the sum
 *  of the input vector is 0 or if negative elements are present in the input vector.
 *
 *  It's used to normalize scattering contributions so that they may be given
 *  on an arbitrary scale such as [10,2,1,1] for a 4-component mixture.
 *
 *  @param contributions :: Relative contributions on arbitrary scale
 *  @return Relative contributions so that the sum is 1.
 */
std::vector<double> PoldiIndexKnownCompounds::getNormalizedContributions(const std::vector<double> &contributions) const
{
    double sum = std::accumulate(contributions.begin(), contributions.end(), 0.0);

    if(sum == 0.0) {
        throw std::invalid_argument("Sum of contributions is 0.");
    }

    std::vector<double> normalizedContributions;
    for(auto it = contributions.begin(); it != contributions.end(); ++it) {
        if(*it < 0.0) {
            throw std::invalid_argument("Contributions less than 0 are not allowed.");
        }

        normalizedContributions.push_back(*it / sum);
    }

    return normalizedContributions;
}

/// Scales the intensities of peaks in expected phases by normalized scattering contributions, throws std::invalid_argument if lengths do not match.
void PoldiIndexKnownCompounds::scaleIntensityEstimates(const std::vector<PoldiPeakCollection_sptr> &peakCollections, const std::vector<double> &normalizedContributions) const
{
    if(peakCollections.size() != normalizedContributions.size()) {
        throw std::invalid_argument("Number of PeakCollections is different from number of contributions. Aborting.");
    }

    for(size_t i = 0; i < peakCollections.size(); ++i) {
        scaleIntensityEstimates(peakCollections[i], normalizedContributions[i]);
    }
}

/** Scales the intensities of all peaks in the collection by the supplied scattering contribution
 *
 *  This method scales intensities of peaks contained in the supplied PoldiPeakCollection
 *  (if it's a null-pointer, the method throws an std::invalid_argument exception). The original
 *  intensity is multiplied by the contribution factor.
 *
 *  @param peakCollection :: PoldiPeakCollection with expected peaks of one phase.
 *  @param contribution :: Scattering contribution of that material.
 */
void PoldiIndexKnownCompounds::scaleIntensityEstimates(const PoldiPeakCollection_sptr &peakCollection, double contribution) const
{
    if(!peakCollection) {
        throw std::invalid_argument("Cannot assign intensities to invalid PoldiPeakCollection.");
    }

    size_t peakCount = peakCollection->peakCount();

    for(size_t i = 0; i < peakCount; ++i) {
        PoldiPeak_sptr peak = peakCollection->peak(i);

        peak->setIntensity(peak->intensity() * contribution);
    }
}

void PoldiIndexKnownCompounds::scaleToExperimentalValues(const std::vector<PoldiPeakCollection_sptr> &peakCollections, const PoldiPeakCollection_sptr &measuredPeaks) const
{
    double maximumExperimentalIntensity = getMaximumIntensity(measuredPeaks);

    double maximumCalculatedIntensity = 0.0;
    for(auto it = peakCollections.begin(); it != peakCollections.end(); ++it) {
        maximumCalculatedIntensity = std::max(getMaximumIntensity(*it), maximumCalculatedIntensity);
    }

    scaleIntensityEstimates(peakCollections, std::vector<double>(peakCollections.size(), maximumExperimentalIntensity / maximumCalculatedIntensity));
}

double PoldiIndexKnownCompounds::getMaximumIntensity(const PoldiPeakCollection_sptr &peakCollection) const
{
    size_t i = getMaximumIntensityPeakIndex(peakCollection);

    return peakCollection->peak(i)->intensity();
}

size_t PoldiIndexKnownCompounds::getMaximumIntensityPeakIndex(const PoldiPeakCollection_sptr &peakCollection) const
{
    double maxInt = 0.0;
    size_t maxIndex = 0;

    for(size_t i = 0; i < peakCollection->peakCount(); ++i) {
        PoldiPeak_sptr currentPeak = peakCollection->peak(i);
        double currentInt = currentPeak->intensity();
        if(currentInt > maxInt) {
            maxInt = currentInt;
            maxIndex = i;
        }
    }

    return maxIndex;
}

/// Returns tolerances to be used for indexing. The actual size of the vector is determined by PoldiIndexKnownCompounds::reshapeVector.
std::vector<double> PoldiIndexKnownCompounds::getTolerances(size_t size) const
{
    return reshapeVector(getProperty("Tolerances"), size);
}

/// Assigns tolerances for each component as FWHM on all expected peaks of the corresponding component, throws std::invalid_argument if vector lengths differ.
void PoldiIndexKnownCompounds::assignFwhmEstimates(const std::vector<PoldiPeakCollection_sptr> &peakCollections, const std::vector<double> &tolerances) const
{
    if(peakCollections.size() != tolerances.size()) {
        throw std::invalid_argument("Number of PeakCollections is different from number of contributions. Aborting.");
    }

    for(size_t i = 0; i < peakCollections.size(); ++i) {
        assignFwhmEstimates(peakCollections[i], tolerances[i]);
    }
}

/// Converts the given tolerance (interpreted as standard deviation of a normal probability distribution) to FWHM and assigns that to all peaks of the supplied collection.
void PoldiIndexKnownCompounds::assignFwhmEstimates(const PoldiPeakCollection_sptr &peakCollection, double tolerance) const
{
    if(!peakCollection) {
        throw std::invalid_argument("Cannot assign intensities to invalid PoldiPeakCollection.");
    }

    size_t peakCount = peakCollection->peakCount();
    double fwhm = sigmaToFwhm(tolerance);

    for(size_t i = 0; i < peakCount; ++i) {
        PoldiPeak_sptr peak = peakCollection->peak(i);
        peak->setFwhm(UncertainValue(fwhm), PoldiPeak::Relative);
    }
}

/** Tries to index the supplied measured peaks
 *
 *  The method tries to index the supplied measured peaks by finding peaks with similar d-spacings
 *  in the collections of known compounds. First, a list of candidate pairs is created
 *  (see PoldiIndexKnownCompounds::getAllIndexCandidatePairs), then PoldiIndexKnownCompounds::assignCandidates
 *  selects the most likely pairs for indexing.
 *
 *  @param measured :: Measured peaks.
 *  @param knownCompoundPeaks :: Collections of expected peaks.
 */
void PoldiIndexKnownCompounds::indexPeaks(const PoldiPeakCollection_sptr &measured, const std::vector<PoldiPeakCollection_sptr> &knownCompoundPeaks)
{
    if(!measured) {
        throw std::invalid_argument("Cannot index invalid PoldiPeakCollection.");
    }

    g_log.information() << "  Creating list of index candidates..." << std::endl;
    std::vector<IndexCandidatePair> candidates = getAllIndexCandidatePairs(measured, knownCompoundPeaks);

    g_log.information() << "  Number of candidate pairs: " << candidates.size() << std::endl
                        << "  Assigning most likely candidates..." << std::endl;
    assignCandidates(candidates);
}

/** Creates a vector of IndexCandidatePair-objects.
 *
 *  This function iterates through all peaks in the measured PoldiPeakCollection, getting possible indexing candidates
 *  for each peak (see PoldiIndexKnownCompounds::getIndexCandidatePairs). If no candidates are found it means that
 *  there are no reflections with similar d-spacings from the known compounds, so it is treated as an unindexed peak.
 *  Otherwise, all candidates for this peak are appended to the list of existing candidate pairs.
 *
 *  @param measured :: Measured peaks.
 *  @param knownCompoundPeaks :: Collections of expected peaks.
 *  @return Vector of index candidates.
 */
std::vector<IndexCandidatePair> PoldiIndexKnownCompounds::getAllIndexCandidatePairs(const PoldiPeakCollection_sptr &measured, const std::vector<PoldiPeakCollection_sptr> &knownCompoundPeaks)
{
    std::vector<IndexCandidatePair> candidates;

    size_t peakCount = measured->peakCount();
    for(size_t i = 0; i < peakCount; ++i) {
        PoldiPeak_sptr currentPeak = measured->peak(i);

        std::vector<IndexCandidatePair> currentCandidates = getIndexCandidatePairs(currentPeak, knownCompoundPeaks);

        if(currentCandidates.empty()) {
            collectUnindexedPeak(currentPeak);
        } else {
            candidates.insert(candidates.end(), currentCandidates.begin(), currentCandidates.end());
        }

        g_log.information() << "    Peak at d=" << static_cast<double>(currentPeak->d()) << " has " << currentCandidates.size() << " candidates." << std::endl;
    }

    return candidates;
}

/// Uses PoldiIndexKnownCompounds::isCandidate to find all candidates for supplied peak in the candidate peak collections.
std::vector<IndexCandidatePair> PoldiIndexKnownCompounds::getIndexCandidatePairs(const PoldiPeak_sptr &peak, const std::vector<PoldiPeakCollection_sptr> &candidateCollections) const
{
    std::vector<IndexCandidatePair> indexCandidates;

    for(size_t i = 0; i < candidateCollections.size(); ++i) {
        PoldiPeakCollection_sptr currentCandidateCollection = candidateCollections[i];

        size_t peakCount = currentCandidateCollection->peakCount();
        for(size_t p = 0; p < peakCount; ++p) {
            PoldiPeak_sptr currentCandidate = currentCandidateCollection->peak(p);

            if(isCandidate(peak, currentCandidate)) {
                indexCandidates.push_back(IndexCandidatePair(peak, currentCandidate, i));
            }
        }
    }

    return indexCandidates;
}

/// Returns true if d-spacing of measured and candidate peak are less than three sigma (of the candidate) apart.
bool PoldiIndexKnownCompounds::isCandidate(const PoldiPeak_sptr &measuredPeak, const PoldiPeak_sptr &possibleCandidate) const
{
    if(!measuredPeak || !possibleCandidate) {
        throw std::invalid_argument("Cannot check null-peaks.");
    }

    return ( fabs(static_cast<double>(measuredPeak->d()) - possibleCandidate->d()) / fwhmToSigma(possibleCandidate->fwhm(PoldiPeak::AbsoluteD)) ) < 3.0;
}

/// Inserts the supplied peak into m_unindexedPeaks, throws std::runtime_error if m_unindexedPeaks has not been initialized.
void PoldiIndexKnownCompounds::collectUnindexedPeak(const PoldiPeak_sptr &unindexedPeak)
{
    if(!m_unindexedPeaks) {
        throw std::runtime_error("Collection for unindexed peaks has not been initialized.");
    }

    m_unindexedPeaks->addPeak(unindexedPeak);
}

/** Assigns most likely indices to measured peaks.
 *
 *  This method takes a list of index candidate pairs and sorts it according to their score, because
 *  a high score corresponds to a high probability of the assignment being correct. Then the function
 *  takes the element with the highest score and inspects the IndexCandidatePair. If the measured reflection
 *  does not have an index yet, it checks whether the candidate index has already been used. In that case,
 *  the measured peak is stored in a buffer. Otherwise, the candidate presents the best solution for the
 *  measured peak (because the current pair has the highest score in the list of remaining candidate pairs) and
 *  the solution is accepted. This means that the measured as well as the candidate peak are marked as "used" and
 *  the measured peak is stored in the PoldiPeakCollection that is located at the index of the current pair.
 *
 *  Then the next element is checked and so on. Whenever a "next best" solution for a measured peak in the mentioned
 *  buffer is found, the peak is removed from that buffer. Once all candidate pairs have been evaluated, the peaks
 *  that are still in the buffer are considered unindexed and treated accordingly.
 *
 *  For a more complete explanation of the principle, please check the documentation in the wiki.
 *
 *  @param candidates :: Vector of possible index candidates.
 */
void PoldiIndexKnownCompounds::assignCandidates(const std::vector<IndexCandidatePair> &candidates)
{
    // Make a copy since this is going to be modified
    std::vector<IndexCandidatePair> workCandidates = candidates;

    /* The vector is sorted by score (see comparison operator of PeakCandidate),
     * so the first element has the lowest score (lowest probability of being
     * a good guess).
     */
    std::sort(workCandidates.begin(), workCandidates.end());

    std::set<PoldiPeak_sptr> usedMeasuredPeaks;
    std::set<PoldiPeak_sptr> usedExpectedPeaks;

    std::set<PoldiPeak_sptr> unassignedMeasuredPeaks;

    /* The candidate at the back of the vector has the highest score,
     * so it's the candidate with the highest probability of being correct.
     * Consequently, the vector is iterated from end to beginning.
     */
    for(auto it = workCandidates.rbegin(); it != workCandidates.rend(); ++it) {
        IndexCandidatePair currentCandidate = *it;

        PoldiPeak_sptr measuredPeak = currentCandidate.observed;
        PoldiPeak_sptr expectedPeak = currentCandidate.candidate;

        g_log.information() << "    Candidate d=" << static_cast<double>(measuredPeak->d()) << " -> "
                            << "Phase: " << currentCandidate.candidateCollectionIndex << " [" << MillerIndicesIO::toString(expectedPeak->hkl()) << "] (d=" << static_cast<double>(expectedPeak->d()) << "), "
                            << "Score=(" << currentCandidate.positionMatch << "): ";

        /* If the peak has not been indexed yet, it is not stored in the set
         * that holds measured peaks that are already indexed, so the candidate
         * needs to be examined further.
         */
        if( !inPeakSet(usedMeasuredPeaks, measuredPeak) ) {

            /* If the theoretical reflection of this index-candidate has already been
             * assigned to a measured peak, the measured peak is inserted into
             * a second set where it is kept in case there is another candidate
             * for this measured peak.
             */
            if ( inPeakSet(usedExpectedPeaks, expectedPeak) ) {
                unassignedMeasuredPeaks.insert(measuredPeak);
                g_log.information() << "      Candidate rejected: Candidate has been already used." << std::endl;
            } else {
                /* Otherwise, the indexed candidate is accepted and the measured peak
                 * is removed from the set of peaks that are waiting for another solution.
                 */
                if( inPeakSet(unassignedMeasuredPeaks, measuredPeak) ) {
                    unassignedMeasuredPeaks.erase(measuredPeak);
                }

                usedExpectedPeaks.insert(expectedPeak);
                usedMeasuredPeaks.insert(measuredPeak);

                assignPeakIndex(currentCandidate);
                g_log.information() << "      Candidate accepted." << std::endl;
            }
        } else {
            g_log.information() << "      Candidate rejected: peak has already been indexed: [" << MillerIndicesIO::toString(measuredPeak->hkl()) << "]." << std::endl;
        }
    }

    /* All peaks that are still in this set at this point are not indexed and thus inserted into the
     * peak collection that holds unindexed peaks.
     */
    for(auto it = unassignedMeasuredPeaks.begin(); it != unassignedMeasuredPeaks.end(); ++it) {
        collectUnindexedPeak(*it);
    }
}

/// Returns true if the supplied set contains the peak.
bool PoldiIndexKnownCompounds::inPeakSet(const std::set<PoldiPeak_sptr> &peakSet, const PoldiPeak_sptr &peak) const
{
    return peakSet.find(peak) != peakSet.end();
}

/// Places the measured peak of the IndexCandidatePair in the correct peak collection.
void PoldiIndexKnownCompounds::assignPeakIndex(const IndexCandidatePair &candidate)
{
    candidate.observed->setHKL(candidate.candidate->hkl());

    m_indexedPeaks[candidate.candidateCollectionIndex]->addPeak(candidate.observed);
}

PoldiPeakCollection_sptr PoldiIndexKnownCompounds::getIntensitySortedPeakCollection(const PoldiPeakCollection_sptr &peaks) const
{
    std::vector<PoldiPeak_sptr> peakVector(peaks->peaks());

    std::sort(peakVector.begin(), peakVector.end(), boost::bind<bool>(&PoldiPeak::greaterThan, _1, _2, &PoldiPeak::intensity));

    PoldiPeakCollection_sptr sortedPeaks = boost::make_shared<PoldiPeakCollection>(peaks->intensityType());
    for(size_t i = 0; i < peakVector.size(); ++i) {
        sortedPeaks->addPeak(peakVector[i]->clone());
    }

    return sortedPeaks;
}

/** Initialize the algorithm's properties.
   */
void PoldiIndexKnownCompounds::init()
{
    declareProperty(
                new WorkspaceProperty<TableWorkspace>("InputWorkspace", "", Direction::Input), "Workspace that contains unindexed peaks.");

    declareProperty(
                new ArrayProperty<std::string>("CompoundWorkspaces"),
                "A comma-separated list of workspace names or a workspace group. Each workspace must contain a list of indexed reflections.");

    declareProperty(
                new ArrayProperty<double>("Tolerances", std::vector<double>(1, 0.01)),
                "Maximum relative tolerance delta(d)/d for lines to be indexed. Either one value or one for each compound.");

    declareProperty(
                new ArrayProperty<double>("ScatteringContributions", std::vector<double>(1, 1.0)),
                "Approximate scattering contribution ratio of the compounds. If omitted, all are assumed to contribute to scattering equally.");

    declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspace","",Direction::Output), "A workspace group that contains workspaces with indexed and unindexed reflections from the input workspace.");
}

/** Execute the algorithm.
   */
void PoldiIndexKnownCompounds::exec()
{
    g_log.information() << "Starting POLDI peak indexing." << std::endl;

    DataObjects::TableWorkspace_sptr peakTableWorkspace = getProperty("InputWorkspace");

    PoldiPeakCollection_sptr unindexedPeaks = boost::make_shared<PoldiPeakCollection>(peakTableWorkspace);
    g_log.information() << "  Number of peaks: " << unindexedPeaks->peakCount() << std::endl;

    std::vector<Workspace_sptr> workspaces = getWorkspaces(getProperty("CompoundWorkspaces"));
    std::vector<PoldiPeakCollection_sptr> peakCollections = getPeakCollections(workspaces);
    g_log.information() << "  Number of phases: " << peakCollections.size() << std::endl;

    /* The procedure is much easier to formulate with some state stored in member variables,
     * which are initialized either from user input or from some defaults.
     */
    setMeasuredPeaks(unindexedPeaks);
    setExpectedPhases(peakCollections);
    setExpectedPhaseNames(getWorkspaceNames(workspaces));

    initializeUnindexedPeaks();
    initializeIndexedPeaks(m_expectedPhases);

    /* For calculating scores in the indexing procedure, scattering contributions are used.
     * The structure factors are scaled accordingly.
     */
    std::vector<double> contributions = getContributions(m_expectedPhases.size());
    std::vector<double> normalizedContributions = getNormalizedContributions(contributions);

    scaleIntensityEstimates(peakCollections, normalizedContributions);
    scaleToExperimentalValues(peakCollections, unindexedPeaks);

    // Tolerances on the other hand are handled as "FWHM".
    std::vector<double> tolerances = getTolerances(m_expectedPhases.size());
    assignFwhmEstimates(peakCollections, tolerances);

    // With all necessary state assigned, the indexing procedure can be executed
    indexPeaks(unindexedPeaks, peakCollections);

    g_log.information() << "  Unindexed peaks: " << m_unindexedPeaks->peakCount() << std::endl;

    /* Finally, the peaks are put into separate workspaces, determined by
     * the phase they have been attributed to, plus unindexed peaks.
     */
    WorkspaceGroup_sptr outputWorkspaces = boost::make_shared<WorkspaceGroup>();

    for(size_t i = 0; i < m_indexedPeaks.size(); ++i) {
        PoldiPeakCollection_sptr intensitySorted = getIntensitySortedPeakCollection(m_indexedPeaks[i]);
        ITableWorkspace_sptr tableWs = intensitySorted->asTableWorkspace();
        AnalysisDataService::Instance().addOrReplace("Indexed_" + m_phaseNames[i], tableWs);

        outputWorkspaces->addWorkspace(tableWs);
    }

    ITableWorkspace_sptr unindexedTableWs = m_unindexedPeaks->asTableWorkspace();

    std::string inputWorkspaceName = getPropertyValue("InputWorkspace");
    AnalysisDataService::Instance().addOrReplace("Unindexed_" + inputWorkspaceName, unindexedTableWs);
    outputWorkspaces->addWorkspace(unindexedTableWs);

    setProperty("OutputWorkspace", outputWorkspaces);
}



} // namespace Poldi
} // namespace Mantid
