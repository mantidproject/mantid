#include "MantidMDAlgorithms/TransposeMD.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include <vector>
#include <algorithm>
#include <numeric>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(TransposeMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TransposeMD::TransposeMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
TransposeMD::~TransposeMD() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string TransposeMD::name() const { return "TransposeMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int TransposeMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string TransposeMD::category() const { return "MDAlgorithms"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string TransposeMD::summary() const {
  return "Transpose the dimensions of a MDWorkspace to create a new output "
         "MDWorkspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void TransposeMD::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input workspace.");

  auto axisValidator = boost::make_shared<ArrayBoundedValidator<int>>();
  axisValidator->clearUpper();
  axisValidator->setLower(0);

  declareProperty(new ArrayProperty<int>("Axes", std::vector<int>(0),
                                         axisValidator, Direction::Input),
                  "Permutes the axes according to the indexes given. Zero "
                  "based indexing. Defaults to no transpose.");

  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void TransposeMD::exec() {
  IMDHistoWorkspace_sptr inWSProp = getProperty("InputWorkspace");
  auto inWS = boost::dynamic_pointer_cast<MDHistoWorkspace>(inWSProp);
  if (!inWS) {
    throw std::invalid_argument(
        "Expect the InputWorkspace to be a MDHistoWorkspace");
  }

  size_t nDimsInput = inWS->getNumDims();
  size_t nDimsOutput = inWS->getNumDims(); // The assumed default.
  std::vector<int> axesInts = this->getProperty("Axes");
  std::vector<size_t> axes(axesInts.begin(), axesInts.end());
  Property *axesProperty = this->getProperty("Axes");
  if (!axesProperty->isDefault()) {
    if (axes.size() > nDimsInput) {
      throw std::invalid_argument(
          "More axis specified than dimensions are avaiable in the input");
    }
    auto it = std::max_element(axes.begin(), axes.end());
    if (*it > nDimsInput) {
      throw std::invalid_argument("One of the axis indexes specified indexes a "
                                  "dimension outside the real dimension range");
    }
    nDimsOutput = axes.size();
  } else {
    axes = std::vector<size_t>(nDimsOutput);
    std::iota(axes.begin(), axes.end(), 0);
  }


  std::vector<coord_t> origin;
  std::vector<Geometry::IMDDimension_sptr> targetGeometry;
  for (size_t i = 0; i < nDimsOutput; ++i) {
    // Clone the dimension corresponding to the axis requested.
    auto cloneDim = Geometry::IMDDimension_sptr(
        new Geometry::MDHistoDimension(inWS->getDimension(axes[i]).get()));
    targetGeometry.push_back(cloneDim);
    // Set the same origin as we have on the input workspace
    origin.push_back(coord_t(cloneDim->getMinimum()));
  }

  // Make the output workspace in the right shape.
  auto outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(targetGeometry));

  // Configure the coordinate transform.
  std::vector<coord_t> scaling(nDimsOutput, 1); // No scaling
  CoordTransformAligned coordTransform(nDimsInput, nDimsOutput, axes, origin,
                                       scaling);

  IMDIterator * inIterator = inWS->createIterator();
  do{
      auto center = inIterator->getCenter();
      const coord_t* incoords = center.getBareArray();
      std::vector<coord_t> outcoords(nDimsOutput);
      coordTransform.apply(incoords, &outcoords[0]);

      size_t index = outWS->getLinearIndexAtCoord(&outcoords[0]);
      outWS->setSignalAt(index, inIterator->getSignal());
      outWS->setErrorSquaredAt(index, inIterator->getError()*inIterator->getError());
      // TODO more otherwise

  }while(inIterator->next());


  this->setProperty("OutputWorkspace",
                    outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
