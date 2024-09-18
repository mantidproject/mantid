// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidCrystal/ConnectedComponentLabeling.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/Cluster.h"
#include "MantidCrystal/ClusterRegister.h"
#include "MantidCrystal/ICluster.h"
#include "MantidKernel/Memory.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Crystal::ConnectedComponentMappingTypes;

namespace Mantid::Crystal {
namespace {
/**
 * Perform integer power to determine the maximum number of face and edge
 * connected
 * neighbours for a given dimensionality
 * @param ws : Workspace with dimensionality
 * @return : Maximum number of possible neighbours
 */
size_t calculateMaxNeighbours(IMDHistoWorkspace const *const ws) {
  const size_t ndims = ws->getNumDims();
  size_t maxNeighbours = 3;
  for (size_t i = 1; i < ndims; ++i) {
    maxNeighbours *= 3;
  }
  maxNeighbours -= 1;
  return maxNeighbours;
}

/**
 * Get the maximum number of clusters possible in an MD space. For use as an
 * offset.
 * @param ws : Workspace to interogate
 * @param nIterators : number of equally sized iterators working on the
 * workspace
 * @return Maximum number of clusters.
 */
size_t calculateMaxClusters(IMDHistoWorkspace const *const ws, size_t nIterators) {
  size_t maxClusters = 1;
  for (size_t i = 0; i < ws->getNumDims(); ++i) {
    maxClusters *= ws->getDimension(i)->getNBins() / 2;
  }
  maxClusters /= nIterators;
  if (maxClusters == 0) {
    maxClusters = ws->getNPoints();
  }
  return maxClusters;
}

/**
 * Helper non-member to clone the input workspace
 * @param inWS: To clone
 * @return : Cloned MDHistoWorkspace
 */
std::shared_ptr<Mantid::API::IMDHistoWorkspace> cloneInputWorkspace(IMDHistoWorkspace_sptr &inWS) {
  IMDHistoWorkspace_sptr outWS(inWS->clone());

  // Initialize to zero.
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < static_cast<int>(outWS->getNPoints()); ++i) {
    outWS->setSignalAt(i, 0);
    outWS->setErrorSquaredAt(i, 0);
  }

