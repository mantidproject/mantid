// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegrateMDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MultiThreaded.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Progress.h"

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>

#include <boost/make_shared.hpp>

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
 * Check for integration binning
 * @param binning : binning
 * @return : true if binning is min, max integration style
 */
bool integrationBinning(const std::vector<double> &binning) {
  return binning.size() == 2 && binning[0] < binning[1];
}

/**
 * Check for similar binning
 * @param binning : binning
 * @return : true if binning similar, with limits but same bin width
 */
bool similarBinning(const std::vector<double> &binning) {
  return binning.size() == 3 && binning[1] == 0.0;
}

/**
 * Determine whether the binning provided is any good.
 * @param binning : Binning property
 * @return : string containing error. Or empty for no error.
 */
std::string checkBinning(const std::vector<double> &binning) {
  std::string error; // No error.
  if (!emptyBinning(binning)) {
    if (binning.size() == 3) {
      const double step = binning[1];
      if (step != 0) {
        error = "Only step size zero is allowed. Denotes copy of original step "
                "size for that dimension.";
      } else {
        auto min = binning[0];
        auto max = binning[2];
        if (min >= max) {
          error = "Min must be < max limit for binning";
        }
      }

    } else if (binning.size() == 2) {
      auto min = binning[0];
      auto max = binning[1];
      if (min >= max) {
        error = "Min must be < max limit for binning";
      }
    } else {
      error = "Unknown binning prameters for dimension.";
    }
  }
  return error;
}

/**
 * Provide a precision correction for Mantid coordinates
 * @param position: a position
 * @returns: a precision corrected position or the original position
 */
Mantid::coord_t getPrecisionCorrectedCoordinate(Mantid::coord_t position,
                                                Mantid::coord_t binWidth) {
  // Find the closest integer value
  const auto up = std::ceil(position);
  const auto down = std::floor(position);
  const auto diffUp = fabs(up - position);
  const auto diffDown = fabs(down - position);
  const auto nearest = diffUp < diffDown ? up : down;

  // Check if the relative deviation is larger than 1e-5
  const auto deviation = fabs((nearest - position) / binWidth);
  const auto tolerance = 1e-5;
  Mantid::coord_t coordinate(position);
  if (deviation < tolerance) {
    coordinate = nearest;
  }
  return coordinate;
}

/**
 * Sets the min, max and number of bins
 * @param pMin: set minimum value passed by reference
 * @param pMax: set maximum value passed by reference
 * @param numberOfBins: the number of bins passed by reference
 * @param dimension: the dimension information
 * @param logger: a logger object
 */
void setMinMaxBins(Mantid::coord_t &pMin, Mantid::coord_t &pMax,
                   size_t &numberOfBins,
                   const IMDDimension_const_sptr &dimension, Logger &logger) {
  // Get workspace extents
  const Mantid::coord_t width = dimension->getBinWidth();
  const Mantid::coord_t max = dimension->getMaximum();

  // Get offset between origin and next bin boundary towards the max value
  // NOTE: GCC shows a conversion warning from double to float here. This
  // is incorrect. Silence warning with explicit cast.
  const Mantid::coord_t offset = static_cast<Mantid::coord_t>(fmod(max, width));

  // Create the shifted pMax and pMin
  auto minBin = (pMin - offset) / width;
  auto maxBin = (pMax - offset) / width;

  // Make sure that we don't snap to the wrong value
  // because of the precision of floats (which coord_t is)
  minBin = getPrecisionCorrectedCoordinate(minBin, width);
  maxBin = getPrecisionCorrectedCoordinate(maxBin, width);
  auto snappedPMin = width * std::floor(minBin);
  auto snappedPMax = width * std::ceil(maxBin);

  // Shift the snappedPMax/snappedPMin values back
  snappedPMax += offset;
  snappedPMin += offset;

  if (snappedPMin < dimension->getMinimum()) {
    snappedPMin = dimension->getMinimum();
  } else if (pMin != snappedPMin) {
    std::stringstream buffer;
    buffer << "Rounding min from: " << pMin
           << " to the nearest whole width at: " << snappedPMin;
    logger.warning(buffer.str());
  }

  if (snappedPMax > dimension->getMaximum()) {
    snappedPMax = dimension->getMaximum();
  } else if (pMax != snappedPMax) {
    std::stringstream buffer;
    buffer << "Rounding max from: " << pMax
           << " to the nearest whole width at: " << snappedPMax;
    logger.warning(buffer.str());
  }

  pMin = snappedPMin;
  pMax = snappedPMax;

  // Bins
  numberOfBins =
      std::lround((pMax - pMin) / width); // round up to a whole number of bins.
}
} // namespace

