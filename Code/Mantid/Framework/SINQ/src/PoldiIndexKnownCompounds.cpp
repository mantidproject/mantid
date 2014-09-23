#include "MantidSINQ/PoldiIndexKnownCompounds.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidSINQ/PoldiUtilities/MillerIndicesIO.h"
#include "MantidAPI/AlgorithmFactory.h"

#include <boost/bind.hpp>

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



//----------------------------------------------------------------------------------------------
/** Constructor
   */
PoldiIndexKnownCompounds::PoldiIndexKnownCompounds()
{
}

//----------------------------------------------------------------------------------------------
/** Destructor
   */
PoldiIndexKnownCompounds::~PoldiIndexKnownCompounds()
{
}

const std::string PoldiIndexKnownCompounds::name() const
{
    return "PoldiIndexKnownCompounds";
}


//----------------------------------------------------------------------------------------------


/// Algorithm's version for identification. @see Algorithm::version
int PoldiIndexKnownCompounds::version() const { return 1;}

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiIndexKnownCompounds::category() const { return "SINQ\\Poldi";}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiIndexKnownCompounds::summary() const { return "Index peaks using known compounds.";}

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

void PoldiIndexKnownCompounds::setMeasuredPeaks(const PoldiPeakCollection_sptr &measuredPeaks)
{
    m_measuredPeaks = measuredPeaks;
}

void PoldiIndexKnownCompounds::setExpectedPhases(const std::vector<PoldiPeakCollection_sptr> &expectedPhases)
{
    m_expectedPhases = expectedPhases;
}

void PoldiIndexKnownCompounds::setExpectedPhaseNames(const std::vector<std::string> &phaseNames)
{
    m_phaseNames = phaseNames;
}

void PoldiIndexKnownCompounds::initializeUnindexedPeaks()
{
    m_unindexedPeaks = boost::make_shared<PoldiPeakCollection>();
}

void PoldiIndexKnownCompounds::initializeIndexedPeaks(const std::vector<PoldiPeakCollection_sptr> &expectedPhases)
{
    m_indexedPeaks.clear();

    for(size_t i = 0; i < expectedPhases.size(); ++i) {
        PoldiPeakCollection_sptr newCollection = boost::make_shared<PoldiPeakCollection>();
        newCollection->setPointGroup(expectedPhases[i]->pointGroup());

        m_indexedPeaks.push_back(newCollection);
    }
}

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

std::vector<std::string> PoldiIndexKnownCompounds::getWorkspaceNames(const std::vector<Workspace_sptr> &workspaces)
{
    std::vector<std::string> names;

    for(auto it = workspaces.begin(); it != workspaces.end(); ++it) {
        names.push_back((*it)->getName());
    }

    return names;
}

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

std::vector<double> PoldiIndexKnownCompounds::getTolerances(size_t size) const
{
    return reshapeVector(getProperty("Tolerances"), size);
}

std::vector<double> PoldiIndexKnownCompounds::getContributions(size_t size) const
{
    return reshapeVector(getProperty("ScatteringContributions"), size);
}

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

void PoldiIndexKnownCompounds::assignIntensityEstimates(const std::vector<PoldiPeakCollection_sptr> &peakCollections, const std::vector<double> &normalizedContributions) const
{
    if(peakCollections.size() != normalizedContributions.size()) {
        throw std::invalid_argument("Number of PeakCollections is different from number of contributions. Aborting.");
    }

    for(size_t i = 0; i < peakCollections.size(); ++i) {
        assignIntensityEstimates(peakCollections[i], normalizedContributions[i]);
    }
}

void PoldiIndexKnownCompounds::assignIntensityEstimates(const PoldiPeakCollection_sptr &peakCollection, double contribution) const
{
    if(!peakCollection) {
        throw std::invalid_argument("Cannot assign intensities to invalid PoldiPeakCollection.");
    }

    size_t peakCount = peakCollection->peakCount();

    for(size_t i = 0; i < peakCount; ++i) {
        PoldiPeak_sptr peak = peakCollection->peak(i);

        V3D hkl = peak->hkl().asV3D();
        double multiplicity = getMultiplicity(peakCollection, hkl);

        peak->setIntensity(UncertainValue(multiplicity * contribution));
    }
}

double PoldiIndexKnownCompounds::getMultiplicity(const PoldiPeakCollection_sptr &peakCollection, const V3D &hkl) const
{
    if(!peakCollection) {
        return 1.0;
    }

    Geometry::PointGroup_sptr pointGroup = peakCollection->pointGroup();

    bool useMultiplicityWeights = getProperty("UseMultiplicityWeights");

    if(!pointGroup || !useMultiplicityWeights) {
        return 1.0;
    }

    return static_cast<double>(pointGroup->getEquivalents(hkl).size());
}

