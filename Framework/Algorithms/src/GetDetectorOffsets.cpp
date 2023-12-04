// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PeakParameterHelper.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(GetDetectorOffsets)

using namespace Kernel;
using Mantid::Kernel::Logger;
using namespace Algorithms::PeakParameterHelper;
using namespace API;
using std::size_t;
using namespace DataObjects;

//-----------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
 */
void GetDetectorOffsets::init() {

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),

                  "A 2D workspace with X values of d-spacing");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);

  declareProperty("Step", 0.001, mustBePositive, "Step size used to bin d-spacing data");
  declareProperty("DReference", 2.0, mustBePositive, "Center of reference peak in d-space");
  declareProperty("XMin", 0.0, "Minimum of CrossCorrelation data to search for peak, usually negative");
  declareProperty("XMax", 0.0, "Maximum of CrossCorrelation data to search for peak, usually positive");

  declareProperty(std::make_unique<FileProperty>("GroupingFileName", "", FileProperty::OptionalSave, ".cal"),
                  "Optional: The name of the output CalFile to save the "
                  "generated OffsetsWorkspace.");
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace containing the offsets.");

  // Mantid's python API _requires_ a non empty-string name for any Output workspace, even when 'PropertyMode::Optional'
  // is specified.
  declareProperty(std::make_unique<WorkspaceProperty<MaskWorkspace>>("MaskWorkspace", "_empty_", Direction::Output,
                                                                     PropertyMode::Optional),
                  "Mask workspace (optional input / output workspace):"
                  "  when specified, if the workspace already exists, any incoming masked detectors will be combined"
                  "  with any additional outgoing masked detectors detected by the algorithm");

  // Only keep peaks
  declareProperty("PeakFunction", "Gaussian",
                  std::make_shared<StringListValidator>(FunctionFactory::Instance().getFunctionNames<IPeakFunction>()),
                  "The function type for fitting the peaks.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("EstimateFWHM", false),
                  "Whether to esimate FWHM of peak function when estimating fit parameters");
  declareProperty("MaxOffset", 1.0, "Maximum absolute value of offsets; default is 1");

  /* Signed mode calculates offset in number of bins */
  std::vector<std::string> modes{"Relative", "Absolute", "Signed"};

  declareProperty("OffsetMode", "Relative", std::make_shared<StringListValidator>(modes),
                  "Whether to calculate a relative, absolute, or signed offset");
  declareProperty("DIdeal", 2.0, mustBePositive,
                  "The known peak centre value from the NIST standard "
                  "information, this is only used in Absolute OffsetMode.");
}

