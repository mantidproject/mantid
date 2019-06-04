// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/TransposeMD.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MultiThreaded.h"

#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <numeric>
#include <vector>

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(TransposeMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string TransposeMD::name() const { return "TransposeMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int TransposeMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string TransposeMD::category() const {
  return "MDAlgorithms\\Transforms";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string TransposeMD::summary() const {
  return "Transpose the dimensions of a MDWorkspace to create a new output "
         "MDWorkspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void TransposeMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace.");

  auto axisValidator = boost::make_shared<ArrayBoundedValidator<int>>();
  axisValidator->setLower(0);

  declareProperty(
      std::make_unique<ArrayProperty<int>>("Axes", std::vector<int>(0),
                                              axisValidator, Direction::Input),
      "Permutes the axes according to the indexes given. Zero "
      "based indexing. Defaults to no transpose.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
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
    Kernel::MDAxisValidator checker(axesInts, nDimsInput, false);
    auto axisErrors = checker.validate();
    if (!axisErrors.empty()) {
      std::string error = axisErrors.begin()->second;
      throw std::invalid_argument(error.c_str());
    }
    nDimsOutput = axes.size();
  } else {
    axes = std::vector<size_t>(nDimsOutput);
    std::iota(axes.begin(), axes.end(), 0);
  }

  std::vector<coord_t> origin(nDimsOutput, 0.0);
  std::vector<Geometry::IMDDimension_sptr> targetGeometry;
  for (size_t i = 0; i < nDimsOutput; ++i) {
    // Clone the dimension corresponding to the axis requested.
    auto cloneDim = Geometry::IMDDimension_sptr(
        new Geometry::MDHistoDimension(inWS->getDimension(axes[i]).get()));
    targetGeometry.push_back(cloneDim);
  }

  // Make the output workspace in the right shape.
  auto outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(targetGeometry));
  outWS->copyExperimentInfos(*inWS);

  // Configure the coordinate transform.
  std::vector<coord_t> scaling(nDimsOutput, 1); // No scaling
  CoordTransformAligned coordTransform(nDimsInput, nDimsOutput, axes, origin,
                                       scaling);

  uint64_t nPoints = inWS->getNPoints();
  Progress progress(this, 0.0, 1.0, size_t(nPoints));

  progress.reportIncrement(
      size_t(double(nPoints) * 0.1)); // Report ~10% progress

  const int nThreads = Mantid::API::FrameworkManager::Instance()
                           .getNumOMPThreads(); // NThreads to Request

  auto iterators = inWS->createIterators(nThreads, nullptr);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int it = 0; it < int(iterators.size()); ++it) { // NOLINT

    PARALLEL_START_INTERUPT_REGION
    auto inIterator = iterators[it].get();
    do {
      auto center = inIterator->getCenter();
      const coord_t *incoords = center.getBareArray();
      std::vector<coord_t> outcoords(nDimsOutput);
      coordTransform.apply(incoords, &outcoords[0]);

      size_t index = outWS->getLinearIndexAtCoord(&outcoords[0]);
      outWS->setSignalAt(index, inIterator->getSignal());
      const signal_t error = inIterator->getError();
      outWS->setErrorSquaredAt(index, error * error);
      outWS->setNumEventsAt(index,
                            Mantid::signal_t(inIterator->getNumEvents()));
      outWS->setMDMaskAt(index, inIterator->getIsMasked());
      progress.report();
    } while (inIterator->next());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  this->setProperty("OutputWorkspace", outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
