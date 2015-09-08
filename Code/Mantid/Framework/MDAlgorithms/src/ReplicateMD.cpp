#include "MantidMDAlgorithms/ReplicateMD.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <utility>
#include <sstream>
#include <string>
#include <memory>

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

size_t findStepSize(const IMDHistoWorkspace &shapeWS,
                    const IMDHistoWorkspace &dataWS) {

  size_t stepSize = 1;
  for (size_t i = 0; i < shapeWS.getNumDims(); ++i) {
    const auto shapeDim = shapeWS.getDimension(i);

    const auto dataDim = findMatchingDimension(dataWS, *shapeDim);
    if (!dataDim || dataDim->getIsIntegrated()) {
      // We've found the index of the dimension in shape not in data
      break;
    } else {
      stepSize *= shapeDim->getNBins();
    }

    if (i + 1 == shapeWS.getNumDims()) {
      throw std::invalid_argument("No unique dimenions in the shape!");
    }
  }
  return stepSize;
}

std::vector<int> findAxes(const IMDHistoWorkspace &shapeWS,
                          const IMDHistoWorkspace &dataWS) {

  std::vector<int> axes;
  for (size_t i = 0; i < dataWS.getNumDims(); ++i) {
    const auto dataDim = dataWS.getDimension(i);
    if (!dataDim->getIsIntegrated()) {
      int index = shapeWS.getDimensionIndexById(dataDim->getDimensionId());
      if (index >= dataWS.getNumDims()) {
        throw std::invalid_argument("Input data workspace cannot be rotated.");
      }
      axes.push_back(index);
    }
  }
  return axes;
}

size_t indexInData(const size_t &linearIndexShape, const size_t &dataSize) {
  return linearIndexShape - size_t(linearIndexShape / dataSize) * dataSize;
}

std::vector<size_t> resolveIndexes(const size_t &linearIndexShape,
                                   const IMDHistoWorkspace &shapeWS) {

  std::vector<size_t> shape;
  for (size_t i = 0; i < shapeWS.getNumDims(); ++i) {
    shape.push_back(shapeWS.getDimension(i)->getNBins());
  }

  size_t remainder = linearIndexShape;
  size_t currentShape = shapeWS.getNPoints();
  std::vector<size_t> result;
  for (auto it = shape.rbegin(); it != shape.rend(); ++it) {
    currentShape = currentShape / *it;
    size_t index_d = size_t(remainder / currentShape);
    remainder = remainder - (index_d * currentShape);
    result.push_back(index_d);
  }
  std::reverse(result.begin(), result.end());
  return result;
}
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReplicateMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReplicateMD::ReplicateMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReplicateMD::~ReplicateMD() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ReplicateMD::name() const { return "ReplicateMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReplicateMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReplicateMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReplicateMD::summary() const {
  return "This is a algorithm to create a higher dimensional dataset by "
         "replicating along an additional axis";
}

IMDHistoWorkspace_const_sptr
ReplicateMD::transposeMD(IMDHistoWorkspace_sptr &toTranspose,
                         const std::vector<int> &axes) {

  auto transposeMD = this->createChildAlgorithm("TransposeMD");
  transposeMD->setProperty("InputWorkspace", toTranspose);
  transposeMD->setProperty("Axes", axes);
  transposeMD->execute();
  IMDHistoWorkspace_sptr outputWS = transposeMD->getProperty("OutputWorkspace");
  return outputWS;
}