  return outWS;
}

/**
 * Helper function to calculate report frequecny
 * @param maxReports : Maximum number of reports wanted
 * @param maxIterations : Maximum number of possible iterations
 * @return
 */
template <typename T> T reportEvery(const T &maxReports, const T &maxIterations) {
  T frequency = maxReports;
  if (maxIterations >= maxReports) {
    frequency = maxIterations / maxReports;
  }
  return frequency;
}

// Helper function for determining if a set contains a specific value.
template <typename Container>
bool does_contain_key(const Container &container, const typename Container::key_type &value) {
  return container.find(value) != container.end();
}

using EdgeIndexPair = boost::tuple<size_t, size_t>;
using VecEdgeIndexPair = std::vector<EdgeIndexPair>;

/**
 * Free function performing the CCL implementation over a range defined by the
 *iterator.
 *
 * @param iterator : Iterator giving access to the image
 * @param strategy : Strategy for identifying background
 * @param neighbourElements : Grid of DisjointElements the same size as the
 *image
 * @param progress : Progress object to update
 * @param maxNeighbours : Maximum number of neighbours each element may have.
 *Determined by dimensionality.
 * @param startLabelId : Start label index to increment
 * @param edgeIndexVec : Vector of edge index pairs. To identify elements across
 *iterator boundaries to resolve later.
 * @return
 */
size_t doConnectedComponentLabeling(IMDIterator *iterator, BackgroundStrategy *const strategy,
                                    VecElements &neighbourElements, Progress &progress, size_t maxNeighbours,
                                    size_t startLabelId, VecEdgeIndexPair &edgeIndexVec) {
  size_t currentLabelCount = startLabelId;
  strategy->configureIterator(iterator); // Set up such things as desired Normalization.
  do {
    if (!strategy->isBackground(iterator)) {

      size_t currentIndex = iterator->getLinearIndex();
      progress.report();
      // Linear indexes of neighbours
      VecIndexes neighbourIndexes = iterator->findNeighbourIndexes();
      VecIndexes nonEmptyNeighbourIndexes;
      nonEmptyNeighbourIndexes.reserve(maxNeighbours);
      SetIds neighbourIds;
      // Discover non-empty neighbours
      for (auto neighIndex : neighbourIndexes) {
        if (!iterator->isWithinBounds(neighIndex)) {
          /* Record labels which appear to belong to the same cluster, but
           cannot be combined in this
           pass and will later need to be conjoined and resolved. As Labels
           cannot be guarnteed to be
           correcly provided for all neighbours until the end. We must store
           indexes instead.
           */
          edgeIndexVec.emplace_back(currentIndex, neighIndex);
          continue;
        }

        const DisjointElement &neighbourElement = neighbourElements[neighIndex];

        if (!neighbourElement.isEmpty()) {
          nonEmptyNeighbourIndexes.emplace_back(neighIndex);
          neighbourIds.insert(neighbourElement.getId());
        }
      }

      if (nonEmptyNeighbourIndexes.empty()) {
        DisjointElement &element = neighbourElements[currentIndex];
        element.setId(static_cast<int>(currentLabelCount));
        ++currentLabelCount;
      } else if (neighbourIds.size() == 1) // Do we have a single unique id amongst all neighbours.
      {
        neighbourElements[currentIndex] = neighbourElements[nonEmptyNeighbourIndexes.front()]; // Copy
                                                                                               // non-empty
                                                                                               // neighbour
      } else {
        // Choose the lowest neighbour index as the parent.
        size_t candidateSourceParentIndex = nonEmptyNeighbourIndexes[0];
        for (size_t i = 1; i < nonEmptyNeighbourIndexes.size(); ++i) {
          size_t neighIndex = nonEmptyNeighbourIndexes[i];
          if (neighbourElements[neighIndex].getRoot() < neighbourElements[candidateSourceParentIndex].getRoot()) {
            candidateSourceParentIndex = neighIndex;
          }
        }
        // Get the chosen parent
        DisjointElement &parentElement = neighbourElements[candidateSourceParentIndex];
        // Union remainder parents with the chosen parent
        for (auto neighIndex : nonEmptyNeighbourIndexes) {
          if (neighIndex != candidateSourceParentIndex) {
            neighbourElements[neighIndex].unionWith(&parentElement);
          }
        }

        neighbourElements[currentIndex].unionWith(&parentElement);
      }
    }
  } while (iterator->next());

  return currentLabelCount;
}

Logger g_log("ConnectedComponentLabeling");

void memoryCheck(size_t nPoints) {
  size_t sizeOfElement = (3 * sizeof(signal_t)) + sizeof(bool);

  MemoryStats memoryStats;
  const size_t freeMemory = memoryStats.availMem();         // in kB
  const size_t memoryCost = sizeOfElement * nPoints / 1000; // in kB
  if (memoryCost > freeMemory) {
    std::string basicMessage = "CCL requires more free memory than you have available.";
    std::stringstream sstream;
    sstream << basicMessage << " Requires " << memoryCost << " KB of contiguous memory.";
    g_log.notice(sstream.str());
    throw std::runtime_error(basicMessage);
  }
}
} // namespace

/**
 * Constructor
 * @param startId : Start Id to use for labeling
 * @param nThreads : Optional argument of number of threads to use.
 */
ConnectedComponentLabeling::ConnectedComponentLabeling(const size_t &startId, const std::optional<int> &nThreads)
    : m_startId(startId) {
  if (nThreads.has_value()) {
    if (nThreads.value() < 0) {
      throw std::invalid_argument("Cannot request that CCL runs with less than one thread!");
    } else {
      m_nThreadsToUse = nThreads.value(); // Follow explicit instructions if provided.
    }
  } else {
    m_nThreadsToUse = API::FrameworkManager::Instance().getNumOMPThreads(); // Figure it out.
  }
}

