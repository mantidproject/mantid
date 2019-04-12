// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ReplicateMD.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Utils.h"
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace {

/**
 * findMatchingDimension, find dimension in search workspace.
 * Returns null shared ptr if nothing found.
 * @param toSearch : Workspace to search
 * @param forDim : Dimension to search for
 * @return found dimension or a null shared ptr of type IMDDimension
 */
IMDDimension_const_sptr findMatchingDimension(const IMDHistoWorkspace &toSearch,
                                              const IMDDimension &forDim) {
  IMDDimension_const_sptr foundDim;
  try {
    // This will throw if it doesn't exist.
    foundDim = toSearch.getDimensionWithId(forDim.getDimensionId());
  } catch (std::invalid_argument &) {
    // Do nothing.
  }
  return foundDim;
}

/**
 * Find the index of the dimension to be replicated. This is any dimension
 * in the shape workspace that does not appear in the data workspace (there
 * should be only 1)
 * or any integrated dimension in the data workspace.
 * @param shapeWS : workspace with complete target dimension set
 * @param dataWS : source of the missing/integrated dimension
 * @return Index in the SHAPE workspace
 */
size_t findReplicationDimension(const IMDHistoWorkspace &shapeWS,
                                const IMDHistoWorkspace &dataWS) {
  size_t index = -1;
  for (size_t i = 0; i < shapeWS.getNumDims(); ++i) {
    const auto shapeDim = shapeWS.getDimension(i);

    const auto dataDim = findMatchingDimension(dataWS, *shapeDim);
    if (!dataDim || dataDim->getIsIntegrated()) {
      index = i;
      break;
    }
  }
  return index;
}

/**
 * Determine the axis to use for a transpose operation.
 * @param shapeWS : Workspace to tranpose into the coordinate system of
 * @param dataWS : Workspace to transpose
 * @return Axis required for the transpose command.
 */
std::vector<int> findAxes(const IMDHistoWorkspace &shapeWS,
                          const IMDHistoWorkspace &dataWS) {

  std::vector<int> axes;
  for (size_t i = 0; i < dataWS.getNumDims(); ++i) {
    const auto dataDim = dataWS.getDimension(i);
    if (!dataDim->getIsIntegrated()) {
      int index = static_cast<int>(
          shapeWS.getDimensionIndexById(dataDim->getDimensionId()));
      axes.push_back(index);
    }
  }
  return axes;
}

/**
 * Convert a linear index in the shape to a linear index in the DataItem
 *
 * This is a flattening procedure.
 *
 * 1) Convert linear index in shape to an n-dimensional set of indexes
 * 2) Remove the index corresponding to the missing dimension (we're going to
 *replicate over this)
 * 3) Using the n-dimensional indexes, create a linear index in the data
 *
 * @param nDimsShape : Number of dimensions in the shape
 * @param shapeReplicationIndex : Index to replicate over
 * @param indexMaxShape : Number of bins for each dim in the shape
 * @param indexMakerShape : Index maker for the shape
 * @param sourceIndex : Linear index in the shape
 * @param indexMakerData : Index maker for the data
 * @param nDimsData : Number of dimensions in the data
 * @return Linear index in the data
 */
size_t linearIndexToLinearIndex(const size_t &nDimsShape,
                                const size_t &shapeReplicationIndex,
                                const std::vector<size_t> &indexMaxShape,
                                const std::vector<size_t> &indexMakerShape,
                                const size_t &sourceIndex,
                                std::vector<size_t> &indexMakerData,
                                const size_t &nDimsData) {
  std::vector<size_t> vecShapeIndexes(nDimsShape);
  Utils::NestedForLoop::GetIndicesFromLinearIndex(
      nDimsShape, sourceIndex, &indexMakerShape[0], &indexMaxShape[0],
      &vecShapeIndexes[0]);

  vecShapeIndexes.erase(vecShapeIndexes.begin() + shapeReplicationIndex);

  const size_t targetIndex = Utils::NestedForLoop::GetLinearIndex(
      nDimsData, &vecShapeIndexes[0], &indexMakerData[0]);

  return targetIndex;
}
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReplicateMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ReplicateMD::name() const { return "ReplicateMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReplicateMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReplicateMD::category() const {
  return "MDAlgorithms\\Creation";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReplicateMD::summary() const {
  return "This is an algorithm to create a higher dimensional dataset by "
         "replicating along an additional axis";
}

/**
 * Transpose the input data workspace according to the axis provided.
 * @param toTranspose Workspace to transpose
 * @param axes : target axes indexes
 * @return : Transposed workspace.
 */
MDHistoWorkspace_const_sptr
ReplicateMD::transposeMD(MDHistoWorkspace_sptr &toTranspose,
                         const std::vector<int> &axes) {

  auto transposeMD = this->createChildAlgorithm("TransposeMD", 0.0, 0.5);
  transposeMD->setProperty("InputWorkspace", toTranspose);
  transposeMD->setProperty("Axes", axes);
  transposeMD->execute();
  IMDHistoWorkspace_sptr outputWS = transposeMD->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<const MDHistoWorkspace>(outputWS);
}

/**
 * Overriden validate inputs
 *
 * @return map of propery name to problem description for any issues
 */
std::map<std::string, std::string> ReplicateMD::validateInputs() {
  std::map<std::string, std::string> errorMap;
  IMDHistoWorkspace_sptr shapeWS = this->getProperty("ShapeWorkspace");
  IMDHistoWorkspace_sptr dataWS = this->getProperty("DataWorkspace");
  if (shapeWS->getNonIntegratedDimensions().size() !=
      dataWS->getNonIntegratedDimensions().size() + 1) {
    errorMap.emplace(
        "DataWorkspace",
        "Expect to have n-1 non-interated dimensions of ShapeWorkspace");
  }

  size_t nonMatchingCount = 0;
  bool haveMatchingIntegratedDims = false;
  for (size_t i = 0; i < shapeWS->getNumDims(); ++i) {
    const auto shapeDim = shapeWS->getDimension(i);

    // Attempt to match dimensions in both input workspaces based on Ids.
    const auto dataDim = findMatchingDimension(*dataWS, *shapeDim);
    if (dataDim) {
      if (dataDim->getIsIntegrated()) {
        if (!shapeDim->getIsIntegrated()) {
          // We count this as a non-matching dimension
          ++nonMatchingCount;
        } else {
          haveMatchingIntegratedDims = true;
        }
      } else {
        // Check bin sizes match between the two dimensions
        if (shapeDim->getNBins() != dataDim->getNBins()) {
          std::stringstream stream;
          stream << "Dimension with id " << shapeDim->getDimensionId()
                 << " in ShapeWorkspace has a different number of bins as the "
                    "same id dimension in the DataWorkspace";
          errorMap.emplace("DataWorkspace", stream.str());
        } else if (haveMatchingIntegratedDims) {
          errorMap.emplace(
              "ShapeWorkspace",
              "Extra integrated dimensions must be only "
              "the last dimensions, e.g.:\n\nThis is allowed:\n  "
              "Shape: {10, 5, 1, 1}\n  Data:  { 1, 5, 1, 1}\n\nBut "
              "this is not:\n  Shape: {10, 1, 5, 1}\n  Data:  { 1, 1, "
              "5, 1}\n\nUse TransposeMD to re-arrange dimensions.");
          break;
        }
      }
    } else {
      ++nonMatchingCount;
    }
  }

  // Check number of missing/integrated dimensions
  if (nonMatchingCount != 1) {
    errorMap.emplace("DataWorkspace",
                     "There should be ONLY 1 dimension present "
                     "in the ShapeWorkspace that is not present "
                     "(or integrated out) in the DataWorkspace");
  }

  return errorMap;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ReplicateMD::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "ShapeWorkspace", "", Direction::Input),
                  "An input workspace defining the shape of the output.");
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "DataWorkspace", "", Direction::Input),
                  "An input workspace containing the data to replicate.");
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace with replicated data.");
}