void PoldiIndexKnownCompounds::assignFwhmEstimates(const std::vector<PoldiPeakCollection_sptr> &peakCollections, const std::vector<double> &tolerances) const
{
    if(peakCollections.size() != tolerances.size()) {
        throw std::invalid_argument("Number of PeakCollections is different from number of contributions. Aborting.");
    }

    for(size_t i = 0; i < peakCollections.size(); ++i) {
        assignFwhmEstimates(peakCollections[i], tolerances[i]);
    }
}

void PoldiIndexKnownCompounds::assignFwhmEstimates(const PoldiPeakCollection_sptr &peakCollection, double tolerance) const
{
    if(!peakCollection) {
        throw std::invalid_argument("Cannot assign intensities to invalid PoldiPeakCollection.");
    }

    size_t peakCount = peakCollection->peakCount();

    for(size_t i = 0; i < peakCount; ++i) {
        PoldiPeak_sptr peak = peakCollection->peak(i);
        peak->setFwhm(UncertainValue(sigmaToFwhm(tolerance)), PoldiPeak::Relative);
    }
}

void PoldiIndexKnownCompounds::indexPeaks(const PoldiPeakCollection_sptr &unindexed, const std::vector<PoldiPeakCollection_sptr> &knownCompounds)
{
    if(!unindexed) {
        throw std::invalid_argument("Cannot index invalid PoldiPeakCollection.");
    }

    g_log.information() << "  Creating list of index candidates..." << std::endl;
    std::vector<PeakCandidate> candidates = getAllCandidates(unindexed, knownCompounds);

    g_log.information() << "  Number of candidate pairs: " << candidates.size() << std::endl
                        << "  Assigning most likely candidates..." << std::endl;
    assignCandidates(candidates);
}

void PoldiIndexKnownCompounds::assignCandidates(const std::vector<PeakCandidate> &candidates)
{
    // Make a copy since this is going to be modified
    std::vector<PeakCandidate> workCandidates = candidates;

    /* The vector is sorted by score (see comparison operator of PeakCandidate),
     * so the first element has the lowest score (lowest probability of being
     * a good guess).
     */
    std::sort(workCandidates.begin(), workCandidates.end());

    std::set<PoldiPeak_sptr> usedMeasuredPeaks;
    std::set<PoldiPeak_sptr> usedExpectedPeaks;

    std::set<PoldiPeak_sptr> unassignedMeasuredPeaks;

    while(!workCandidates.empty()) {
        /* The candidate at the back of the vector has the highest score,
         * so it's the candidate with the highest probability of being correct.
         */
        PeakCandidate currentCandidate = workCandidates.back();

        PoldiPeak_sptr measuredPeak = currentCandidate.unindexed;
        PoldiPeak_sptr expectedPeak = currentCandidate.candidate;

        g_log.information() << "    Candidate d=" << static_cast<double>(measuredPeak->d()) << " -> "
                            << "Phase: " << currentCandidate.candidateCollection << " [" << MillerIndicesIO::toString(expectedPeak->hkl()) << "] (d=" << static_cast<double>(expectedPeak->d()) << "), "
                            << "Score=" << currentCandidate.score << ": ";

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

        /* The candidate has been processed in some way, so it is no longer required
         * and removed from the list.
         */
        workCandidates.pop_back();
    }

    /* All peaks that are still in this set at this point are not indexed and thus inserted into the
     * peak collection that holds unindexed peaks.
     */
    for(auto it = unassignedMeasuredPeaks.begin(); it != unassignedMeasuredPeaks.end(); ++it) {
        collectUnindexedPeak(*it);
    }
}

std::vector<PeakCandidate> PoldiIndexKnownCompounds::getAllCandidates(const PoldiPeakCollection_sptr &unindexed, const std::vector<PoldiPeakCollection_sptr> &knownCompounds)
{
    std::vector<PeakCandidate> candidates;

    size_t peakCount = unindexed->peakCount();
    for(size_t i = 0; i < peakCount; ++i) {
        PoldiPeak_sptr currentPeak = unindexed->peak(i);

        std::vector<PeakCandidate> currentCandidates = getPeakCandidates(currentPeak, knownCompounds);

        if(currentCandidates.empty()) {
            collectUnindexedPeak(currentPeak);
        } else {
            candidates.insert(candidates.end(), currentCandidates.begin(), currentCandidates.end());
        }

        g_log.information() << "    Peak at d=" << static_cast<double>(currentPeak->d()) << " has " << currentCandidates.size() << " candidates." << std::endl;
    }

    return candidates;
}

