// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/FindClusterFaces.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Utils.h"

#include "MantidCrystal/PeakClusterProjection.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using Mantid::DataObjects::PeaksWorkspace;
using Mantid::DataObjects::PeaksWorkspace_sptr;

namespace {
using namespace Mantid::Crystal;
// Map of label ids to peak index in peaks workspace.
using LabelMap = std::map<int, int>;
// Optional set of labels
using OptionalLabelPeakIndexMap = std::optional<LabelMap>;

/**
 * Create an optional label set for filtering.
 * @param dimensionality : Dimensionality of the workspace.
 * @param emptyLabelId : Label id corresponding to empty.
 * @param filterWorkspace : Peaks workspace to act as filter.
 * @param clusterImage : Image
 * @return (optional) Map of labels to inspect for to the Peaks Index in the
 * peaks workspace which matches the cluster id.
 */
OptionalLabelPeakIndexMap createOptionalLabelFilter(size_t dimensionality, int emptyLabelId,
                                                    const IPeaksWorkspace_sptr &filterWorkspace,
                                                    IMDHistoWorkspace_sptr &clusterImage) {
  OptionalLabelPeakIndexMap optionalAllowedLabels;

  if (filterWorkspace) {
    if (dimensionality < 3) {
      throw std::invalid_argument("A FilterWorkspace has been given, but the "
                                  "dimensionality of the labeled workspace is "
                                  "< 3.");
    }
    LabelMap allowedLabels;
    PeakClusterProjection projection(clusterImage);

    for (int i = 0; i < filterWorkspace->getNumberPeaks(); ++i) {
      Mantid::Geometry::IPeak &peak = filterWorkspace->getPeak(i);
      const auto labelIdAtPeakCenter = static_cast<int>(projection.signalAtPeakCenter(peak));
      if (labelIdAtPeakCenter > emptyLabelId) {
        allowedLabels.emplace(labelIdAtPeakCenter, i);
      }
    }
    optionalAllowedLabels = allowedLabels;
  }
  return optionalAllowedLabels;
}

/**
Type to represent cluster face (a.k.a a row in the output table)
*/
struct ClusterFace {
  int clusterId;
  size_t workspaceIndex;
  int faceNormalDimension;
  bool maxEdge;
  double radius;
};

using ClusterFaces = std::deque<ClusterFace>;
using VecClusterFaces = std::vector<ClusterFaces>;

/**
Check that the data point signal value is an integer.
@param linearIndex : MDHistoWorkspace linear index
@param signalValue : signalValue at linearIndex
@throws runtime_error if signalValue is not an integer
*/
void checkDataPoint(const size_t &linearIndex, const double signalValue) {
  double intPart;

  // Check that the signal value looks like a label id.
  if (std::modf(signalValue, &intPart) != 0.0) {
    std::stringstream buffer;
    buffer << "Problem at linear index: " << linearIndex << " SignalValue is not an integer: " << signalValue
           << " Suggests wrong input IMDHistoWorkspace passed to "
              "FindClusterFaces.";

    throw std::runtime_error(buffer.str());
  }
}

/**
Find faces at the workspace index and write them to the localClusterFaces
container
@param linearIndex : IMDHistoWorkspace linear index
@param mdIterator : workspace iterator
@param clusterImage : IMDHistoWorkspace image
@param radius : radius from peak centre
@param id : label id
@param emptyLabelId : definition of empty label id
@param imageShape : shape of IMDHistoWorkspace
@param localClusterFaces : collection of cluster faces to add faces to
(writable)
*/
void findFacesAtIndex(const size_t linearIndex, IMDIterator *mdIterator, IMDHistoWorkspace_sptr &clusterImage,
                      const double &radius, const int &id, const int &emptyLabelId,
                      const std::vector<size_t> &imageShape, ClusterFaces &localClusterFaces) {
  auto indexes = Mantid::Kernel::Utils::getIndicesFromLinearIndex(linearIndex, imageShape);

  const auto neighbours = mdIterator->findNeighbourIndexesFaceTouching();
  for (auto neighbourLinearIndex : neighbours) {
    const auto neighbourId = static_cast<int>(clusterImage->getSignalAt(neighbourLinearIndex));

    if (neighbourId <= emptyLabelId) {
      // We have an edge!

      // In which dimension is the edge?
      auto neighbourIndexes = Mantid::Kernel::Utils::getIndicesFromLinearIndex(neighbourLinearIndex, imageShape);
      for (size_t j = 0; j < imageShape.size(); ++j) {
        if (indexes[j] != neighbourIndexes[j]) {
          const bool maxEdge = neighbourLinearIndex > linearIndex;

          ClusterFace face;
          face.clusterId = id;
          face.workspaceIndex = linearIndex;
          face.faceNormalDimension = static_cast<int>(j);
          face.maxEdge = maxEdge;
          face.radius = radius;

          localClusterFaces.emplace_back(face);
        }
      }
    }
  }
}

/**
Process without peak filtering

@param mdIterator : workspace iterator
@param localClusterFaces : collection of cluster faces to add faces to
(writable)
@param progress : progress reporting object
@param clusterImage : IMDHistoWorkspace image
@param imageShape : shape of IMDHistoWorkspace

*/
void executeUnFiltered(IMDIterator *mdIterator, ClusterFaces &localClusterFaces, Progress &progress,
                       IMDHistoWorkspace_sptr &clusterImage, const std::vector<size_t> &imageShape) {
  const int emptyLabelId = 0;
  const double radius = -1;
  do {
    const Mantid::signal_t signalValue = mdIterator->getSignal();
    const auto id = static_cast<int>(signalValue);

    if (id > emptyLabelId) {
      const size_t linearIndex = mdIterator->getLinearIndex();

      // Sanity check the signal value.
      checkDataPoint(linearIndex, signalValue);

      progress.report();

      // Find faces
      findFacesAtIndex(linearIndex, mdIterator, clusterImage, radius, id, emptyLabelId, imageShape, localClusterFaces);
    }
  } while (mdIterator->next());
}

/**
Process with peak filtering

@param mdIterator : workspace iterator
@param localClusterFaces : collection of cluster faces to add faces to
(writable)
@param progress : progress reporting object
@param clusterImage : IMDHistoWorkspace image
@param imageShape : shape of IMDHistoWorkspace
@param filterWorkspace : Peaks workspace to use as a filter
@param optionalAllowedLabels : Labels to consider (ignoring any others) in
processing.

*/
void executeFiltered(IMDIterator *mdIterator, ClusterFaces &localClusterFaces, Progress &progress,
                     IMDHistoWorkspace_sptr &clusterImage, const std::vector<size_t> &imageShape,
                     PeaksWorkspace_sptr &filterWorkspace, const OptionalLabelPeakIndexMap &optionalAllowedLabels) {
  const int emptyLabelId = 0;
  PeakClusterProjection projection(clusterImage);
  do {
    const Mantid::signal_t signalValue = mdIterator->getSignal();
    const auto id = static_cast<int>(signalValue);

    auto it = optionalAllowedLabels->find(id);
    if (it != optionalAllowedLabels->end()) {

      if (id > emptyLabelId) {

        const size_t linearIndex = mdIterator->getLinearIndex();

        // Sanity check data.
        checkDataPoint(linearIndex, signalValue);

        // Find the peak center
        const int &peakIndex = it->second;
        const IPeak &peak = filterWorkspace->getPeak(peakIndex);
        V3D peakCenter = projection.peakCenter(peak);

        // Calculate the radius
        VMD positionND = clusterImage->getCenter(mdIterator->getLinearIndex());
        V3D cellPosition(positionND[0], positionND[1], positionND[2]);
        double radius = cellPosition.distance(peakCenter);

        progress.report();

        // Find faces
        findFacesAtIndex(linearIndex, mdIterator, clusterImage, radius, id, emptyLabelId, imageShape,
                         localClusterFaces);
      }
    }

  } while (mdIterator->next());
}
} // namespace

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindClusterFaces)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string FindClusterFaces::name() const { return "FindClusterFaces"; }

