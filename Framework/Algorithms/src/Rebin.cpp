// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Rebin.h"
#include "MantidHistogramData/Exception.h"
#include "MantidHistogramData/Rebin.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Rebin)

using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;
using HistogramData::BinEdges;
using HistogramData::Exception::InvalidBinEdgesError;
using HistogramData::Frequencies;
using HistogramData::FrequencyStandardDeviations;
using HistogramData::Histogram;

//---------------------------------------------------------------------------------------------
// Public static methods
//---------------------------------------------------------------------------------------------

/**
 * Return the rebin parameters from a user input
 * @param inParams Input vector from user
 * @param inputWS Input workspace from user
 * @param logger A reference to a logger
 * @returns A new vector containing the rebin parameters
 */
std::vector<double>
Rebin::rebinParamsFromInput(const std::vector<double> &inParams,
                            const API::MatrixWorkspace &inputWS,
                            Kernel::Logger &logger) {
  std::vector<double> rbParams;
  // The validator only passes parameters with size 1, or 3xn.  No need to check
  // again here
  if (inParams.size() >= 3) {
    // Input are min, delta, max
    rbParams = inParams;
  } else if (inParams.size() == 1) {
    double xmin = 0.;
    double xmax = 0.;
    inputWS.getXMinMax(xmin, xmax);

    logger.information() << "Using the current min and max as default " << xmin
                         << ", " << xmax << '\n';
    rbParams.resize(3);
    rbParams[0] = xmin;
    rbParams[1] = inParams[0];
    rbParams[2] = xmax;
    if ((rbParams[1] < 0.) && (xmin < 0.) && (xmax > 0.)) {
      std::stringstream msg;
      msg << "Cannot create logorithmic binning that changes sign (xmin="
          << xmin << ", xmax=" << xmax << ")";
      throw std::runtime_error(msg.str());
    }
  }
  return rbParams;
}

//---------------------------------------------------------------------------------------------
// Public methods
//---------------------------------------------------------------------------------------------

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void Rebin::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "Workspace containing the input data");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to give the output workspace");

  declareProperty(
      make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally "
      "this can be followed by a comma and more widths and last boundary "
      "pairs. "
      "Optionally this can also be a single number, which is the bin width. "
      "In this case, the boundary of binning will be determined by minimum and "
      "maximum TOF "
      "values among all events, or previous binning boundary, in case of event "
      "Workspace, or "
      "non-event Workspace, respectively. Negative width values indicate "
      "logarithmic binning. ");

  declareProperty(
      "PreserveEvents", true,
      "Keep the output workspace as an EventWorkspace, "
      "if the input has events. If the input and output EventWorkspace "
      "names are the same, only the X bins are set, which is very quick. If "
      "false, "
      "then the workspace gets converted to a Workspace2D histogram.");

  declareProperty(
      "FullBinsOnly", false,
      "Omit the final bin if it's width is smaller than the step size");

  declareProperty("IgnoreBinErrors", false,
                  "Ignore errors related to "
                  "zero/negative bin widths in "
                  "input/output workspaces. When ignored, the signal and "
                  "errors are set to zero");
}

/** Executes the rebin algorithm
 *
 *  @throw runtime_error Thrown if the bin range does not intersect the range of
 *the input workspace
 */