/**
 * Set a custom start id. This has no bearing on the output of the process other
 * than
 * the initial id used.
 * @param id: Id to start with
 */
void ConnectedComponentLabeling::startLabelingId(const size_t &id) { m_startId = id; }

/**
 @return: The start label id.
 */
size_t ConnectedComponentLabeling::getStartLabelId() const { return m_startId; }

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConnectedComponentLabeling::~ConnectedComponentLabeling() = default;

/**
 * Perform the work of the CCL algorithm
 * - Pre filtering of background
 * - Labeling using DisjointElements
 *
 * @param ws : MDHistoWorkspace to run CCL algorithm on
 * @param baseStrategy : Background strategy
 * @param progress : Progress object
 * @return : Map of label ids to clusters.
 */
ClusterMap ConnectedComponentLabeling::calculateDisjointTree(const IMDHistoWorkspace_sptr &ws,
                                                             BackgroundStrategy *const baseStrategy,
                                                             Progress &progress) const {
  std::map<size_t, std::shared_ptr<ICluster>> clusterMap;
  VecElements neighbourElements(ws->getNPoints());

  const size_t maxNeighbours = calculateMaxNeighbours(ws.get());

  progress.doReport("Identifying clusters");
  auto frequency = reportEvery<size_t>(10000, ws->getNPoints());
  progress.resetNumSteps(frequency, 0.0, 0.8);

  // For each process maintains pair of index from within process bounds to
  // index outside process bounds
  if (m_nThreadsToUse > 1) {
    auto iterators = ws->createIterators(m_nThreadsToUse);
    const size_t maxClustersPossible = calculateMaxClusters(ws.get(), m_nThreadsToUse);

    std::vector<VecEdgeIndexPair> parallelEdgeVec(m_nThreadsToUse);

    std::vector<std::map<size_t, std::shared_ptr<Cluster>>> parallelClusterMapVec(m_nThreadsToUse);

    // ------------- Stage One. Local CCL in parallel.
    g_log.debug("Parallel solve local CCL");
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < m_nThreadsToUse; ++i) {
      API::IMDIterator *iterator = iterators[i].get();
      boost::scoped_ptr<BackgroundStrategy> strategy(baseStrategy->clone()); // local strategy
      VecEdgeIndexPair &edgeVec = parallelEdgeVec[i];                        // local edge indexes

      const size_t startLabel = m_startId + (i * maxClustersPossible); // Ensure that label ids are
                                                                       // totally unique within each
                                                                       // parallel unit.
      const size_t endLabel = doConnectedComponentLabeling(iterator, strategy.get(), neighbourElements, progress,
                                                           maxNeighbours, startLabel, edgeVec);

      // Create clusters from labels.
      std::map<size_t, std::shared_ptr<Cluster>> &localClusterMap = parallelClusterMapVec[i]; // local cluster map.
      for (size_t labelId = startLabel; labelId != endLabel; ++labelId) {
        auto cluster = std::make_shared<Cluster>(labelId); // Create a cluster for the label and key it by the label.
        localClusterMap[labelId] = cluster;
      }

      // Associate the member DisjointElements with a cluster. Involves looping
      // back over iterator.
      iterator->jumpTo(0); // Reset
      do {
        if (!baseStrategy->isBackground(iterator)) {
          // Second pass smoothing step
          const size_t currentIndex = iterator->getLinearIndex();

          const size_t &labelAtIndex = neighbourElements[currentIndex].getRoot();
          localClusterMap[labelAtIndex]->addIndex(currentIndex);
        }
      } while (iterator->next());
    }

    // -------------------- Stage 2 --- Preparation stage for combining
    // equivalent clusters. Must be done in sequence.
    // Combine cluster maps processed by each thread.
    ClusterRegister clusterRegister;
    for (const auto &parallelClusterMap : parallelClusterMapVec) {
      for (const auto &cluster : parallelClusterMap) {
        clusterRegister.add(cluster.first, cluster.second);
      }
    }

    // Percolate minimum label across boundaries for indexes where there is
    // ambiguity.
    g_log.debug("Percolate minimum label across boundaries");

    for (auto &indexPairVec : parallelEdgeVec) {
      for (auto &iit : indexPairVec) {
        const DisjointElement &a = neighbourElements[iit.get<0>()];
        const DisjointElement &b = neighbourElements[iit.get<1>()];
        clusterRegister.merge(a, b);
      }
    }
    clusterMap = clusterRegister.clusters(neighbourElements);

  } else {
    auto iterator = ws->createIterator(nullptr);
    VecEdgeIndexPair edgeIndexPair; // This should never get filled in a single
                                    // threaded situation.
    size_t endLabelId = doConnectedComponentLabeling(iterator.get(), baseStrategy, neighbourElements, progress,
                                                     maxNeighbours, m_startId, edgeIndexPair);

    // Create clusters from labels.
    for (size_t labelId = m_startId; labelId != endLabelId; ++labelId) {
      auto cluster = std::make_shared<Cluster>(labelId); // Create a cluster for the label and key it by the label.
      clusterMap[labelId] = cluster;
    }

    // Associate the member DisjointElements with a cluster. Involves looping
    // back over iterator.
    iterator->jumpTo(0); // Reset
    do {
      const size_t currentIndex = iterator->getLinearIndex();
      if (!baseStrategy->isBackground(iterator.get())) {
        const int labelAtIndex = neighbourElements[currentIndex].getRoot();
        clusterMap[labelAtIndex]->addIndex(currentIndex);
      }
    } while (iterator->next());
  }
  return clusterMap;
}