/// Algorithm's version for identification. @see Algorithm::version
int FindClusterFaces::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FindClusterFaces::category() const { return "Crystal\\Integration"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void FindClusterFaces::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input image workspace consisting of cluster ids.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("FilterWorkspace", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "Optional filtering peaks workspace. Used to restrict face finding to "
                  "clusters in image which correspond to peaks in the workspace.");

  declareProperty("LimitRows", true, "Limit the report output to a maximum number of rows");

  declareProperty(std::make_unique<PropertyWithValue<int>>("MaximumRows", 100000,
                                                           std::make_shared<BoundedValidator<int>>(), Direction::Input),
                  "The number of neighbours to utilise. Defaults to 100000.");
  setPropertySettings("MaximumRows", std::make_unique<EnabledWhenProperty>("LimitRows", IS_DEFAULT));

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output table workspace containing cluster face information.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("TruncatedOutput", false, Direction::Output),
                  "Indicates that the output results were truncated if True");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void FindClusterFaces::exec() {
  IMDHistoWorkspace_sptr clusterImage = getProperty("InputWorkspace");
  const int emptyLabelId = 0;

  std::vector<size_t> imageShape;
  const size_t dimensionality = clusterImage->getNumDims();
  for (size_t i = 0; i < dimensionality; ++i) {
    imageShape.emplace_back(clusterImage->getDimension(i)->getNBins());
  }

  // Get the peaks workspace
  PeaksWorkspace_sptr filterWorkspace = this->getProperty("FilterWorkspace");

  // Use the peaks workspace to filter to labels of interest
  OptionalLabelPeakIndexMap optionalAllowedLabels =
      createOptionalLabelFilter(dimensionality, emptyLabelId, filterWorkspace, clusterImage);

  const int nThreads = Mantid::API::FrameworkManager::Instance().getNumOMPThreads(); // NThreads to Request
  auto mdIterators = clusterImage->createIterators(nThreads);                        // Iterators
  const auto nIterators = static_cast<int>(mdIterators.size());                      // Number of iterators yielded.
  VecClusterFaces clusterFaces(nIterators);
  size_t nSteps = 1000;
  if (optionalAllowedLabels.has_value()) {
    nSteps = optionalAllowedLabels->size();
  }
  const bool usingFiltering = optionalAllowedLabels.has_value();

  Progress progress(this, 0.0, 1.0, nSteps);
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int it = 0; it < nIterators; ++it) {
    PARALLEL_START_INTERRUPT_REGION
    ClusterFaces &localClusterFaces = clusterFaces[it];
    auto mdIterator = mdIterators[it].get();

    if (usingFiltering) {
      executeFiltered(mdIterator, localClusterFaces, progress, clusterImage, imageShape, filterWorkspace,
                      optionalAllowedLabels);
    } else {
      executeUnFiltered(mdIterator, localClusterFaces, progress, clusterImage, imageShape);
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  const bool limitRows = getProperty("LimitRows");
  const int maxRows = getProperty("MaximumRows");

  // Create an output table workspace now that all local cluster faces have been
  // found in parallel.
  auto out = WorkspaceFactory::Instance().createTable("TableWorkspace");
  out->addColumn("int", "ClusterId");
  out->addColumn("double", "MDWorkspaceIndex");
  out->addColumn("int", "FaceNormalDimension");
  out->addColumn("bool", "MaxEdge");
  out->addColumn("double", "Radius");
  size_t totalFaces = 0;
  for (int i = 0; i < nIterators; ++i) {
    const ClusterFaces &localClusterFaces = clusterFaces[i];

    for (const auto &clusterFace : localClusterFaces) {
      if (!limitRows || (out->rowCount() < size_t(maxRows))) {
        TableRow row = out->appendRow();
        row << clusterFace.clusterId << double(clusterFace.workspaceIndex) << clusterFace.faceNormalDimension
            << clusterFace.maxEdge << clusterFace.radius;
      }
      ++totalFaces;
    }
  }

  bool truncatedOutput = false;
  if (limitRows && out->rowCount() == size_t(maxRows)) {
    truncatedOutput = true;
    std::stringstream buffer;
    buffer << "More faces found than can be reported given the MaximumRows "
              "limit. Row limit at: "
           << maxRows << " Total faces available: " << totalFaces;
    g_log.warning(buffer.str());
  }

  setProperty("OutputWorkspace", out);
  setProperty("TruncatedOutput", truncatedOutput);
}

} // namespace Mantid::Crystal
