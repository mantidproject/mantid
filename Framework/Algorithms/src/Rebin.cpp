// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Rebin.h"
#include "MantidHistogramData/Exception.h"
#include "MantidHistogramData/Rebin.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {

namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string PARAMS("Params");
const std::string PRSRV_EVENTS("PreserveEvents");
const std::string FULL_BIN_ONLY("FullBinsOnly");
const std::string IGNR_BIN_ERR("IgnoreBinErrors");
const std::string RVRS_LOG_BIN("UseReverseLogarithmic");
const std::string POWER("Power");
const std::string BINMODE("BinningMode");
} // namespace PropertyNames

namespace {
const std::vector<std::string> binningModeNames{"Default", "Linear", "Logarithmic", "ReverseLogarithmic", "Power"};
enum class BinningMode { DEFAULT, LINEAR, LOGARITHMIC, REVERSELOG, POWER, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;
} // namespace

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
using HistogramData::Frequencies;
using HistogramData::FrequencyStandardDeviations;
using HistogramData::Histogram;
using HistogramData::Exception::InvalidBinEdgesError;

//---------------------------------------------------------------------------------------------
// Public static methods
//---------------------------------------------------------------------------------------------

/**
 * Return the rebin parameters from a user input
 * @param inParams Input vector from user
 * @param inputWS Input workspace from user
 * @param logger A reference to a logger
 * @param binModeName The enumerated string specifying the binning mode
 *    ("Default", "Linear", "Logarithmic", "ReverseLogarithmic", "Power")
 * @returns A new vector containing the rebin parameters
 * @throw runtime_error if bounds of logarithmic binning go from negative topositive
 */
std::vector<double> Rebin::rebinParamsFromInput(const std::vector<double> &inParams,
                                                const API::MatrixWorkspace &inputWS, Kernel::Logger &logger,
                                                const std::string &binModeName) {
  // EnumeratedString<Rebin::BinningMode, binningModeNames> binMode = binModeName;
  std::vector<double> rbParams;
  // The validator only passes parameters with size 2n+1. No need to check again here
  if (inParams.size() >= 3) {
    // Input are min, delta1, mid1, delta2, mid2, ... , max
    rbParams = inParams;
  } else if (inParams.size() == 1) {
    double xmin = 0.;
    double xmax = 0.;
    inputWS.getXMinMax(xmin, xmax);

    logger.information() << "Using the current min and max as default " << xmin << ", " << xmax << '\n';
    rbParams.resize(3);
    rbParams[0] = xmin;
    rbParams[1] = inParams[0];
    rbParams[2] = xmax;
  }

  // if linear or power binning specified, require positive bin width
  // if logarithmic binning specified, require "negative" bin width
  BINMODE binMode = binModeName;
  if (binMode != BinningMode::DEFAULT) {
    logger.information() << "Bin mode set, forcing bin parameters to match.";
    for (size_t i = 0; i < rbParams.size() - 2; i += 2) { // e.g. xmin, xstep1, xmid1, xstep2, xmid2, xstep3, xmax
      if (binMode == BinningMode::LINEAR || binMode == BinningMode::POWER) {
        rbParams[i + 1] = fabs(rbParams[i + 1]);
      } else if (binMode == BinningMode::LOGARITHMIC || binMode == BinningMode::REVERSELOG) {
        rbParams[i + 1] = -fabs(rbParams[i + 1]);
      }
    }
  } // end if
  for (size_t i = 0; i < rbParams.size() - 2; i += 2) {
    // make sure logarithmic binning does not change signs
    if (rbParams[i] < 0 && rbParams[i + 1] < 0 && rbParams[i + 2] > 0) {
      std::stringstream msg;
      msg << "Cannot create logarithmic binning that changes sign (xmin=";
      msg << rbParams[i] << ", xmax=" << rbParams[i + 2] << ")";
      throw std::runtime_error(msg.str());
    }
  } // end for
  return rbParams;
}

//---------------------------------------------------------------------------------------------
// Public methods
//---------------------------------------------------------------------------------------------

/// Validate that the input properties are sane.
std::map<std::string, std::string> Rebin::validateInputs() {
  std::map<std::string, std::string> helpMessages;

  // determing the binning mode, if present, or use default setting
  BINMODE binMode;
  if (existsProperty(PropertyNames::BINMODE))
    binMode = getPropertyValue(PropertyNames::BINMODE);
  else
    binMode = "Default";

  // validate the rebin params, and outside default mode, reset them
  MatrixWorkspace_sptr inputWS = getProperty(PropertyNames::INPUT_WKSP);
  std::vector<double> rbParams = getProperty(PropertyNames::PARAMS);
  if (inputWS == nullptr) {
    // The workspace could exist, but not be a MatrixWorkspace, e.g. it might be
    // a group workspace. In that case we don't want a validation error for the
    // Rebin parameters.
    const std::string &inputWsName = getProperty(PropertyNames::INPUT_WKSP);
    if (!AnalysisDataService::Instance().doesExist(inputWsName)) {
      helpMessages[PropertyNames::INPUT_WKSP] = "Input workspace not in ADS.";
    }
  } else {
    try {
      std::vector<double> validParams = rebinParamsFromInput(rbParams, *inputWS, g_log, binMode);
      // if the binmode has been set, force the rebin params to be consistent
      if (binMode != BinningMode::DEFAULT) {
        setProperty(PropertyNames::PARAMS, validParams);
        rbParams = validParams;
      }
    } catch (std::exception &err) {
      helpMessages[PropertyNames::PARAMS] = err.what();
    }
  }

  // if user specifies a binning mode, set this flag for them
  if (binMode == BinningMode::REVERSELOG) {
    if (existsProperty(PropertyNames::RVRS_LOG_BIN)) {
      setProperty(PropertyNames::RVRS_LOG_BIN, true);
    }
  } else if (binMode != BinningMode::DEFAULT) {
    if (existsProperty(PropertyNames::RVRS_LOG_BIN)) {
      setProperty(PropertyNames::RVRS_LOG_BIN, false);
    }
  }

  // perform checks on the power property, if valid
  if (existsProperty(PropertyNames::POWER)) {
    // ensure that the power property is set if using power binning
    if (isDefault(PropertyNames::POWER) && binMode == BinningMode::POWER) {
      std::string msg = "The binning mode was set to 'Power', but no power was given.";
      helpMessages[PropertyNames::POWER] = msg;
      helpMessages[PropertyNames::BINMODE] = msg;
      return helpMessages;
    }
    // if the power is set, perform checks
    else if (!isDefault(PropertyNames::POWER)) {
      // power is only available in Default and Power binning modes
      if (binMode != BinningMode::DEFAULT && binMode != BinningMode::POWER) {
        g_log.information() << "Discarding input power for incompatible binning mode.";
        setProperty(PropertyNames::POWER, 0.0);
      } else { // power is a property, is not default, and binning mode is power of default
        const double power = getProperty(PropertyNames::POWER);

        // attempt to roughly guess how many bins these parameters imply
        double roughEstimate = 0;

        // Five significant places of the Euler-Mascheroni constant is probably more than enough for our needs
        double eulerMascheroni = 0.57721;

        // Params is checked by the validator first, so we can assume it is in a correct format
        for (size_t i = 0; i < rbParams.size() - 2; i += 2) {
          double upperLimit = rbParams[i + 2];
          double lowerLimit = rbParams[i];
          double factor = rbParams[i + 1];

          // in default mode, give error if try to mix power and log binning
          // because of prior validation, we can assume this will only happen in default mode
          if (factor <= 0) {
            helpMessages[PropertyNames::PARAMS] = "Provided width value cannot be negative for inverse power binning.";
            return helpMessages;
          }
          if (power == 1) {
            roughEstimate += std::exp((upperLimit - lowerLimit) / factor - eulerMascheroni);
          } else {
            roughEstimate += std::pow(((upperLimit - lowerLimit) / factor) * (1 - power) + 1, 1 / (1 - power));
          }
        } // end for i in rbParams.size()
        // Prevent the user form creating too many bins
        if (roughEstimate > 10000) {
          helpMessages[PropertyNames::POWER] = "This binning is expected to give more than 10000 bins.";
        }
      } // end else
    } // end else if
  } // end if property power exists
  return helpMessages;
}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void Rebin::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
                  "Workspace containing the input data");
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
                  "The name to give the output workspace");

  declareProperty(
      std::make_unique<ArrayProperty<double>>(PropertyNames::PARAMS, std::make_shared<RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally this can be followed by a comma and more widths and last boundary pairs. "
      "Optionally this can also be a single number, which is the bin width. In this case, the boundary of "
      "binning will be determined by minimum and maximum TOF values among all events, or previous binning "
      "boundary, in case of event Workspace, or non-event Workspace, respectively. "
      "Negative width values indicate logarithmic binning.");

  declareProperty(PropertyNames::PRSRV_EVENTS, true,
                  "Keep the output workspace as an EventWorkspace, if the input has events. If the input and output "
                  "EventWorkspace names are the same, only the X bins are set, which is very quick. If false, then the "
                  "workspace gets converted to a Workspace2D histogram.");

  declareProperty(PropertyNames::FULL_BIN_ONLY, false, "Omit the final bin if its width is smaller than the step size");

  declareProperty(PropertyNames::IGNR_BIN_ERR, false,
                  "Ignore errors related to zero/negative bin widths in input/output workspaces. When ignored, the "
                  "signal and errors are set to zero");

  declareProperty(
      PropertyNames::RVRS_LOG_BIN, false,
      "For logarithmic intervals, the splitting starts from the end and goes back to the start, ie the bins are bigger "
      "at the start getting exponentially smaller until they reach the end. For these bins, the FullBinsOnly flag is "
      "ignored.");

  auto powerValidator = std::make_shared<Mantid::Kernel::BoundedValidator<double>>();
  powerValidator->setLower(0);
  powerValidator->setUpper(1);
  declareProperty(PropertyNames::POWER, 0., powerValidator,
                  "Splits the interval in bins which actual width is equal to requested width / (i ^ power); default "
                  "is linear. Power must be between 0 and 1.");

  declareProperty(
      std::make_unique<EnumeratedStringProperty<BinningMode, &binningModeNames>>(PropertyNames::BINMODE),
      "Optional. "
      "Binning behavior can be specified in the usual way through sign of binwidth and other properties ('Default'); "
      "or can be set to one of the allowed binning modes. "
      "This will override all other specification or default behavior.");
}