void PoldiIndexKnownCompounds::collectUnindexedPeak(const PoldiPeak_sptr &unindexedPeak)
{
    if(!m_unindexedPeaks) {
        throw std::runtime_error("Collection for unindexed peaks has not been initialized.");
    }

    m_unindexedPeaks->addPeak(unindexedPeak);
}

std::vector<PeakCandidate> PoldiIndexKnownCompounds::getPeakCandidates(const PoldiPeak_sptr &peak, const std::vector<PoldiPeakCollection_sptr> &candidateCollections) const
{
    std::vector<PeakCandidate> indexCandidates;

    for(size_t i = 0; i < candidateCollections.size(); ++i) {
        PoldiPeakCollection_sptr currentCandidateCollection = candidateCollections[i];

        size_t peakCount = currentCandidateCollection->peakCount();
        for(size_t p = 0; p < peakCount; ++p) {
            PoldiPeak_sptr currentCandidate = currentCandidateCollection->peak(p);

            if(isCandidate(peak, currentCandidate)) {
                indexCandidates.push_back(PeakCandidate(peak, currentCandidate, i));
            }
        }
    }

    return indexCandidates;
}

bool PoldiIndexKnownCompounds::isCandidate(const PoldiPeak_sptr &measuredPeak, const PoldiPeak_sptr &possibleCandidate) const
{
    if(!measuredPeak || !possibleCandidate) {
        throw std::invalid_argument("Cannot check null-peaks.");
    }

    return ( fabs(static_cast<double>(measuredPeak->d()) - possibleCandidate->d()) / fwhmToSigma(possibleCandidate->fwhm(PoldiPeak::AbsoluteD)) ) < 3.0;
}

bool PoldiIndexKnownCompounds::inPeakSet(const std::set<PoldiPeak_sptr> &peakSet, const PoldiPeak_sptr &peak) const
{
    return peakSet.find(peak) != peakSet.end();
}

double PoldiIndexKnownCompounds::fwhmToSigma(double fwhm) const
{
    return fwhm / (2.0 * sqrt(2.0 * log(2.0)));
}

double PoldiIndexKnownCompounds::sigmaToFwhm(double sigma) const
{
    return sigma * (2.0 * sqrt(2.0 * log(2.0)));
}

void PoldiIndexKnownCompounds::assignPeakIndex(const PeakCandidate &candidate)
{
    candidate.unindexed->setHKL(candidate.candidate->hkl());

    m_indexedPeaks[candidate.candidateCollection]->addPeak(candidate.unindexed);
}


//----------------------------------------------------------------------------------------------
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

    declareProperty("UseMultiplicityWeights", false, "Weight peaks position probability by reflection multiplicty.");

    declareProperty(new WorkspaceProperty<WorkspaceGroup>("OutputWorkspace","",Direction::Output), "A workspace group that contains workspaces with indexed and unindexed reflections from the input workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
   */
void PoldiIndexKnownCompounds::exec()
{
    g_log.information() << "Starting POLDI peak indexing." << std::endl;

    PoldiPeakCollection_sptr unindexedPeaks = boost::make_shared<PoldiPeakCollection>(getProperty("InputWorkspace"));
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

    /* For calculating scores in the indexing procedure, scattering contributions as well as
     * reflection multiplicities can be used (if available). These are assigned as "intensities"
     * on the reflections of the respective peak collections.
     */
    std::vector<double> contributions = getContributions(m_expectedPhases.size());
    std::vector<double> normalizedContributions = getNormalizedContributions(contributions);

    assignIntensityEstimates(peakCollections, normalizedContributions);

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
        ITableWorkspace_sptr tableWs = m_indexedPeaks[i]->asTableWorkspace();
        AnalysisDataService::Instance().add("Indexed_" + m_phaseNames[i], tableWs);

        outputWorkspaces->addWorkspace(tableWs);
    }


    ITableWorkspace_sptr unindexedTableWs = m_unindexedPeaks->asTableWorkspace();
    AnalysisDataService::Instance().add("Unindexed", unindexedTableWs);
    outputWorkspaces->addWorkspace(unindexedTableWs);

    setProperty("OutputWorkspace", outputWorkspaces);
}



} // namespace Poldi
} // namespace Mantid