/**
 * Execute CCL to produce a cluster output workspace containing labels
 * @param ws : Workspace to perform CCL on
 * @param strategy : Background strategy
 * @param progress : Progress object
 * @return Cluster output workspace of results
 */
std::shared_ptr<Mantid::API::IMDHistoWorkspace> ConnectedComponentLabeling::execute(IMDHistoWorkspace_sptr ws,
                                                                                    BackgroundStrategy *const strategy,
                                                                                    Progress &progress) const {
  ClusterTuple result = executeAndFetchClusters(std::move(ws), strategy, progress);
  IMDHistoWorkspace_sptr outWS = result.get<0>(); // Get the workspace, but discard cluster objects.
  return outWS;
}

/**
 * Execute
 * @param ws : Image workspace to integrate
 * @param strategy : Background strategy
 * @param progress : Progress object
 * @return Image Workspace containing clusters as well as a map of label ids to
 * cluster objects.
 */
ClusterTuple ConnectedComponentLabeling::executeAndFetchClusters(IMDHistoWorkspace_sptr ws,
                                                                 BackgroundStrategy *const strategy,
                                                                 Progress &progress) const {
  // Can we run the analysis
  memoryCheck(ws->getNPoints());

  // Perform the bulk of the connected component analysis, but don't collapse
  // the elements yet.
  ClusterMap clusters = calculateDisjointTree(ws, strategy, progress);

  // Create the output workspace from the input workspace
  g_log.debug("Start cloning input workspace");
  IMDHistoWorkspace_sptr outWS = cloneInputWorkspace(ws);
  g_log.debug("Finish cloning input workspace");

  // Get the keys (label ids) first in order to do the next stage in parallel.
  VecIndexes keys;
  keys.reserve(clusters.size());
  std::transform(clusters.cbegin(), clusters.cend(), std::back_inserter(keys),
                 [](const auto &cluster) { return cluster.first; });
  // Write each cluster out to the output workspace
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < static_cast<int>(keys.size()); ++i) { // NOLINT
    clusters[keys[i]]->writeTo(outWS);
  }

  return ClusterTuple(outWS, clusters);
}

} // namespace Mantid::Crystal