void Rebin::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // Are we preserving event workspace-iness?
  bool PreserveEvents = getProperty("PreserveEvents");

  // Rebinning in-place
  bool inPlace = (inputWS == outputWS);

  std::vector<double> rbParams =
      rebinParamsFromInput(getProperty("Params"), *inputWS, g_log);

  const bool dist = inputWS->isDistribution();
  const bool isHist = inputWS->isHistogramData();

  // workspace independent determination of length
  const int histnumber = static_cast<int>(inputWS->getNumberHistograms());

  bool fullBinsOnly = getProperty("FullBinsOnly");

  HistogramData::BinEdges XValues_new(0);
  // create new output X axis
  static_cast<void>(VectorHelper::createAxisFromRebinParams(
      rbParams, XValues_new.mutableRawData(), true, fullBinsOnly));

  // Now, determine if the input workspace is actually an EventWorkspace
  EventWorkspace_const_sptr eventInputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  if (eventInputWS != nullptr) {
    //------- EventWorkspace as input -------------------------------------

    if (PreserveEvents) {
      if (!inPlace) {
        outputWS = inputWS->clone();
      }
      auto eventOutputWS =
          boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
      // This only sets the X axis. Actual rebinning will be done upon data
      // access.
      eventOutputWS->setAllX(XValues_new);
    } else {
      //--------- Different output, OR you're inplace but not preserving Events
      g_log.information() << "Creating a Workspace2D from the EventWorkspace "
                          << eventInputWS->getName() << ".\n";
      outputWS = DataObjects::create<DataObjects::Workspace2D>(
          *inputWS, histnumber, XValues_new);

      // Initialize progress reporting.
      Progress prog(this, 0.0, 1.0, histnumber);

      // Go through all the histograms and set the data
      PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
      for (int i = 0; i < histnumber; ++i) {
        PARALLEL_START_INTERUPT_REGION
        // Get a const event list reference. eventInputWS->dataY() doesn't work.
        const EventList &el = eventInputWS->getSpectrum(i);
        MantidVec y_data, e_data;
        // The EventList takes care of histogramming.
        el.generateHistogram(XValues_new.rawData(), y_data, e_data);

        // Copy the data over.
        outputWS->mutableY(i) = std::move(y_data);
        outputWS->mutableE(i) = std::move(e_data);

        // Report progress
        prog.report(name());
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Copy all the axes
      for (int i = 1; i < inputWS->axes(); i++) {
        outputWS->replaceAxis(i, inputWS->getAxis(i)->clone(outputWS.get()));
        outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
      }

      // Copy the units over too.
      for (int i = 0; i < outputWS->axes(); ++i)
        outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
      outputWS->setYUnit(eventInputWS->YUnit());
      outputWS->setYUnitLabel(eventInputWS->YUnitLabel());
    }

    // Assign it to the output workspace property
    setProperty("OutputWorkspace", outputWS);

  } // END ---- EventWorkspace

  else

  { //------- Workspace2D or other MatrixWorkspace ---------------------------

    if (!isHist) {
      g_log.information() << "Rebin: Converting Data to Histogram.\n";
      Mantid::API::Algorithm_sptr ChildAlg =
          createChildAlgorithm("ConvertToHistogram");
      ChildAlg->initialize();
      ChildAlg->setProperty("InputWorkspace", inputWS);
      ChildAlg->execute();
      inputWS = ChildAlg->getProperty("OutputWorkspace");
    }

    // This will be the output workspace (exact type may vary)
    API::MatrixWorkspace_sptr outputWS;

    // make output Workspace the same type is the input, but with new length of
    // signal array
    outputWS = DataObjects::create<API::HistoWorkspace>(*inputWS, histnumber,
                                                        XValues_new);

    // Copy over the 'vertical' axis
    if (inputWS->axes() > 1)
      outputWS->replaceAxis(1, inputWS->getAxis(1)->clone(outputWS.get()));
    bool ignoreBinErrors = getProperty("IgnoreBinErrors");

    Progress prog(this, 0.0, 1.0, histnumber);
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
    for (int hist = 0; hist < histnumber; ++hist) {
      PARALLEL_START_INTERUPT_REGION

      try {
        outputWS->setHistogram(
            hist, HistogramData::rebin(inputWS->histogram(hist), XValues_new));
      } catch (InvalidBinEdgesError &) {
        if (ignoreBinErrors)
          outputWS->setBinEdges(hist, XValues_new);
        else
          throw;
      }
      prog.report(name());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
    outputWS->setDistribution(dist);

    // Now propagate any masking correctly to the output workspace
    // More efficient to have this in a separate loop because
    // MatrixWorkspace::maskBins blocks multi-threading
    for (int i = 0; i < histnumber; ++i) {
      if (inputWS->hasMaskedBins(
              i)) // Does the current spectrum have any masked bins?
      {
        this->propagateMasks(inputWS, outputWS, i);
      }
    }
    // Copy the units over too.
    for (int i = 0; i < outputWS->axes(); ++i) {
      outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
    }

    if (!isHist) {
      g_log.information() << "Rebin: Converting Data back to Data Points.\n";
      Mantid::API::Algorithm_sptr ChildAlg =
          createChildAlgorithm("ConvertToPointData");
      ChildAlg->initialize();
      ChildAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
      ChildAlg->execute();
      outputWS = ChildAlg->getProperty("OutputWorkspace");
    }

    // Assign it to the output workspace property
    setProperty("OutputWorkspace", outputWS);

  } // END ---- Workspace2D
}

/** Takes the masks in the input workspace and apportions the weights into the
 *new bins that overlap
 *  with a masked bin. These bins are then masked with the calculated weight.
 *
 *  @param inputWS ::  The input workspace
 *  @param outputWS :: The output workspace
 *  @param hist ::    The index of the current histogram
 */
void Rebin::propagateMasks(API::MatrixWorkspace_const_sptr inputWS,
                           API::MatrixWorkspace_sptr outputWS, int hist) {
  // Not too happy with the efficiency of this way of doing it, but it's a lot
  // simpler to use the
  // existing rebin algorithm to distribute the weights than to re-implement it
  // for this

  MantidVec masked_bins, weights;
  // Get a reference to the list of masked bins for this spectrum
  const MatrixWorkspace::MaskList &mask = inputWS->maskedBins(hist);
  // Now iterate over the list, building up a vector of the masked bins
  auto it = mask.cbegin();
  auto &XValues = inputWS->x(hist);
  masked_bins.push_back(XValues[(*it).first]);
  weights.push_back((*it).second);
  masked_bins.push_back(XValues[(*it).first + 1]);
  for (++it; it != mask.end(); ++it) {
    const double currentX = XValues[(*it).first];
    // Add an intermediate bin with zero weight if masked bins aren't
    // consecutive
    if (masked_bins.back() != currentX) {
      weights.push_back(0.0);
      masked_bins.push_back(currentX);
    }
    weights.push_back((*it).second);
    masked_bins.push_back(XValues[(*it).first + 1]);
  }

  //// Create a zero vector for the errors because we don't care about them here
  auto errSize = weights.size();
  Histogram oldHist(BinEdges(std::move(masked_bins)),
                    Frequencies(std::move(weights)),
                    FrequencyStandardDeviations(errSize, 0));
  // Use rebin function to redistribute the weights. Note that distribution flag
  // is set
  bool ignoreErrors = getProperty("IgnoreBinErrors");

  try {
    auto newHist = HistogramData::rebin(oldHist, outputWS->binEdges(hist));
    auto &newWeights = newHist.y();

    // Now process the output vector and fill the new masking list
    for (size_t index = 0; index < newWeights.size(); ++index) {
      if (newWeights[index] > 0.0)
        outputWS->flagMasked(hist, index, newWeights[index]);
    }
  } catch (InvalidBinEdgesError &) {
    if (!ignoreErrors)
      throw;
  }
}

} // namespace Algorithms
} // namespace Mantid
