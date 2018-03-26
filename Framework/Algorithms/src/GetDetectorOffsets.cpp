#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(GetDetectorOffsets)

using namespace Kernel;
using namespace API;
using std::size_t;
using namespace DataObjects;

//-----------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
 */
void GetDetectorOffsets::init() {

  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("dSpacing")),
                  "A 2D workspace with X values of d-spacing");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);

  declareProperty("Step", 0.001, mustBePositive,
                  "Step size used to bin d-spacing data");
  declareProperty("DReference", 2.0, mustBePositive,
                  "Center of reference peak in d-space");
  declareProperty(
      "XMin", 0.0,
      "Minimum of CrossCorrelation data to search for peak, usually negative");
  declareProperty(
      "XMax", 0.0,
      "Maximum of CrossCorrelation data to search for peak, usually positive");

  declareProperty(make_unique<FileProperty>("GroupingFileName", "",
                                            FileProperty::OptionalSave, ".cal"),
                  "Optional: The name of the output CalFile to save the "
                  "generated OffsetsWorkspace.");
  declareProperty(make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace containing the offsets.");
  declareProperty(make_unique<WorkspaceProperty<>>("MaskWorkspace", "Mask",
                                                   Direction::Output),
                  "An output workspace containing the mask.");
  // Only keep peaks
  declareProperty(
      "PeakFunction", "Gaussian",
      boost::make_shared<StringListValidator>(
          FunctionFactory::Instance().getFunctionNames<IPeakFunction>()),
      "The function type for fitting the peaks.");

  declareProperty("MaxOffset", 1.0,
                  "Maximum absolute value of offsets; default is 1");

  std::vector<std::string> modes{"Relative", "Absolute"};

  declareProperty("OffsetMode", "Relative",
                  boost::make_shared<StringListValidator>(modes),
                  "Whether to calculate a relative or absolute offset");
  declareProperty("DIdeal", 2.0, mustBePositive,
                  "The known peak centre value from the NIST standard "
                  "information, this is only used in Absolute OffsetMode.");

  declareProperty(
      "FitEachPeakTwice", false,
      "If true, then each peak will be fitted twice."
      "Peak fitting range for the first round fitting wil be determined by "
      "given XMin and XMax"
      "And in second round peak fitting, fit range will be limited to FHWM. ");

  declareProperty(make_unique<WorkspaceProperty<TableWorkspace>>(
                      "PeakFitResultTableWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "Name of the input Tableworkspace containing peak fit window "
                  "information for each spectrum. ");
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

  std::string mode = getProperty("OffsetMode");
  bool isAbsolute = false;
  if (mode == "Absolute") {
    isAbsolute = true;
  }

  m_dideal = getProperty("DIdeal");

  int64_t nspec = inputW->getNumberHistograms();
  // Create the output OffsetsWorkspace
  auto outputW = boost::make_shared<OffsetsWorkspace>(inputW->getInstrument());
  // Create the output MaskWorkspace
  auto maskWS = boost::make_shared<MaskWorkspace>(inputW->getInstrument());
  // To get the workspace index from the detector ID
  const detid2index_map pixel_to_wi =
      maskWS->getDetectorIDToWorkspaceIndexMap(true);

  // fit each peak twice?
  bool fit_peak_twice = getProperty("FitEachPeakTwice");

  // output fitting result?
  std::string result_ws_name = getPropertyValue("PeakFitResultTableWorkspace");
  TableWorkspace_sptr fit_result_table;
  if (result_ws_name.size() > 0) {
    fit_result_table = boost::make_shared<TableWorkspace>();
    fit_result_table->addColumn("int", "wsindex");
    fit_result_table->addColumn("double", "center");
    fit_result_table->addColumn("double", "fwhm");
    fit_result_table->addColumn("double", "height");
    fit_result_table->addColumn("double", "A0");
    fit_result_table->addColumn("double", "A1");
    for (size_t iws = 0; iws < inputW->getNumberHistograms(); ++iws)
      fit_result_table->appendRow();
  } else {
    fit_result_table = 0;
  }

  // Fit all the spectra with a gaussian
  Progress prog(this, 0.0, 1.0, nspec);
  auto &spectrumInfo = maskWS->mutableSpectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputW))
  for (int64_t wi = 0; wi < nspec; ++wi) {
    PARALLEL_START_INTERUPT_REGION
    // Fit the peak
    double peak_center, peak_fwhm;
    GetDetectorsOffset::PeakLinearFunction fit_result;
    double offset = fitSpectra(wi, isAbsolute, m_Xmin, m_Xmax, peak_center,
                               peak_fwhm, fit_result);
    if (fit_peak_twice && peak_fwhm > 0.5) {
      double xmin = peak_center - 0.5 * peak_fwhm;
      double xmax = peak_center + 0.5 * peak_fwhm;
      g_log.notice() << "[DB...BAT] ws-index " << wi
                     << " found: center = " << peak_center
                     << ", FHWM = " << peak_fwhm << "; new fit range: " << xmin
                     << ", " << xmax << "\n";
      offset = fitSpectra(wi, isAbsolute, xmin, xmax, peak_center, peak_fwhm,
                          fit_result);
    }

    double mask = 0.0;
    if (std::abs(offset) > m_maxOffset) {
      offset = 0.0;
      mask = 1.0;
    }

    // Get the list of detectors in this pixel
    const auto &dets = inputW->getSpectrum(wi).getDetectorIDs();

    // Most of the exec time is in FitSpectra, so this critical block should not
    // be a problem.
    PARALLEL_CRITICAL(GetDetectorOffsets_setValue) {
      // Use the same offset for all detectors from this pixel
      for (const auto &det : dets) {
        outputW->setValue(det, offset);
        const auto mapEntry = pixel_to_wi.find(det);
        if (mapEntry == pixel_to_wi.end())
          continue;
        const size_t workspaceIndex = mapEntry->second;
        if (mask == 1.) {
          // Being masked
          maskWS->getSpectrum(workspaceIndex).clearData();
          spectrumInfo.setMasked(workspaceIndex, true);
          maskWS->mutableY(workspaceIndex)[0] = mask;
        } else {
          // Using the detector
          maskWS->mutableY(workspaceIndex)[0] = mask;
        }
      }
      if (fit_result_table) {
        API::TableRow row = fit_result_table->getRow(wi);
        row << static_cast<int>(wi) << fit_result.center << fit_result.fwhm
            << fit_result.height << fit_result.a0 << fit_result.a1;
      }
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Return the output
  setProperty("OutputWorkspace", outputW);
  setProperty("MaskWorkspace", maskWS);

  // Also save to .cal file, if requested
  std::string filename = getProperty("GroupingFileName");
  if (!filename.empty()) {
    progress(0.9, "Saving .cal file");
    IAlgorithm_sptr childAlg = createChildAlgorithm("SaveCalFile");
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
 *  @param isAbsolbute :: Whether to calculate an absolute offset
 *  @param xmin: minimum value in fit range
 *  @param xmax: maximum value in fit range
 *  @param peak_center: peak center found
 *  @param peak_fwhm: peak FHWM found
 *  @return The calculated offset value
 */
double GetDetectorOffsets::fitSpectra(
    const int64_t s, bool isAbsolbute, const double xmin, const double xmax,
    double &peak_center, double &peak_fwhm,
    GetDetectorsOffset::PeakLinearFunction &fit_result) {
  // Find point of peak centre
  const auto &yValues = inputW->y(s);
  auto it = std::max_element(yValues.cbegin(), yValues.cend());
  const double peakHeight = *it;
  const double peakLoc = inputW->x(s)[it - yValues.begin()];
  // Return if peak of Cross Correlation is nan (Happens when spectra is zero)
  // Pixel with large offset will be masked
  if (std::isnan(peakHeight))
    return (1000.);

  IAlgorithm_sptr fit_alg;
  try {
    // set the ChildAlgorithm no to log as this will be run once per spectra
    fit_alg = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate Fit algorithm");
    throw;
  }
  auto fun = createFunction(peakHeight, peakLoc);
  fit_alg->setProperty("Function", fun);

  fit_alg->setProperty("InputWorkspace", inputW);
  fit_alg->setProperty<int>(
      "WorkspaceIndex",
      static_cast<int>(s)); // TODO what is the right thing to do here?
  fit_alg->setProperty("StartX", xmin);
  fit_alg->setProperty("EndX", xmax);
  fit_alg->setProperty("MaxIterations", 100);

  IFunction_sptr fun_ptr = createFunction(peakHeight, peakLoc);

  fit_alg->setProperty("Function", fun_ptr);
  fit_alg->executeAsChildAlg();
  std::string fitStatus = fit_alg->getProperty("OutputStatus");
  // Pixel with large offset will be masked
  if (fitStatus != "success")
    return (1000.);

  // std::vector<double> params = fit_alg->getProperty("Parameters");
  API::IFunction_sptr function = fit_alg->getProperty("Function");
  // double offset = function->getParameter(3); // params[3]; // f1.PeakCentre

  peak_center = function->getParameter(3);
  peak_fwhm = function->getParameter(2);
  // peak_center = function->center();
  // peak_fwhm = function->getFWHM();
  throw std::runtime_error("Need to make sure peak center, peak fwhm are 2 and "
                           "3 for all peak function!");

  // get fitted result
  fit_result.center = peak_center;
  fit_result.fwhm = peak_fwhm;
  fit_result.height = function->getParameter(0);
  fit_result.a0 = function->getParameter(0);
  fit_result.a1 = function->getParameter(1);

  double offset =
      -1. * peak_center * m_step / (m_dreference + peak_center * m_step);
  // factor := factor * (1+offset) for d-spacemap conversion so factor cannot be
  // negative

  if (isAbsolbute) {
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
IFunction_sptr GetDetectorOffsets::createFunction(const double peakHeight,
                                                  const double peakLoc) {
  FunctionFactoryImpl &creator = FunctionFactory::Instance();
  auto background = creator.createFunction("LinearBackground");
  auto peak = boost::dynamic_pointer_cast<IPeakFunction>(
      creator.createFunction(getProperty("PeakFunction")));
  peak->setHeight(peakHeight);
  peak->setCentre(peakLoc);
  const double sigma(10.0);
  peak->setFwhm(2.0 * std::sqrt(2.0 * M_LN2) * sigma);

  auto fitFunc = new CompositeFunction(); // Takes ownership of the functions
  fitFunc->addFunction(background);
  fitFunc->addFunction(peak);

  return boost::shared_ptr<IFunction>(fitFunc);
}

} // namespace Algorithm
} // namespace Mantid