/**
 * Create the output workspace in the right shape.
 * @param inWS : Input workspace for dimensionality
 * @param pbins : User provided binning
 * @param logger : Logging object
 * @return
 */
MDHistoWorkspace_sptr createShapedOutput(IMDHistoWorkspace const *const inWS,
                                         std::vector<std::vector<double>> pbins,
                                         Logger &logger) {
  const size_t nDims = inWS->getNumDims();
  std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions(nDims);
  for (size_t i = 0; i < nDims; ++i) {

    IMDDimension_const_sptr inDim = inWS->getDimension(i);
    auto outDim = boost::make_shared<MDHistoDimension>(inDim.get());
    // Apply dimensions as inputs.
    if (i < pbins.size() && integrationBinning(pbins[i])) {
      auto binning = pbins[i];
      outDim->setRange(
          1 /*single bin*/,
          static_cast<Mantid::coord_t>(binning.front()) /*min*/,
          static_cast<Mantid::coord_t>(
              binning.back()) /*max*/); // Set custom min, max and nbins.
    } else if (i < pbins.size() && similarBinning(pbins[i])) {
      auto binning = pbins[i];

      Mantid::coord_t pMin = static_cast<Mantid::coord_t>(binning.front());
      Mantid::coord_t pMax = static_cast<Mantid::coord_t>(binning.back());
      size_t numberOfBins;

      setMinMaxBins(pMin, pMax, numberOfBins, inDim, logger);

      outDim->setRange(numberOfBins, static_cast<Mantid::coord_t>(pMin) /*min*/,
                       static_cast<Mantid::coord_t>(
                           pMax) /*max*/); // Set custom min, max and nbins.
    }
    dimensions[i] = outDim;
  }
  return boost::make_shared<MDHistoWorkspace>(dimensions);
}

/**
 * Perform a weighted sum at the iterator position. This function does not
 * increment the iterator. Masked bins do not contribute.
 * @param iterator : Iterator to use in sum
 * @param box : Box implicit function defining valid region.
 * @param sumSignal : Accumlation in/out ref.
 * @param sumSQErrors : Accumulation error in/out ref. Squared value.
 * @param sumNEvents : Accumulation n_event in/out ref.
 */
void performWeightedSum(MDHistoWorkspaceIterator const *const iterator,
                        MDBoxImplicitFunction &box, double &sumSignal,
                        double &sumSQErrors, double &sumNEvents) {
  if (!iterator->getIsMasked()) {
    const double weight = box.fraction(iterator->getBoxExtents());
    if (weight != 0) {
      sumSignal += weight * iterator->getSignal();
      const double error = iterator->getError();
      sumSQErrors += weight * (error * error);
      sumNEvents += weight * double(iterator->getNumEventsFraction());
    }
  }
}

namespace Mantid {
namespace MDAlgorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegrateMDHistoWorkspace)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string IntegrateMDHistoWorkspace::name() const {
  return "IntegrateMDHistoWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int IntegrateMDHistoWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string IntegrateMDHistoWorkspace::category() const {
  return "MDAlgorithms\\Slicing";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string IntegrateMDHistoWorkspace::summary() const {
  return "Performs axis aligned integration of MDHistoWorkspaces";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegrateMDHistoWorkspace::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace.");

  const std::vector<double> defaultBinning;
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("P1Bin", defaultBinning),
      "Projection 1 binning.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("P2Bin", defaultBinning),
      "Projection 2 binning.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("P3Bin", defaultBinning),
      "Projection 3 binning.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("P4Bin", defaultBinning),
      "Projection 4 binning.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("P5Bin", defaultBinning),
      "Projection 5 binning.");

