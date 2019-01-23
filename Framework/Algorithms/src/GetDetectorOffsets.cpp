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
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/TableWorkspace.h"

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

  declareProperty(
      make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          "PeakFitResultTableWorkspace", "",
          Direction::Output),
      "Name of the input Tableworkspace containing peak fit window "
      "information for each spectrum. ");

  declareProperty("OutputFitResult", false,
                  "If true, then a TableWorkspace containing all the fitted "
                  "parameters' values will be output with the name as "
                  "$OUTPUTWORKSPACE_FitResult");

  declareProperty(
      "MinimumPeakHeight", EMPTY_DBL(),
      "If it is specified and FitEachPeakTwice is specified as true "
      "then any spectrum having peak with height less this value in the "
      "first-round "
      "peak fitting will be masked.");
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
  double minimum_peak_height = getProperty("MinimumPeakHeight");

  // output fitting result?
  bool output_fit_result = getProperty("OutputFitResult");
  // TableWorkspace_sptr fit_result_table(0);
  // fit_result_table = GenerateFitResultTable();
  API::ITableWorkspace_sptr fit_result_table = GenerateFitResultTable();

  // Fit all the spectra with a gaussian
  Progress prog(this, 0.0, 1.0, nspec);
  auto &spectrumInfo = maskWS->mutableSpectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputW))
  for (int64_t wi = 0; wi < nspec; ++wi) {
    PARALLEL_START_INTERUPT_REGION
    // Fit the peak
    GetDetectorsOffset::PeakLinearFunction fit_result;
    double offset =
        fitSpectra(wi, isAbsolute, m_Xmin, m_Xmax, fit_result, false);

    // fit peak for the second time
    bool mask_it(false);
    if (fit_peak_twice) {
      double offset2(0.);
      try {
        offset2 = fitPeakSecondTime(wi, isAbsolute, minimum_peak_height,
                                         fit_result, mask_it);
      } catch (const std::string& ex) {
        g_log.error() << "Caught exception when fitting ws-index " << wi << "\n";
	throw std::runtime_error("Need to find out how to deal with this!");
      } catch (...) {
        g_log.error() << "Caught exception 2 when fitting ws-index " << wi << "\n";
	throw std::runtime_error("Need to find out how to deal with this! 2 ");
      }
      if (mask_it) {
        if (offset2 < -1.9)
          g_log.debug() << "ws-index " << wi << " has NaN.";
        else
          g_log.debug() << "ws-index " << wi << " has either too-low peak ("
                        << fit_result.height << ") or too-narrow peak ("
                        << fit_result.sigma << ")\n";
      }

      offset = offset2;
    }

    double mask = 0.0;
    if (mask_it || std::abs(offset) > m_maxOffset) {
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
      if (output_fit_result) {
        API::TableRow row = fit_result_table->getRow(wi);
        row << static_cast<int>(wi) << fit_result.center << fit_result.sigma
            << fit_result.height << fit_result.a0 << fit_result.a1;
      }
    }
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.notice("Fitting peak is done!");

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
  // Return the output fit result table
  // std::string fit_result_table_name = getPropertyValue("OutputWorkspace");
  // fit_result_table_name += "_FitResult";
  setProperty("PeakFitResultTableWorkspace", fit_result_table);
 
}

/** Fit the peak for the second time
 * @brief GetDetectorOffsets::fitPeakSecondTime
 * @param wi
 * @param isAbsolute
 * @param minimum_peak_height
 * @param fit_result
 * @param mask_it
 * @return
 */
double GetDetectorOffsets::fitPeakSecondTime(
    size_t wi, const bool isAbsolute, const double minimum_peak_height,
    GetDetectorsOffset::PeakLinearFunction &fit_result, bool &mask_it) {
  // check y
  double offset(0);
  mask_it = false;

  if (minimum_peak_height != EMPTY_DBL() &&
      fit_result.height < minimum_peak_height) {
    // doesn't meet the minumum peak height requirement
    mask_it = true;
    g_log.warning() << "ws-index " << wi << " peak height " << fit_result.height
                    << " too low!"
                    << "\n";
  } else if (fit_result.sigma < 0.5) {
    // doesn't meet the minimum peak sigma
    mask_it = true;
  }

  if (mask_it) {
    offset = -1;
    return offset;
  }

  // also check whether there is any nan in vector Y
  auto vec_y = inputW->histogram(wi).y();
  for (auto iter : vec_y) {
    if (std::isnan(iter)) {
      mask_it = true;
      break;
    }
  }
  if (mask_it) {
    // set offset to -2 to indicate it is an array with NaN in Y value
    offset = -2;
    return offset;
  }

  // fit second time!
  double xmin = fit_result.center - 0.5 * fit_result.sigma * 2.355;
  double xmax = fit_result.center + 0.5 * fit_result.sigma * 2.355;
  // g_log.debug() << "ws-index " << wi << " found: center = " << fit_result.center
  //               << ", FHWM = " << fit_result.sigma
  //               << "; new fit range: " << xmin << ", " << xmax << "\n";
  offset = fitSpectra(wi, isAbsolute, xmin, xmax, fit_result, true);
  if (offset > 1.E10) {
    g_log.error() << "ws-index " << wi << " found: center = " << fit_result.center
                  << ", FHWM = " << fit_result.sigma
                  << "; new fit range: " << xmin << ", " << xmax << "\n";
  }

  return offset;
}

//-----------------------------------------------------------------------------------------
/** Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
 *
 *  @param s :: The Workspace Index to fit
 *  @param isAbsolbute :: Whether to calculate an absolute offset
 *  @param xmin: minimum value in fit range
 *  @param xmax: maximum value in fit range
 *  @param fit_result: collection of fitting result
 *  @param use_fit_result:
 *  @return The calculated offset value
 */