std::map<std::string, std::string> GetDetectorOffsets::validateInputs() {
  std::map<std::string, std::string> result;

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    result["InputWorkspace"] = "The InputWorkspace must be a MatrixWorkspace.";
    return result;
  }

  const auto unit = inputWS->getAxis(0)->unit()->caption();
  if (unit != "Bins of Shift" && unit != "d-Spacing") {
    const auto unitErrorMsg = "GetDetectorOffsets only supports input workspaces with units 'Bins of Shift' or "
                              "'d-Spacing', your unit was : " +
                              unit;
    result["InputWorkspace"] = unitErrorMsg;
    return result;
  }

  if (MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace")) {
    // detectors which are monitors are not included in the mask
    if (maskWS->getInstrument()->getNumberDetectors(true) != inputWS->getInstrument()->getNumberDetectors(true)) {
      result["MaskWorkspace"] = "incoming mask workspace must have the same instrument as the input workspace";
    } else if (maskWS->getNumberHistograms() != inputWS->getInstrument()->getNumberDetectors(true)) {
      result["MaskWorkspace"] = "incoming mask workspace must have one spectrum per detector";
    }
  }
  return result;
}
//-----------------------------------------------------------------------------------------
/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 */
void GetDetectorOffsets::exec() {
  inputW = getProperty("InputWorkspace");
  m_Xmin = getProperty("XMin");
  m_Xmax = getProperty("XMax");
  m_maxOffset = getProperty("MaxOffset");
  if (m_Xmin >= m_Xmax)
    throw std::runtime_error("Must specify m_Xmin<m_Xmax");
  m_dreference = getProperty("DReference");
  m_step = getProperty("Step");
  m_estimateFWHM = getProperty("EstimateFWHM");

  std::string mode_str = getProperty("OffsetMode");

  if (mode_str == "Absolute") {
    mode = offset_mode::absolute_offset;
  }

  else if (mode_str == "Relative") {
    mode = offset_mode::relative_offset;
  }

  else if (mode_str == "Signed") {
    mode = offset_mode::signed_offset;
  }

  m_dideal = getProperty("DIdeal");

  int64_t nspec = inputW->getNumberHistograms();

  // Create the output OffsetsWorkspace and initialize it to zero.
  auto outputW = std::make_shared<OffsetsWorkspace>(inputW->getInstrument());
  size_t N_d = outputW->getNumberHistograms();
  for (size_t di = 0; di < N_d; ++di)
    outputW->mutableY(di) = 0.0; // calls detail::FixedLengthVector::assign

  // Use the incoming mask workspace, or start a new one if the workspace does not exist.
  MaskWorkspace_sptr maskWS;
  if (!isDefault("MaskWorkspace")) {
    maskWS = getProperty("MaskWorkspace");
  }
  if (!maskWS) {
    g_log.debug() << "[GetDetectorOffsets]: CREATING new MaskWorkspace.\n";
    // A new mask is completely cleared at creation.
    maskWS = std::make_shared<MaskWorkspace>(inputW->getInstrument());
  } else {
    g_log.debug() << "[GetDetectorOffsets]: Using EXISTING MaskWorkspace.\n";
  }
  // Include any incoming masked detector flags in the mask-workspace values.
  maskWS->combineFromDetectorMasks(inputW->detectorInfo());

  // Fit all the spectra with a gaussian
  Progress prog(this, 0.0, 1.0, nspec);
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputW))
  for (int64_t wi = 0; wi < nspec; ++wi) {
    PARALLEL_START_INTERRUPT_REGION
    // Get the list of detectors in this pixel
    const auto &dets = inputW->getSpectrum(wi).getDetectorIDs();

    // If the entire spectrum is already masked, there's nothing to do.
    if (maskWS->isMasked(dets))
      continue;

    // Fit the peak
    double offset = fitSpectra(wi);
    bool spectrumIsMasked = false;
    if (std::abs(offset) > m_maxOffset) {
      g_log.debug() << "[GetDetectorOffsets]: fit failure: offset: " << std::abs(offset)
                    << " is greater than maximum allowed: " << m_maxOffset << ".\n";
      spectrumIsMasked = true;
    }

    // Most of the exec time is in FitSpectra, so this critical block should not
    // be a problem.
    PARALLEL_CRITICAL(GetDetectorOffsets_setValue) {
      // Use the same offset for all detectors from this pixel
      for (const auto &det : dets) {
        if (spectrumIsMasked) {
          maskWS->setMasked(det, true);
          continue;
        }
        // Warning: individual detectors in a spectrum may be masked.
        if (!maskWS->isMasked(det))
          outputW->setValue(det, offset);
      }
    }
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  // Make sure that the output workspaces' detector masks are consistent with the mask values.
  maskWS->combineToDetectorMasks();
  maskWS->combineToDetectorMasks(outputW->mutableDetectorInfo());

  if (g_log.getLevel() >= Logger::Priority::PRIO_TRACE) {
    auto &trace(g_log.getLogStream(Logger::Priority::PRIO_TRACE));
    trace << "[GetDetectorOffsets]: Computed offsets:\n" << std::endl;
    for (size_t ns = 0; ns < inputW->getNumberHistograms(); ++ns) {
      const auto dets = inputW->getSpectrum(ns).getDetectorIDs();
      for (const auto &det : dets)
        trace << "  " << outputW->getValue(det) << (maskWS->isMasked(det) ? "*" : "") << "\n";
    }
  }
  // Return the output
  setProperty("OutputWorkspace", outputW);

  // Only return the mask workspace if it was specified.
  if (!isDefault("MaskWorkspace"))
    setProperty("MaskWorkspace", maskWS);

  // Also save to .cal file, if requested
  std::string filename = getProperty("GroupingFileName");
  if (!filename.empty()) {
    progress(0.9, "Saving .cal file");
    auto childAlg = createChildAlgorithm("SaveCalFile");
    childAlg->setProperty("OffsetsWorkspace", outputW);
    childAlg->setProperty("MaskWorkspace", maskWS);
    childAlg->setPropertyValue("Filename", filename);
    childAlg->executeAsChildAlg();
  }
}

//-----------------------------------------------------------------------------------------
/** Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
 *
 *  @param s :: The Workspace Index to fit
 *  @return The calculated offset value
 */