  declareProperty(make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
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

  const size_t emptyCount =
      std::count_if(pbins.begin(), pbins.end(), emptyBinning);
  if (emptyCount == pbins.size()) {
    // No work to do.
    g_log.information(this->name() + " Direct clone of input.");
    this->setProperty("OutputWorkspace",
                      boost::shared_ptr<IMDHistoWorkspace>(inWS->clone()));
  } else {

    /* Create the output workspace in the right shape. This allows us to iterate
       over our output
       structure and fill it.
     */
    MDHistoWorkspace_sptr outWS = createShapedOutput(inWS.get(), pbins, g_log);

    Progress progress(this, 0.0, 1.0, size_t(outWS->getNPoints()));

    // Store in each dimension
    std::vector<Mantid::coord_t> binWidthsOut(nDims);
    std::vector<int> widthVector(nDims); // used for nearest neighbour search
    for (size_t i = 0; i < nDims; ++i) {
      binWidthsOut[i] = outWS->getDimension(i)->getBinWidth();

      // Maximum width vector for region in output workspace corresponding to
      // region in input workspace.

      /* ceil(wout/win) = n_pixels in input corresponding to 1 pixel in output.
         The width vector is the total width. So we always need to double it to
         take account of the whole region.
         For example, 8/4 = 2, but thats only 1 pixel on each side of the
         center, we need 2 * that to give the correct answer of 4.
      */
      widthVector[i] =
          2 * static_cast<int>(std::ceil(binWidthsOut[i] /
                                         inWS->getDimension(i)->getBinWidth()));

      if (widthVector[i] % 2 == 0) {
        widthVector[i] += 1; // make it odd if not already.
      }
    }

    // Outer loop over the output workspace iterator poistions.
    const int nThreads = Mantid::API::FrameworkManager::Instance()
                             .getNumOMPThreads(); // NThreads to Request

    auto outIterators = outWS->createIterators(nThreads, nullptr);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < int(outIterators.size()); ++i) { // NOLINT
      PARALLEL_START_INTERUPT_REGION
      auto outIterator =
          dynamic_cast<MDHistoWorkspaceIterator *>(outIterators[i].get());
      if (!outIterator) {
        throw std::logic_error(
            "Failed to cast iterator to MDHistoWorkspaceIterator");
      }

      do {

        Mantid::Kernel::VMD outIteratorCenter = outIterator->getCenter();

        // Calculate the extents for this out iterator position.
        std::vector<Mantid::coord_t> mins(nDims);
        std::vector<Mantid::coord_t> maxs(nDims);
        for (size_t j = 0; j < nDims; ++j) {
          const coord_t delta = binWidthsOut[j] / 2;
          mins[j] = outIteratorCenter[j] - delta;
          maxs[j] = outIteratorCenter[j] + delta;
        }
        MDBoxImplicitFunction box(mins, maxs);

        double sumSignal = 0;
        double sumSQErrors = 0;
        double sumNEvents = 0;

        // Create a thread-local input iterator.

        auto iterator = inWS->createIterator();
        auto inIterator =
            dynamic_cast<MDHistoWorkspaceIterator *>(iterator.get());
        if (!inIterator) {
          throw std::runtime_error(
              "Could not convert IMDIterator to a MDHistoWorkspaceIterator");
        }

        /*
        We jump to the iterator position which is closest in the model
        coordinates
        to the centre of our output iterator. This allows us to consider a much
        smaller region of space as part of our inner loop
        rather than iterating over the full set of boxes of the input workspace.
        */
        inIterator->jumpToNearest(outIteratorCenter);
        performWeightedSum(inIterator, box, sumSignal, sumSQErrors,
                           sumNEvents); // Use the present position. neighbours
                                        // below exclude the current position.
        // Look at all of the neighbours of our position. We previously
        // calculated what the width vector would need to be.
        auto neighbourIndexes =
            inIterator->findNeighbourIndexesByWidth(widthVector);
        for (auto neighbourIndex : neighbourIndexes) {
          inIterator->jumpTo(neighbourIndex); // Go to that neighbour
          performWeightedSum(inIterator, box, sumSignal, sumSQErrors,
                             sumNEvents);
        }

        const size_t iteratorIndex = outIterator->getLinearIndex();
        outWS->setSignalAt(iteratorIndex, sumSignal);
        outWS->setErrorSquaredAt(iteratorIndex, sumSQErrors);
        outWS->setNumEventsAt(iteratorIndex, sumNEvents);

        progress.report();
      } while (outIterator->next());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
    outWS->setDisplayNormalization(inWS->displayNormalizationHisto());
    this->setProperty("OutputWorkspace", outWS);
  }
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
      errors.emplace(propertyName, result);
    }
  }
  return errors;
}

} // namespace MDAlgorithms
} // namespace Mantid
