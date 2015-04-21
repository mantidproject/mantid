#include "MantidMDAlgorithms/IntegrateMDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MultiThreaded.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Progress.h"

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"

#include <algorithm>
#include <map>
#include <utility>

#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace {

/**
 * Check for empty binning
 * @param binning : binning
 * @return : true if binning is empty
 */
bool emptyBinning(const std::vector<double> &binning) {
  return binning.empty();
}

/**
 * Determine whether the binning provided is any good.
 * @param binning : Binning property
 * @return : string containing error. Or empty for no error.
 */
std::string checkBinning(const std::vector<double> &binning) {
  std::string error; // No error.
  if (!emptyBinning(binning) && binning.size() != 2) {
    error = "You may only integrate out dimensions between limits.";
  } else if (binning.size() == 2) {
    auto min = binning[0];
    auto max = binning[1];
    if (min >= max) {
      error = "min must be < max limit for binning";
    }
  }
  return error;
}

/**
 * Create the output workspace in the right shape.
 * @param inWS : Input workspace for dimensionality
 * @param pbins : User provided binning
 * @return
 */
MDHistoWorkspace_sptr
createShapedOutput(IMDHistoWorkspace const *const inWS,
                   std::vector<std::vector<double>> pbins) {
  const size_t nDims = inWS->getNumDims();
  std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions(nDims);
  for (size_t i = 0; i < nDims; ++i) {

    IMDDimension_const_sptr inDim = inWS->getDimension(i);
    auto outDim = boost::make_shared<MDHistoDimension>(inDim.get());
    // Apply dimensions as inputs.
    if (i < pbins.size() && !emptyBinning(pbins[i])) {
      auto binning = pbins[i];
      outDim->setRange(
          1 /*single bin*/,
          static_cast<Mantid::coord_t>(binning.front()) /*min*/,
          static_cast<Mantid::coord_t>(
              binning.back()) /*max*/); // Set custom min, max and nbins.
    }
    dimensions[i] = outDim;
  }
  return MDHistoWorkspace_sptr(new MDHistoWorkspace(dimensions));
}

/**
 * Perform a weighted sum at the iterator position. This function does not increment the iterator.
 * @param iterator : Iterator to use in sum
 * @param box : Box implicit function defining valid region.
 * @param sumSignal : Accumlation in/out ref.
 * @param sumSQErrors : Accumulation error in/out ref. Squared value.
 */