/**
 * fetches and down casts
 * @return ShapeWorkspace property
 */
MDHistoWorkspace_sptr ReplicateMD::getShapeWorkspace() const {
  MDHistoWorkspace_sptr shapeWS;
  {
    IMDHistoWorkspace_sptr temp = this->getProperty("ShapeWorkspace");
    shapeWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(temp);
  }

  return shapeWS;
}

/**
 * fetches and down casts
 * @return DataWorkspace property
 */
MDHistoWorkspace_sptr ReplicateMD::getDataWorkspace() const {
  MDHistoWorkspace_sptr dataWS;
  {
    IMDHistoWorkspace_sptr temp = this->getProperty("DataWorkspace");
    dataWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(temp);
  }

  return dataWS;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ReplicateMD::exec() {

  MDHistoWorkspace_sptr shapeWS = getShapeWorkspace();
  MDHistoWorkspace_sptr dataWS = getDataWorkspace();

  const size_t nDimsShape = shapeWS->getNumDims();
  size_t nDimsData = dataWS->getNumDims();
  Progress progress(this, 0.0, 1.0, size_t(shapeWS->getNPoints()));

  /*
   We transpose the input so that we have the same order of dimensions in data
   as in the shape. This is important. If they are not in the same order, then
   the linear index -> linear index calculation below will not work correctly.
   */
  MDHistoWorkspace_const_sptr transposedDataWS = dataWS;
  if (nDimsData <= nDimsShape) {
    auto axes = findAxes(*shapeWS, *dataWS);
    // Check that the indices stored in axes are compatible with the
    // dimensionality of the data workspace
    const auto numberOfDimensionsOfDataWorkspace = static_cast<int>(nDimsData);
    const auto found =
        std::find_if(axes.cbegin(), axes.cend(),
                     [numberOfDimensionsOfDataWorkspace](const auto &axis) {
                       return axis >= numberOfDimensionsOfDataWorkspace;
                     });
    if (found != axes.cend()) {
      std::string message =
          "ReplicateMD: Cannot transpose the data workspace. Attempting to "
          "swap dimension index " +
          std::to_string(
              std::distance(static_cast<const int *>(&axes[0]), &(*found))) +
          " with index " + std::to_string(*found) +
          ", but the dimensionality of the data workspace is " +
          std::to_string(nDimsData);
      throw std::runtime_error(message);
    }
    transposedDataWS = transposeMD(dataWS, axes);
    nDimsData = transposedDataWS->getNumDims();
  }

  // Set up index maker for shape
  std::vector<size_t> indexMakerShape(nDimsShape);
  std::vector<size_t> indexMaxShape(nDimsShape);
  for (size_t i = 0; i < nDimsShape; ++i) {
    indexMaxShape[i] = shapeWS->getDimension(i)->getNBins();
  }
  Utils::NestedForLoop::SetUpIndexMaker(nDimsShape, &indexMakerShape[0],
                                        &indexMaxShape[0]);

  // Set up index maker for data.
  std::vector<size_t> indexMakerData(nDimsData);
  std::vector<size_t> indexMaxData(nDimsData);
  for (size_t i = 0; i < nDimsData; ++i) {
    indexMaxData[i] = transposedDataWS->getDimension(i)->getNBins();
  }
  Utils::NestedForLoop::SetUpIndexMaker(nDimsData, &indexMakerData[0],
                                        &indexMaxData[0]);

  // Dimension index into the shape workspace where we will be replicating
  const size_t shapeReplicationIndex =
      findReplicationDimension(*shapeWS, *transposedDataWS);

  // Create the output workspace from the shape.
  MDHistoWorkspace_sptr outputWS(shapeWS->clone());
  const int nThreads = Mantid::API::FrameworkManager::Instance()
                           .getNumOMPThreads(); // NThreads to Request

  // collection of iterators
  auto iterators = outputWS->createIterators(nThreads, nullptr);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int it = 0; it < int(iterators.size()); ++it) { // NOLINT

    PARALLEL_START_INTERUPT_REGION
    auto outIt = dynamic_cast<MDHistoWorkspaceIterator *>(iterators[it].get());

    // Iterate over the output workspace
    do {

      const auto sourceIndex = outIt->getLinearIndex();

      // Figure out the linear index in the data.
      const size_t targetIndex = linearIndexToLinearIndex(
          nDimsShape, shapeReplicationIndex, indexMaxShape, indexMakerShape,
          sourceIndex, indexMakerData, nDimsData);

      // Copy the data across
      outputWS->setSignalAt(sourceIndex,
                            transposedDataWS->getSignalAt(targetIndex));
      outputWS->setErrorSquaredAt(
          sourceIndex, transposedDataWS->getErrorAt(targetIndex) *
                           transposedDataWS->getErrorAt(targetIndex));
      outputWS->setNumEventsAt(sourceIndex,
                               transposedDataWS->getNumEventsAt(targetIndex));
      outputWS->setMDMaskAt(sourceIndex,
                            transposedDataWS->getIsMaskedAt(targetIndex));
      progress.report();

    } while (outIt->next());

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