double GetDetectorOffsets::fitSpectra(const int64_t s) {
  const double FIT_FAILURE = DBL_MAX;

  // Find point of peak centre
  const auto &yValues = inputW->y(s);
  auto it = std::max_element(yValues.cbegin(), yValues.cend());

  // Set the default peak height and location
  double peakHeight = *it;
  const double peakLoc = inputW->x(s)[it - yValues.begin()];

  // Return if peak of Cross Correlation is nan (Happens when spectra is zero)
  // Pixel with large offset will be masked
  if (std::isnan(peakHeight))
    return (FIT_FAILURE);

  IFunction_sptr fun_ptr = createFunction(peakHeight, peakLoc);

  // Try to observe the peak height and location
  const auto &histogram = inputW->histogram(s);
  const auto &vector_x = histogram.points();
  const auto start_index = findXIndex(vector_x, m_Xmin);
  const auto stop_index = findXIndex(vector_x, m_Xmax, start_index);
  // observe parameters if we found a peak range, otherwise use defaults
  if (start_index != stop_index) {
    // create a background function
    auto bkgdFunction = std::dynamic_pointer_cast<IBackgroundFunction>(fun_ptr->getFunction(0));
    auto peakFunction = std::dynamic_pointer_cast<IPeakFunction>(fun_ptr->getFunction(1));
    int result = estimatePeakParameters(histogram, std::pair<size_t, size_t>(start_index, stop_index), peakFunction,
                                        bkgdFunction, m_estimateFWHM, EstimatePeakWidth::Observation, EMPTY_DBL(), 0.0);
    if (result != PeakFitResult::GOOD) {
      g_log.debug() << "ws index: " << s
                    << "  bad result for estimating peak parameters, using default peak height and loc\n";
    }
  } else {
    g_log.notice() << "ws index: " << s
                   << "  range size is zero when estimating peak parameters, using default peak height and loc\n";
  }

  IAlgorithm_sptr fit_alg;
  try {
    // set the ChildAlgorithm no to log as this will be run once per spectra
    fit_alg = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate Fit algorithm");
    throw;
  }

  fit_alg->setProperty("Function", fun_ptr);

  fit_alg->setProperty("InputWorkspace", inputW);
  fit_alg->setProperty<int>("WorkspaceIndex",
                            static_cast<int>(s)); // TODO what is the right thing to do here?
  fit_alg->setProperty("StartX", m_Xmin);
  fit_alg->setProperty("EndX", m_Xmax);
  fit_alg->setProperty("MaxIterations", 100);

  fit_alg->executeAsChildAlg();
  std::string fitStatus = fit_alg->getProperty("OutputStatus");
  // Pixel with large offset will be masked
  if (fitStatus != "success") {
    g_log.debug() << "[GetDetectorOffsets]: Fit algorithm failure: " << fitStatus << "\n";
    return (FIT_FAILURE);
  }

  // std::vector<double> params = fit_alg->getProperty("Parameters");
  API::IFunction_sptr function = fit_alg->getProperty("Function");

  double offset = function->getParameter(3); // params[3]; // f1.PeakCentre

  if (mode == offset_mode::signed_offset) {
    // factor := factor * (1+offset) for d-spacemap conversion so factor cannot be
    // negative
    offset *= -1;
  }

  /* offset relative to the reference */
  else if (mode == offset_mode::relative_offset) {
    // factor := factor * (1+offset) for d-spacemap conversion so factor cannot be
    // negative
    offset = -1. * offset * m_step / (m_dreference + offset * m_step);
  }

  /* Offset relative to the ideal */
  else if (mode == offset_mode::absolute_offset) {

    offset = -1. * offset * m_step / (m_dreference + offset * m_step);

    // translated from(DIdeal - FittedPeakCentre)/(FittedPeakCentre)
    // given by Matt Tucker in ticket #10642

    offset += (m_dideal - m_dreference) / m_dreference;
  }

  return offset;
}

/**
 * Create a function string from the given parameters and the algorithm inputs
 * @param peakHeight :: The height of the peak
 * @param peakLoc :: The location of the peak
 */
IFunction_sptr GetDetectorOffsets::createFunction(const double peakHeight, const double peakLoc) {
  const FunctionFactoryImpl &creator = FunctionFactory::Instance();
  auto background = creator.createFunction("LinearBackground");
  auto peak = std::dynamic_pointer_cast<IPeakFunction>(creator.createFunction(getProperty("PeakFunction")));
  peak->setHeight(peakHeight);
  peak->setCentre(peakLoc);
  const double sigma(10.0);
  peak->setFwhm(2.0 * std::sqrt(2.0 * M_LN2) * sigma);

  auto fitFunc = new CompositeFunction(); // Takes ownership of the functions
  fitFunc->addFunction(background);
  fitFunc->addFunction(peak);

  return std::shared_ptr<IFunction>(fitFunc);
}

} // namespace Mantid::Algorithms