std::map<std::string, std::string> ReplicateMD::validateInputs() {
  std::map<std::string, std::string> errorMap;
  IMDHistoWorkspace_sptr shapeWS = this->getProperty("ShapeWorkspace");
  IMDHistoWorkspace_sptr dataWS = this->getProperty("DataWorkspace");
  if (shapeWS->getNonIntegratedDimensions().size() !=
      dataWS->getNonIntegratedDimensions().size() + 1) {
    errorMap.insert(std::make_pair(
        "DataWorkspace",
        "Expect to have n-1 non-interated dimensions of ShapeWorkspace"));
  }

  size_t nonMatchingCount = 0;
  for (size_t i = 0; i < shapeWS->getNumDims(); ++i) {
    const auto shapeDim = shapeWS->getDimension(i);

    // Attempt to match dimensions in both input workspaces based on Ids.
    const auto dataDim = findMatchingDimension(*dataWS, *shapeDim);
    if (dataDim) {
      if (dataDim->getIsIntegrated()) {
        // We count this as a non-matching dimension
        ++nonMatchingCount;
      } else {
        // Check bin sizes match between the two dimensions
        if (shapeDim->getNBins() != dataDim->getNBins()) {
          std::stringstream stream;
          stream << "Dimension with id " << shapeDim->getDimensionId()
                 << "in ShapeWorkspace has a different number of bins as the "
                    "same id dimension in the DataWorkspace";
          errorMap.insert(std::make_pair("DataWorkspace", stream.str()));
        }
      }
    } else {
      ++nonMatchingCount;
    }
  }

  // Check number of missing/integrated dimensions
  if (nonMatchingCount != 1) {
    errorMap.insert(std::make_pair("DataWorkspace",
                                   "There should be ONLY 1 dimension present "
                                   "in the ShapeWorkspace that is not present "
                                   "(or integrated out) in the DataWorkspace"));
  }

  return errorMap;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ReplicateMD::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("ShapeWorkspace", "",
                                                           Direction::Input),
                  "An input workspace defining the shape of the output.");
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("DataWorkspace", "",
                                                           Direction::Input),
                  "An input workspace containing the data to replicate.");
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace with replicated data.");
}

/*
 *def resolve_index(index, data):

    len = data.size
    newshape = list(reversed(data.shape))
    remainder = index
    current_shape = len
    result = list()
    for dim in newshape:
        current_shape = current_shape / dim
        index_d = int( remainder / current_shape)
        remainder = remainder - (index_d * current_shape)
        result.append( index_d )
    return list(reversed(result))




x = np.arange(3*4*5).reshape(3,4,5)
index = 4
integrated_dim = 0
y = resolve_index(index, x)
print y
del y[integrated_dim]

z = np.arange(4*5).reshape(4,5)

shape_prod = 1
lin_index = 0
for i in range(0, len(y)):
    lin_index += y[i] * shape_prod
    shape_prod = z.shape[i] * shape_prod

print lin_index

 */

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ReplicateMD::exec() {
  MDHistoWorkspace_sptr shapeWS;
  {
    IMDHistoWorkspace_sptr temp = this->getProperty("ShapeWorkspace");
    shapeWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(temp);
  }
  IMDHistoWorkspace_sptr dataWS = this->getProperty("DataWorkspace");

  IMDHistoWorkspace_const_sptr transposedDataWS = dataWS;
  if (dataWS->getNumDims() == shapeWS->getNumDims()) {
    auto axes = findAxes(*shapeWS, *dataWS);
    transposedDataWS = transposeMD(dataWS, axes);
  }

  size_t shapeReplicationIndex = findReplicationDimension(*shapeWS, *dataWS);

  // const size_t dataSize = size_t(transposedDataWS->getNPoints());
  // const size_t dataSize = findStepSize(*shapeWS, *dataWS);

  MDHistoWorkspace_sptr outputWS = shapeWS->clone();
  auto outIt = std::unique_ptr<MDHistoWorkspaceIterator>(
      dynamic_cast<MDHistoWorkspaceIterator *>(outputWS->createIterator()));
  auto dataIt = std::unique_ptr<MDHistoWorkspaceIterator>(
      dynamic_cast<MDHistoWorkspaceIterator *>(
          transposedDataWS->createIterator()));
  do {

    const auto sourceIndex = outIt->getLinearIndex();
    std::vector<size_t> vecShapeIndexes = resolveIndexes(sourceIndex, *shapeWS);

    vecShapeIndexes.erase(vecShapeIndexes.begin() + shapeReplicationIndex);

    size_t shape_prod = 1;
    size_t lin_index = 0;
    for (size_t i = 0; i < vecShapeIndexes.size(); ++i) {
      lin_index += vecShapeIndexes[i] * shape_prod;
      shape_prod = dataWS->getDimension(i)->getNBins() * shape_prod;
    }

    const auto targetIndex = lin_index;

    // const auto targetIndex = indexInData(sourceIndex, shapeWS);
    dataIt->jumpTo(targetIndex);
    outputWS->setSignalAt(sourceIndex, dataIt->getSignal());
    outputWS->setErrorSquaredAt(sourceIndex,
                                dataIt->getError() * dataIt->getError());
    outputWS->setNumEventsAt(sourceIndex, dataIt->getNumEvents());
    outputWS->setMDMaskAt(sourceIndex, dataIt->getIsMasked());

  } while (outIt->next());

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