/** Executes the rebin algorithm
 *
 *  @throw runtime_error Thrown if the bin range does not intersect the range of
 *the input workspace
 */
void Rebin::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr inputWS = getProperty(PropertyNames::INPUT_WKSP);
  MatrixWorkspace_sptr outputWS = getProperty(PropertyNames::OUTPUT_WKSP);

  // Are we preserving event workspace-iness?
  bool PreserveEvents = getProperty(PropertyNames::PRSRV_EVENTS);

  // Rebinning in-place
  bool inPlace = (inputWS == outputWS);

  std::vector<double> rbParams = rebinParamsFromInput(getProperty(PropertyNames::PARAMS), *inputWS, g_log);

  const bool dist = inputWS->isDistribution();
  const bool isHist = inputWS->isHistogramData();

  // workspace independent determination of length
  const auto histnumber = static_cast<int>(inputWS->getNumberHistograms());

  bool fullBinsOnly = getProperty(PropertyNames::FULL_BIN_ONLY);
  bool useReverseLog = getProperty(PropertyNames::RVRS_LOG_BIN);
  double power = getProperty(PropertyNames::POWER);

  double xmin = 0.;
  double xmax = 0.;
  inputWS->getXMinMax(xmin, xmax);

  HistogramData::BinEdges XValues_new(0);
  // create new output X axis
  static_cast<void>(VectorHelper::createAxisFromRebinParams(rbParams, XValues_new.mutableRawData(), true, fullBinsOnly,
                                                            xmin, xmax, useReverseLog, power));

  // Now, determine if the input workspace is actually an EventWorkspace
  EventWorkspace_const_sptr eventInputWS = std::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  if (eventInputWS != nullptr) {
    //------- EventWorkspace as input -------------------------------------

    if (PreserveEvents) {
      if (!inPlace) {
        outputWS = inputWS->clone();
      }
      auto eventOutputWS = std::dynamic_pointer_cast<EventWorkspace>(outputWS);
      // This only sets the X axis. Actual rebinning will be done upon data
      // access.
      eventOutputWS->setAllX(XValues_new);
    } else {
      //--------- Different output, OR you're inplace but not preserving Events
      g_log.information() << "Creating a Workspace2D from the EventWorkspace " << eventInputWS->getName() << ".\n";
      outputWS = DataObjects::create<DataObjects::Workspace2D>(*inputWS, histnumber, XValues_new);

      // Initialize progress reporting.
      Progress prog(this, 0.0, 1.0, histnumber);

      bool useUnsortingHistogram = (rbParams.size() < 4) && !useReverseLog && power == 0.0;
      g_log.information() << "Generating histogram without sorting=" << useUnsortingHistogram << "\n";

      // Go through all the histograms and set the data
      PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
      for (int i = 0; i < histnumber; ++i) {
        PARALLEL_START_INTERRUPT_REGION
        // Get a const event list reference. eventInputWS->dataY() doesn't work.
        const EventList &el = eventInputWS->getSpectrum(i);
        MantidVec y_data, e_data;
        // The EventList takes care of histogramming.
        if (useUnsortingHistogram)
          el.generateHistogram(rbParams[1], XValues_new.rawData(), y_data, e_data);
        else
          el.generateHistogram(XValues_new.rawData(), y_data, e_data);

        // Copy the data over.
        outputWS->mutableY(i) = y_data;
        outputWS->mutableE(i) = e_data;

        // Report progress
        prog.report(name());
        PARALLEL_END_INTERRUPT_REGION
      }
      PARALLEL_CHECK_INTERRUPT_REGION
    }

    // Assign it to the output workspace property
    setProperty(PropertyNames::OUTPUT_WKSP, outputWS);

  } // END ---- EventWorkspace

  else

  { //------- Workspace2D or other MatrixWorkspace ---------------------------

    if (!isHist) {
      g_log.information() << "Rebin: Converting Data to Histogram.\n";
      Mantid::API::Algorithm_sptr ChildAlg = createChildAlgorithm("ConvertToHistogram");
      ChildAlg->initialize();
      ChildAlg->setProperty("InputWorkspace", inputWS);
      ChildAlg->execute();
      inputWS = ChildAlg->getProperty("OutputWorkspace");
    }

    // make output Workspace the same type is the input, but with new length of
    // signal array
    outputWS = DataObjects::create<API::HistoWorkspace>(*inputWS, histnumber, XValues_new);

    bool ignoreBinErrors = getProperty(PropertyNames::IGNR_BIN_ERR);

    Progress prog(this, 0.0, 1.0, histnumber);
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
    for (int hist = 0; hist < histnumber; ++hist) {
      PARALLEL_START_INTERRUPT_REGION

      try {
        outputWS->setHistogram(hist, HistogramData::rebin(inputWS->histogram(hist), XValues_new));
      } catch (InvalidBinEdgesError &) {
        if (ignoreBinErrors)
          outputWS->setBinEdges(hist, XValues_new);
        else
          throw;
      }
      prog.report(name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
    outputWS->setDistribution(dist);

    // Now propagate any masking correctly to the output workspace
    // More efficient to have this in a separate loop because
    // MatrixWorkspace::maskBins blocks multi-threading
    for (int i = 0; i < histnumber; ++i) {
      if (inputWS->hasMaskedBins(i)) // Does the current spectrum have any masked bins?
      {
        this->propagateMasks(inputWS, outputWS, i, ignoreBinErrors);
      }
    }

    if (!isHist) {
      g_log.information() << "Rebin: Converting Data back to Data Points.\n";
      Mantid::API::Algorithm_sptr ChildAlg = createChildAlgorithm("ConvertToPointData");
      ChildAlg->initialize();
      ChildAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
      ChildAlg->execute();
      outputWS = ChildAlg->getProperty("OutputWorkspace");
    }

    // Assign it to the output workspace property
    setProperty(PropertyNames::OUTPUT_WKSP, outputWS);

  } // END ---- Workspace2D
}

/** Takes the masks in the input workspace and apportions the weights into the
 *new bins that overlap
 *  with a masked bin. These bins are then masked with the calculated weight.
 *
 *  @param inputWS ::  The input workspace
 *  @param outputWS :: The output workspace
 *  @param hist ::    The index of the current histogram
 *  @param ignoreErrors :: If to ignore bin errors
 */
void Rebin::propagateMasks(const API::MatrixWorkspace_const_sptr &inputWS, const API::MatrixWorkspace_sptr &outputWS,
                           const int hist, const bool ignoreErrors) {
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
  masked_bins.emplace_back(XValues[(*it).first]);
  weights.emplace_back((*it).second);
  masked_bins.emplace_back(XValues[(*it).first + 1]);
  for (++it; it != mask.end(); ++it) {
    const double currentX = XValues[(*it).first];
    // Add an intermediate bin with zero weight if masked bins aren't
    // consecutive
    if (masked_bins.back() != currentX) {
      weights.emplace_back(0.0);
      masked_bins.emplace_back(currentX);
    }
    weights.emplace_back((*it).second);
    masked_bins.emplace_back(XValues[(*it).first + 1]);
  }

  //// Create a zero vector for the errors because we don't care about them here
  auto errSize = weights.size();
  Histogram oldHist(BinEdges(std::move(masked_bins)), Frequencies(std::move(weights)),
                    FrequencyStandardDeviations(errSize, 0));
  // Use rebin function to redistribute the weights. Note that distribution flag
  // is set

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