void performWeightedSum(MDHistoWorkspaceIterator const * const iterator, MDBoxImplicitFunction& box, double& sumSignal, double& sumSQErrors) {
    const double weight = box.fraction(iterator->getBoxExtents());
    sumSignal += weight * iterator->getSignal();
    const double error = iterator->getError();
    sumSQErrors +=
        weight * (error * error);
}

}

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateMDHistoWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IntegrateMDHistoWorkspace::IntegrateMDHistoWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IntegrateMDHistoWorkspace::~IntegrateMDHistoWorkspace() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string IntegrateMDHistoWorkspace::name() const {
  return "IntegrateMDHistoWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateMDHistoWorkspace::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateMDHistoWorkspace::category() const {
  return "MDAlgorithms";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string IntegrateMDHistoWorkspace::summary() const {
  return "Performs axis aligned integration of MDHistoWorkspaces";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegrateMDHistoWorkspace::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input workspace.");

  const std::vector<double> defaultBinning;
  declareProperty(new ArrayProperty<double>("P1Bin", defaultBinning),
                  "Projection 1 binning.");
  declareProperty(new ArrayProperty<double>("P2Bin", defaultBinning),
                  "Projection 2 binning.");
  declareProperty(new ArrayProperty<double>("P3Bin", defaultBinning),
                  "Projection 3 binning.");
  declareProperty(new ArrayProperty<double>("P4Bin", defaultBinning),
                  "Projection 4 binning.");
  declareProperty(new ArrayProperty<double>("P5Bin", defaultBinning),
                  "Projection 5 binning.");

  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void IntegrateMDHistoWorkspace::exec() {
  IMDHistoWorkspace_sptr inWS = this->getProperty("InputWorkspace");
  const size_t nDims = inWS->getNumDims();
  std::vector<std::vector<double>> pbins(5);
  pbins[0] = this->getProperty("P1Bin");
  pbins[1] = this->getProperty("P2Bin");
  pbins[2] = this->getProperty("P3Bin");
  pbins[3] = this->getProperty("P4Bin");
  pbins[4] = this->getProperty("P5Bin");

  IMDHistoWorkspace_sptr outWS;
  const size_t emptyCount =
      std::count_if(pbins.begin(), pbins.end(), emptyBinning);
  if (emptyCount == pbins.size()) {
    // No work to do.
    g_log.information(this->name() + " Direct clone of input.");
    outWS = inWS->clone();
  } else {

    /* Create the output workspace in the right shape. This allows us to iterate over our output
       structure and fill it.
     */
    outWS = createShapedOutput(inWS.get(), pbins);

    Progress progress(this, 0, 1, size_t(outWS->getNPoints()));

    // Store in each dimension
    std::vector<Mantid::coord_t> binWidthsOut(nDims);
    std::vector<int> widthVector(nDims); // used for nearest neighbour search
    for (size_t i = 0; i < nDims; ++i) {
      binWidthsOut[i] = outWS->getDimension(i)->getBinWidth();

      // Maximum width vector for region in output workspace corresponding to region in input workspace.

      /* int(wout/win + 0.5) = n_pixels in input corresponding to 1 pixel in output. Rounded up.
         The width vector is the total width. So we always need to double it to take account of the whole region.
         For example, 8/4 + 0.5 = 2, but thats only 1 pixel on each side of the center, we need 2 * that to give the correct
         answer of 4.
      */
      widthVector[i] =  2 * int((binWidthsOut[i] / inWS->getDimension(i)->getBinWidth()) + 0.5); // round up.

      if(widthVector[i]%2 == 0) {
          widthVector[i]+= 1; // make it odd if not already.
      }
    }

    // Outer loop over the output workspace iterator poistions.
    const int nThreads = Mantid::API::FrameworkManager::Instance()
                             .getNumOMPThreads(); // NThreads to Request

    auto outIterators = outWS->createIterators(nThreads, NULL);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < int(outIterators.size()); ++i) {

    PARALLEL_START_INTERUPT_REGION
    boost::scoped_ptr<MDHistoWorkspaceIterator> outIterator(
                  dynamic_cast<MDHistoWorkspaceIterator *>(outIterators[i]));

    do {

      Mantid::Kernel::VMD outIteratorCenter = outIterator->getCenter();

      // Calculate the extents for this out iterator position.
      std::vector<Mantid::coord_t> mins(nDims);
      std::vector<Mantid::coord_t> maxs(nDims);
      for (size_t i = 0; i < nDims; ++i) {
        const coord_t delta = binWidthsOut[i] / 2;
        mins[i] = outIteratorCenter[i] - delta;
        maxs[i] = outIteratorCenter[i] + delta;
      }
      MDBoxImplicitFunction box(mins, maxs);

      double sumSignal = 0;
      double sumSQErrors = 0;

      // Create a thread-local input iterator.
      boost::scoped_ptr<MDHistoWorkspaceIterator> inIterator(
          dynamic_cast<MDHistoWorkspaceIterator *>(inWS->createIterator()));

      /*
      We jump to the iterator position which is closest in the model coordinates
      to the centre of our output iterator. This allows us to consider a much smaller region of space as part of our inner loop
      rather than iterating over the full set of boxes of the input workspace.
      */
      inIterator->jumpToNearest(outIteratorCenter);

      performWeightedSum(inIterator.get(), box, sumSignal, sumSQErrors); // Use the present position. neighbours below exclude the current position.

      // Look at all of the neighbours of our position. We previously calculated what the width vector would need to be.
      auto neighbourIndexes = inIterator->findNeighbourIndexesByWidth(widthVector);
      for (size_t i = 0; i < neighbourIndexes.size(); ++i) {
          inIterator->jumpTo(neighbourIndexes[i]); // Go to that neighbour
          performWeightedSum(inIterator.get(), box, sumSignal, sumSQErrors);
      }

      const size_t iteratorIndex = outIterator->getLinearIndex();
      outWS->setSignalAt(iteratorIndex, sumSignal);
      outWS->setErrorSquaredAt(iteratorIndex, sumSQErrors);

      progress.report();
    } while (outIterator->next());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  }

  this->setProperty("OutputWorkspace", outWS);
}

/**
 * Overriden validate inputs
 * @return map of property names to problems for bad inputs
 */
std::map<std::string, std::string>
Mantid::MDAlgorithms::IntegrateMDHistoWorkspace::validateInputs() {
  // Check binning parameters
  std::map<std::string, std::string> errors;

  for (int i = 1; i < 6; ++i) {
    std::stringstream propBuffer;
    propBuffer << "P" << i << "Bin";
    std::string propertyName = propBuffer.str();
    std::vector<double> binning = this->getProperty(propertyName);
    std::string result = checkBinning(binning);
    if (!result.empty()) {
      errors.insert(std::make_pair(propertyName, result));
    }
  }
  return errors;
}

} // namespace MDAlgorithms
} // namespace Mantid