double GetDetectorOffsets::fitSpectra(
    const int64_t s, bool isAbsolbute, const double xmin, const double xmax,
    GetDetectorsOffset::PeakLinearFunction &fit_result,
    const bool use_fit_result) {

  double bkgd_a0(0.), bkgd_a1(0.);
  double peakHeight(0.), peakLoc(0.), peakSigma(-1);

  if (use_fit_result) {
    bkgd_a0 = fit_result.a0;
    bkgd_a1 = fit_result.a1;
    peakHeight = fit_result.height;
    peakLoc = fit_result.center;
    peakSigma = fit_result.sigma;
  } else {
    // Find point of peak centre
    const auto &yValues = inputW->y(s);
    auto it = std::max_element(yValues.cbegin(), yValues.cend());
    peakHeight = *it;
    peakLoc = inputW->x(s)[it - yValues.begin()];
  }

  //  double peakSigma(-1);
  //  if (use_fit_result) {
  //      peakSigma = fit_result.sigma;
  //  }
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
    throw std::runtime_error("Cannot locate Fit algorithm");
  }
  auto fun = createFunction(peakHeight, peakLoc, peakSigma, bkgd_a0, bkgd_a1);
  fit_alg->setProperty("Function", fun);

  fit_alg->setProperty("InputWorkspace", inputW);
  fit_alg->setProperty<int>(
      "WorkspaceIndex",
      static_cast<int>(s)); // TODO what is the right thing to do here?
  fit_alg->setProperty("StartX", xmin);
  fit_alg->setProperty("EndX", xmax);
  fit_alg->setProperty("MaxIterations", 100);

  //  IFunction_sptr fun_ptr = createFunction(peakHeight, peakLoc);
  //  fit_alg->setProperty("Function", fun_ptr);
  try {
    fit_alg->executeAsChildAlg();
  } catch (std::invalid_argument &) {
    g_log.error() << "Input workspace: " << inputW->getName() << ". ws-index = " << s << ", xmin = " << xmin
	          << ", xmax = " << xmax << "\n";
    return (1.E20);
  }
  std::string fitStatus = fit_alg->getProperty("OutputStatus");
  // Pixel with large offset will be masked
  if (fitStatus != "success")
    return (1000.);

  // std::vector<double> params = fit_alg->getProperty("Parameters");
  API::IFunction_sptr function = fit_alg->getProperty("Function");
  // double offset = function->getParameter(3); // params[3]; // f1.PeakCentre

  // So far the function parameters are set up as thus: A0, A1, Height,
  // PeakCenter, Sigma
  std::vector<std::string> param_names = function->getParameterNames();

  if (use_fit_result) {
    std::stringstream dbss;
    dbss << "[Debug] ws-index = " << s << ". fit range: " << xmin << ", "
         << xmax << "\n";
    for (size_t i = 0; i < function->nParams(); ++i) {
      dbss << param_names[i] << "(" << i << "): " << function->getParameter(i)
           << "\n";
    }
    g_log.debug(dbss.str());
  }

  // get fitted result
  double peak_center = function->getParameter(3);
  fit_result.center = peak_center;
  fit_result.sigma = function->getParameter(4);
  fit_result.height = function->getParameter(2);
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

//-----------------------------------------------------------------------------------------
/** Create a function string from the given parameters and the algorithm inputs
 * @brief GetDetectorOffsets::createFunction
 * @param peakHeight
 * @param peakLoc
 * @param peakSigma
 * @param a0
 * @param a1
 * @return
 */
IFunction_sptr GetDetectorOffsets::createFunction(const double peakHeight,
                                                  const double peakLoc,
                                                  const double peakSigma,
                                                  const double a0,
                                                  const double a1) {
  // background
  FunctionFactoryImpl &creator = FunctionFactory::Instance();
  auto background = creator.createFunction("LinearBackground");
  background->setParameter(0, a0);
  background->setParameter(1, a1);

  // peak
  auto peak = boost::dynamic_pointer_cast<IPeakFunction>(
      creator.createFunction(getProperty("PeakFunction")));
  peak->setHeight(peakHeight);
  peak->setCentre(peakLoc);
  if (peakSigma <= 0) {
    const double sigma(10.0);
    peak->setFwhm(2.0 * std::sqrt(2.0 * M_LN2) * sigma);
  } else {
    peak->setFwhm(2.0 * std::sqrt(2.0 * M_LN2) * peakSigma);
  }

  auto fitFunc = new CompositeFunction(); // Takes ownership of the functions
  fitFunc->addFunction(background);
  fitFunc->addFunction(peak);

  return boost::shared_ptr<IFunction>(fitFunc);
}

/**
 * @brief GetDetectorOffsets::GenerateFitResultTable
 * @return
 */
ITableWorkspace_sptr GetDetectorOffsets::GenerateFitResultTable() {

  TableWorkspace_sptr anytable = boost::make_shared<DataObjects::TableWorkspace>();
  API::ITableWorkspace_sptr fit_result_table = boost::dynamic_pointer_cast<API::ITableWorkspace>(anytable);
  fit_result_table->addColumn("int", "wsindex");
  fit_result_table->addColumn("double", "center");
  fit_result_table->addColumn("double", "fwhm");
  fit_result_table->addColumn("double", "height");
  fit_result_table->addColumn("double", "A0");
  fit_result_table->addColumn("double", "A1");
  for (size_t iws = 0; iws < inputW->getNumberHistograms(); ++iws)
    fit_result_table->appendRow();

  return fit_result_table;
}

} // namespace Algorithm
} // namespace Mantid
