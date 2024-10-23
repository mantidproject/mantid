// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/AlignAndFocusPowder.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/System.h"

using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::WorkflowAlgorithms {
using namespace Kernel;
using API::FileProperty;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::WorkspaceProperty;

namespace { // anonymous namespace
namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string UNFOCUS_WKSP("UnfocussedWorkspace");
const std::string CAL_FILE("CalFileName");
const std::string GROUP_FILE("GroupFilename");
const std::string GROUP_WKSP("GroupingWorkspace");
const std::string CAL_WKSP("CalibrationWorkspace");
const std::string OFFSET_WKSP("OffsetsWorkspace");
const std::string MASK_WKSP("MaskWorkspace");
const std::string MASK_TABLE("MaskBinTable");
const std::string BINNING("Params");
const std::string RESAMPLEX("ResampleX");
const std::string BIN_IN_D("Dspacing");
const std::string D_MINS("DMin");
const std::string D_MAXS("DMax");
const std::string RAGGED_DELTA("DeltaRagged");
const std::string TOF_MIN("TMin");
const std::string TOF_MAX("TMax");
const std::string WL_MIN("CropWavelengthMin");
const std::string WL_MAX("CropWavelengthMax");
const std::string PRESERVE_EVENTS("PreserveEvents");
const std::string REMOVE_PROMPT_PULSE("RemovePromptPulseWidth");
const std::string RESONANCE_UNITS("ResonanceFilterUnits");
const std::string RESONANCE_LOWER_LIMITS("ResonanceFilterLowerLimits");
const std::string RESONANCE_UPPER_LIMITS("ResonanceFilterUpperLimits");
const std::string COMPRESS_TOF_TOL("CompressTolerance");
const std::string COMPRESS_WALL_TOL("CompressWallClockTolerance");
const std::string COMPRESS_WALL_START("CompressStartTime");
const std::string COMPRESS_MODE("CompressBinningMode");
const std::string L1("PrimaryFlightPath");
const std::string SPEC_IDS("SpectrumIDs");
const std::string L2("L2");
const std::string POLAR("Polar");
const std::string AZIMUTHAL("Azimuthal");
const std::string PM_NAME("ReductionProperties");
const std::string LORENTZ("LorentzCorrection");
const std::string UNWRAP_REF("UnwrapRef");
const std::string LOWRES_REF("LowResRef");
const std::string LOWRES_SPEC_OFF("LowResSpectrumOffset");
} // namespace PropertyNames

void getTofRange(const MatrixWorkspace_const_sptr &wksp, double &tmin, double &tmax) {
  if (const auto eventWksp = std::dynamic_pointer_cast<const EventWorkspace>(wksp)) {
    eventWksp->getEventXMinMax(tmin, tmax);
  } else {
    wksp->getXMinMax(tmin, tmax);
  }
}

const std::vector<std::string> binningModeNames{"Default", "Linear", "Logarithmic"};
enum class BinningMode { DEFAULT, LINEAR, LOGARITHMIC, enum_count };
typedef Mantid::Kernel::EnumeratedString<BinningMode, &binningModeNames> BINMODE;

} // anonymous namespace

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AlignAndFocusPowder)

//----------------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
 */
void AlignAndFocusPowder::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
                  "The input workspace");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "The result of diffraction focussing of InputWorkspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::UNFOCUS_WKSP, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Treated data in d-spacing before focussing (optional). This will likely "
                  "need rebinning.");
  // declareProperty(
  //   new WorkspaceProperty<MatrixWorkspace>("LowResTOFWorkspace", "",
  //   Direction::Output, PropertyMode::Optional),
  //   "The name of the workspace containing the filtered low resolution TOF
  //   data.");
  declareProperty(std::make_unique<FileProperty>(PropertyNames::CAL_FILE, "", FileProperty::OptionalLoad,
                                                 std::vector<std::string>{".h5", ".hd5", ".hdf", ".cal"}),
                  "The name of the calibration file with offset, masking, and "
                  "grouping data");
  declareProperty(std::make_unique<FileProperty>(PropertyNames::GROUP_FILE, "", FileProperty::OptionalLoad,
                                                 std::vector<std::string>{".xml", ".cal"}),
                  "Overrides grouping from CalFileName");
  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>(PropertyNames::GROUP_WKSP, "",
                                                                         Direction::InOut, PropertyMode::Optional),
                  "Optional: A GroupingWorkspace giving the grouping info.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(PropertyNames::CAL_WKSP, "", Direction::InOut,
                                                                       PropertyMode::Optional),
                  "Optional: A Workspace containing the calibration information. Either "
                  "this or CalibrationFile needs to be specified.");
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>(PropertyNames::OFFSET_WKSP, "",
                                                                        Direction::Input, PropertyMode::Optional),
                  "Optional: An OffsetsWorkspace giving the detector calibration values.");
  declareProperty(std::make_unique<WorkspaceProperty<MaskWorkspace>>(PropertyNames::MASK_WKSP, "", Direction::InOut,
                                                                     PropertyMode::Optional),
                  "Optional: A workspace giving which detectors are masked.");
  declareProperty(
      std::make_unique<WorkspaceProperty<TableWorkspace>>("MaskBinTable", "", Direction::Input, PropertyMode::Optional),
      "Optional: A workspace giving pixels and bins to mask.");
  declareProperty( // intentionally not using the RebinParamsValidator
      std::make_unique<ArrayProperty<double>>(PropertyNames::BINNING),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally\n"
      "this can be followed by a comma and more widths and last boundary "
      "pairs.\n"
      "Negative width values indicate logarithmic binning.");
  declareProperty(PropertyNames::RESAMPLEX, 0,
                  "Number of bins in x-axis. Non-zero value "
                  "overrides \"Params\" property. Negative "
                  "value means logarithmic binning.");
  setPropertySettings(PropertyNames::BINNING,
                      std::make_unique<EnabledWhenProperty>(PropertyNames::RESAMPLEX, IS_DEFAULT));
  declareProperty(PropertyNames::BIN_IN_D, true, "Bin in Dspace. (True is Dspace; False is TOF)");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::D_MINS),
                  "Minimum for Dspace axis. (Default 0.) ");
  mapPropertyName(PropertyNames::D_MINS, "d_min");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::D_MAXS),
                  "Maximum for Dspace axis. (Default 0.) ");
  mapPropertyName(PropertyNames::D_MAXS, "d_max");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::RAGGED_DELTA), "Step parameter for rebin");
  mapPropertyName(PropertyNames::RAGGED_DELTA, "delta");
  declareProperty(PropertyNames::TOF_MIN, EMPTY_DBL(), "Minimum for TOF axis. Defaults to 0. ");
  mapPropertyName(PropertyNames::TOF_MIN, "tof_min");
  declareProperty(PropertyNames::TOF_MAX, EMPTY_DBL(), "Maximum for TOF or dspace axis. Defaults to 0. ");
  mapPropertyName(PropertyNames::TOF_MAX, "tof_max");
  declareProperty(PropertyNames::PRESERVE_EVENTS, true,
                  "If the InputWorkspace is an "
                  "EventWorkspace, this will preserve "
                  "the full event list (warning: this "
                  "will use much more memory!).");
  declareProperty(PropertyNames::REMOVE_PROMPT_PULSE, 0.,
                  "Width of events (in "
                  "microseconds) near the prompt "
                  "pulse to remove. 0 disables");
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  std::vector<std::string> allowedResonanceUnits({"Energy", "Wavelength"});
  declareProperty(PropertyNames::RESONANCE_UNITS, allowedResonanceUnits.back(),
                  std::make_shared<StringListValidator>(allowedResonanceUnits),
                  "Units for resonances to be filtered in. "
                  "The data will be converted to these units temporarily to filter.");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::RESONANCE_LOWER_LIMITS),
                  "Minimum values to filter absorption resonance. This must have same number of values as "
                  "ResonanceFilterUpperLimits. Default behavior is to not filter.");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::RESONANCE_UPPER_LIMITS),
                  "Maximum values to filter absorption resonance. This must have same number of values as "
                  "ResonanceFilterLowerLimits. Default behavior is to not filter.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(PropertyNames::COMPRESS_TOF_TOL, 1e-5, Direction::Input),
                  "Compress events (in microseconds) within this tolerance. (Default 1e-5). If negative then do "
                  "logorithmic compression.");
  declareProperty(std::make_unique<PropertyWithValue<double>>(PropertyNames::COMPRESS_WALL_TOL, EMPTY_DBL(),
                                                              mustBePositive, Direction::Input),
                  "The tolerance (in seconds) on the wall-clock time for comparison. Unset "
                  "means compressing all wall-clock times together disabling pulsetime "
                  "resolution.");
  auto dateValidator = std::make_shared<DateTimeValidator>();
  dateValidator->allowEmpty(true);
  declareProperty(PropertyNames::COMPRESS_WALL_START, "", dateValidator,
                  "An ISO formatted date/time string specifying the timestamp for "
                  "starting filtering. Ignored if WallClockTolerance is not specified. "
                  "Default is start of run",
                  Direction::Input);
  declareProperty(PropertyNames::COMPRESS_MODE, binningModeNames[size_t(BinningMode::DEFAULT)],
                  std::make_shared<Mantid::Kernel::StringListValidator>(binningModeNames),
                  "Optional.  Binning behavior can be specified in the usual way through sign of tolerance "
                  "('Default'); or can be set to one of the allowed binning modes. ");
  declareProperty(PropertyNames::LORENTZ, false,
                  "Multiply each spectrum by "
                  "sin(theta) where theta is "
                  "half of the Bragg angle");
  declareProperty(PropertyNames::UNWRAP_REF, 0.,
                  "Reference total flight path for frame "
                  "unwrapping. Zero skips the correction");
  declareProperty(PropertyNames::LOWRES_REF, 0., "Reference DIFC for resolution removal. Zero skips the correction");
  declareProperty("CropWavelengthMin", 0., "Crop the data at this minimum wavelength. Overrides LowResRef.");
  mapPropertyName(PropertyNames::WL_MIN, "wavelength_min");
  declareProperty("CropWavelengthMax", EMPTY_DBL(),
                  "Crop the data at this maximum wavelength. Forces use of "
                  "CropWavelengthMin.");
  mapPropertyName(PropertyNames::WL_MAX, "wavelength_max");
  declareProperty(PropertyNames::L1, -1.0, "If positive, focus positions are changed.  (Default -1) ");
  declareProperty(std::make_unique<ArrayProperty<int32_t>>(PropertyNames::SPEC_IDS),
                  "Optional: Spectrum Nos (note that it is not detector ID or "
                  "workspace indices).");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::L2),
                  "Optional: Secondary flight (L2) paths for each detector");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::POLAR),
                  "Optional: Polar angles (two thetas) for detectors");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::AZIMUTHAL),
                  "Azimuthal angles (out-of-plain) for detectors");

  declareProperty(PropertyNames::LOWRES_SPEC_OFF, -1,
                  "Offset on spectrum No of low resolution spectra from high "
                  "resolution one. "
                  "If negative, then all the low resolution TOF will not be "
                  "processed.  Otherwise, low resolution TOF "
                  "will be stored in an additional set of spectra. "
                  "If offset is equal to 0, then the low resolution will have "
                  "same spectrum Nos as the normal ones.  "
                  "Otherwise, the low resolution spectra will have spectrum "
                  "IDs offset from normal ones. ");
  declareProperty(PropertyNames::PM_NAME, "__powdereduction", Direction::Input);
}

std::map<std::string, std::string> AlignAndFocusPowder::validateInputs() {
  std::map<std::string, std::string> result;

  if (!isDefault(PropertyNames::UNFOCUS_WKSP)) {
    if (getPropertyValue(PropertyNames::OUTPUT_WKSP) == getPropertyValue(PropertyNames::UNFOCUS_WKSP)) {
      result[PropertyNames::OUTPUT_WKSP] = "Cannot be the same as UnfocussedWorkspace";
      result[PropertyNames::UNFOCUS_WKSP] = "Cannot be the same as OutputWorkspace";
    }
  }

  if ((!isDefault(PropertyNames::RAGGED_DELTA)) && (!isDefault(PropertyNames::RESAMPLEX))) {
    result[PropertyNames::RAGGED_DELTA] = "Cannot specify with " + PropertyNames::RESAMPLEX;
    result[PropertyNames::RESAMPLEX] = "Cannot specify with " + PropertyNames::RAGGED_DELTA;
  }

  m_inputW = getProperty(PropertyNames::INPUT_WKSP);
  auto inputEW = std::dynamic_pointer_cast<EventWorkspace>(m_inputW);
  if (inputEW && inputEW->getNumberEvents() <= 0)
    result[PropertyNames::INPUT_WKSP] = "Empty workspace encounter, possibly due to beam down."
                                        "Please plot the pCharge-time to identify suitable range for "
                                        "re-time-slicing";

  m_resonanceLower = getProperty(PropertyNames::RESONANCE_LOWER_LIMITS);
  m_resonanceUpper = getProperty(PropertyNames::RESONANCE_UPPER_LIMITS);
  // verify that they are the same length
  if (m_resonanceLower.size() == m_resonanceUpper.size()) {
    // verify that the lowers are less than the uppers
    const size_t NUM_WINDOWS = m_resonanceLower.size();
    bool ok = true;
    for (size_t i = 0; i < NUM_WINDOWS; ++i) {
      if (m_resonanceLower[i] >= m_resonanceUpper[i])
        ok = false;
    }
    if (!ok) {
      const std::string msg = "Lower limits must be less than upper limits";
      result[PropertyNames::RESONANCE_LOWER_LIMITS] = msg;
      result[PropertyNames::RESONANCE_UPPER_LIMITS] = msg;
    }
  } else {
    result[PropertyNames::RESONANCE_LOWER_LIMITS] =
        "Must have same number of values as " + PropertyNames::RESONANCE_UPPER_LIMITS;
    result[PropertyNames::RESONANCE_UPPER_LIMITS] =
        "Must have same number of values as " + PropertyNames::RESONANCE_LOWER_LIMITS;
  }

  return result;
}

template <typename NumT> struct RegLowVectorPair {
  std::vector<NumT> reg;
  std::vector<NumT> low;
};

template <typename NumT>
RegLowVectorPair<NumT> splitVectors(const std::vector<NumT> &orig, const size_t numVal, const std::string &label) {
  RegLowVectorPair<NumT> out;

  // check that there is work to do
  if (!orig.empty()) {
    // do the spliting
    if (orig.size() == numVal) {
      out.reg.assign(orig.begin(), orig.end());
      out.low.assign(orig.begin(), orig.end());
    } else if (orig.size() == 2 * numVal) {
      out.reg.assign(orig.begin(), orig.begin() + numVal);
      out.low.assign(orig.begin() + numVal, orig.begin());
    } else {
      std::stringstream msg;
      msg << "Input number of " << label << " ids is not equal to "
          << "the number of histograms or empty (" << orig.size() << " != 0 or " << numVal << " or " << (2 * numVal)
          << ")";
      throw std::runtime_error(msg.str());
    }
  }
  return out;
}

//----------------------------------------------------------------------------------------------
/**
 * Function to get a vector property either from a PropertyManager or the
 * algorithm
 * properties. If both PM and algorithm properties are specified, the algorithm
 * one wins.
 * The return value is the first element in the vector if it is not empty.
 * @param name : The algorithm property to retrieve.
 * @param avec : The vector to hold the property value.
 * @return : The default value of the requested property.
 */
double AlignAndFocusPowder::getVecPropertyFromPmOrSelf(const std::string &name, std::vector<double> &avec) {
  avec = getProperty(name);
  if (!avec.empty()) {
    return avec[0];
  }
  // No overrides provided.
  return 0.0;
}

//----------------------------------------------------------------------------------------------
/** Executes the algorithm
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 * successfully
 *  @throw runtime_error If unable to run one of the Child Algorithms
 * successfully
 */
void AlignAndFocusPowder::exec() {
  // retrieve the properties
  m_inputW = getProperty(PropertyNames::INPUT_WKSP);
  m_instName = m_inputW->getInstrument()->getName();
  try {
    m_instName = Kernel::ConfigService::Instance().getInstrument(m_instName).shortName();
  } catch (Exception::NotFoundError &) {
    ; // not noteworthy
  }
  std::string calFilename = getPropertyValue(PropertyNames::CAL_FILE);
  std::string groupFilename = getPropertyValue(PropertyNames::GROUP_FILE);
  m_calibrationWS = getProperty(PropertyNames::CAL_WKSP);
  m_maskWS = getProperty(PropertyNames::MASK_WKSP);
  m_groupWS = getProperty(PropertyNames::GROUP_WKSP);
  DataObjects::TableWorkspace_sptr maskBinTableWS = getProperty(PropertyNames::MASK_TABLE);
  m_l1 = getProperty(PropertyNames::L1);
  specids = getProperty(PropertyNames::SPEC_IDS);
  l2s = getProperty(PropertyNames::L2);
  tths = getProperty(PropertyNames::POLAR);
  phis = getProperty(PropertyNames::AZIMUTHAL);
  m_params = getProperty(PropertyNames::BINNING);
  binInDspace = getProperty(PropertyNames::BIN_IN_D);
  auto dmin = getVecPropertyFromPmOrSelf(PropertyNames::D_MINS, m_dmins);
  auto dmax = getVecPropertyFromPmOrSelf(PropertyNames::D_MAXS, m_dmaxs);
  this->getVecPropertyFromPmOrSelf(PropertyNames::RAGGED_DELTA, m_delta_ragged);
  LRef = getProperty(PropertyNames::UNWRAP_REF);
  DIFCref = getProperty(PropertyNames::LOWRES_REF);
  const bool applyLorentz = getProperty(PropertyNames::LORENTZ);
  minwl = getProperty(PropertyNames::WL_MIN);
  maxwl = getProperty(PropertyNames::WL_MAX);
  if (maxwl == 0.)
    maxwl = EMPTY_DBL(); // python can only specify 0 for unused
  tmin = getProperty(PropertyNames::TOF_MIN);
  tmax = getProperty(PropertyNames::TOF_MAX);
  m_preserveEvents = getProperty(PropertyNames::PRESERVE_EVENTS);
  m_resampleX = getProperty(PropertyNames::RESAMPLEX);
  double compressEventsTolerance = getProperty(PropertyNames::COMPRESS_TOF_TOL);
  const double wallClockTolerance = getProperty(PropertyNames::COMPRESS_WALL_TOL);
  const BINMODE mode = getPropertyValue(PropertyNames::COMPRESS_MODE);
  if (mode == BinningMode::LINEAR)
    compressEventsTolerance = std::fabs(compressEventsTolerance);
  else if (mode == BinningMode::LOGARITHMIC)
    compressEventsTolerance = -1. * std::fabs(compressEventsTolerance);

  // determine some bits about d-space and binning
  if (m_resampleX != 0) {
    // ignore the normal rebin parameters
    m_params.clear();
  } else if (m_params.size() == 1 && m_delta_ragged.empty()) {
    // if there is 1 binning parameter and not in ragged rebinning mode
    // ignore what people asked for
    binInDspace = bool(dmax > 0.);
  }
  if (binInDspace) {
    if (m_params.size() == 1 && (!isEmpty(dmin)) && (!isEmpty(dmax))) {
      if (dmin > 0. && dmax > dmin) {
        double step = m_params[0];
        m_params.clear();
        m_params.emplace_back(dmin);
        m_params.emplace_back(step);
        m_params.emplace_back(dmax);
        g_log.information() << "d-Spacing binning updated: " << m_params[0] << "  " << m_params[1] << "  "
                            << m_params[2] << "\n";
      } else {
        g_log.warning() << "something is wrong with dmin (" << dmin << ") and dmax (" << dmax
                        << "). They are being ignored.\n";
      }
    }
  } else {
    if (m_params.size() == 1 && (!isEmpty(tmin)) && (!isEmpty(tmax))) {
      if (tmin > 0. && tmax > tmin) {
        double step = m_params[0];
        m_params[0] = tmin;
        m_params.emplace_back(step);
        m_params.emplace_back(tmax);
        g_log.information() << "TOF binning updated: " << m_params[0] << "  " << m_params[1] << "  " << m_params[2]
                            << "\n";
      } else {
        g_log.warning() << "something is wrong with tmin (" << tmin << ") and tmax (" << tmax
                        << "). They are being ignored.\n";
      }
    }
  }
  xmin = 0.;
  xmax = 0.;
  if (tmin > 0.) {
    xmin = tmin;
  }
  if (tmax > 0.) {
    xmax = tmax;
  }
  if (!binInDspace && m_params.size() == 3) {
    xmin = m_params[0];
    xmax = m_params[2];
  }

  // Low resolution
  int lowresoffset = getProperty(PropertyNames::LOWRES_SPEC_OFF);
  if (lowresoffset < 0) {
    m_processLowResTOF = false;
  } else {
    m_processLowResTOF = true;
    m_lowResSpecOffset = static_cast<size_t>(lowresoffset);
  }

  loadCalFile(calFilename, groupFilename);

  // Now setup the output workspace
  m_outputW = getProperty(PropertyNames::OUTPUT_WKSP);
  if (auto inputEW = std::dynamic_pointer_cast<EventWorkspace>(m_inputW)) {
    // event workspace
    if (m_outputW != m_inputW) {
      // out-of-place: clone the input EventWorkspace
      EventWorkspace_sptr outputEW = inputEW->clone();
      m_outputW = std::dynamic_pointer_cast<MatrixWorkspace>(outputEW);
    } // in-place doesn't require anything
  } else {
    // workspace2D
    if (m_outputW != m_inputW) {
      m_outputW = m_inputW->clone();
    }
  }

  if (m_processLowResTOF) {
    if (auto inputEW = std::dynamic_pointer_cast<EventWorkspace>(m_inputW)) {
      // Make a brand new EventWorkspace
      m_lowResEW = std::dynamic_pointer_cast<EventWorkspace>(
          WorkspaceFactory::Instance().create("EventWorkspace", inputEW->getNumberHistograms(), 2, 1));

      // Cast to the matrixOutputWS and save it
      m_lowResW = std::dynamic_pointer_cast<MatrixWorkspace>(m_lowResEW);
      // m_lowResW->setName(lowreswsname);
    } else {
      throw std::runtime_error("Input workspace is not EventWorkspace.  It is not supported now.");
    }
  }

  // set up a progress bar with the "correct" number of steps
  m_progress = std::make_unique<Progress>(this, 0., 1., 22);

  if (m_maskWS) {
    g_log.information() << "running MaskDetectors started at " << Types::Core::DateAndTime::getCurrentTime() << "\n";

    API::IAlgorithm_sptr maskDetAlg = createChildAlgorithm("MaskDetectors");
    // cast to Workspace for MaksDetectors alg
    Workspace_sptr outputw = std::dynamic_pointer_cast<Workspace>(m_outputW);
    maskDetAlg->setProperty("Workspace", outputw);
    MatrixWorkspace_sptr mksws = std::dynamic_pointer_cast<MatrixWorkspace>(m_maskWS);
    maskDetAlg->setProperty("MaskedWorkspace", mksws);
    maskDetAlg->executeAsChildAlg();
    outputw = maskDetAlg->getProperty("Workspace");
    // casting
    m_outputW = std::dynamic_pointer_cast<MatrixWorkspace>(outputw);
  }
  m_progress->report();

  // hold onto over tof range for CropWorkspace(tof) and RemovePromptPulse
  double tofmin = EMPTY_DBL();
  double tofmax = EMPTY_DBL();

  // crop the workspace in time-of-flight
  if (((!isEmpty(xmin)) && (xmin >= 0.)) || ((!isEmpty(xmax)) && (xmax > 0.))) {
    getTofRange(m_outputW, tofmin, tofmax);

    API::IAlgorithm_sptr cropAlg = createChildAlgorithm("CropWorkspace");
    cropAlg->setProperty("InputWorkspace", m_outputW);
    cropAlg->setProperty("OutputWorkspace", m_outputW);
    bool setxmin = false;
    if ((xmin >= 0.) && (xmin > tofmin)) {
      cropAlg->setProperty("Xmin", xmin);
      tofmin = xmin; // increase value
      setxmin = true;
    }
    bool setxmax = false;
    if ((xmax > 0.) && (xmax < tofmax)) {
      cropAlg->setProperty("Xmax", xmax);
      tofmax = xmax;
      setxmax = true;
    }
    // only run if either xmin or xmax was set
    if (setxmin || setxmax) {
      g_log.information() << "running CropWorkspace(TOFmin=" << xmin << ", TOFmax=" << xmax << ") started at "
                          << Types::Core::DateAndTime::getCurrentTime() << "\n";
      cropAlg->executeAsChildAlg();
      m_outputW = cropAlg->getProperty("OutputWorkspace");
    }
  }
  m_progress->report();

  // filter the input events if appropriate
  const double removePromptPulseWidth = getProperty(PropertyNames::REMOVE_PROMPT_PULSE);
  if (removePromptPulseWidth > 0.) {
    bool removePromptPulse(false);
    if (auto outputEW = std::dynamic_pointer_cast<EventWorkspace>(m_outputW)) {
      removePromptPulse = (outputEW->getNumberEvents() > 0);
    }
    if (removePromptPulse) {
      g_log.information() << "running RemovePromptPulse(Width=" << removePromptPulseWidth << ") started at "
                          << Types::Core::DateAndTime::getCurrentTime() << "\n";
      API::IAlgorithm_sptr filterPAlg = createChildAlgorithm("RemovePromptPulse");
      filterPAlg->setProperty("InputWorkspace", m_outputW);
      filterPAlg->setProperty("OutputWorkspace", m_outputW);
      filterPAlg->setProperty("Width", removePromptPulseWidth);

      // if some of the range was known in CropWorkspace-TOF, use it again here
      // they default to EMPTY_DBL which the alg interprets as unset
      filterPAlg->setProperty("TMin", tofmin);
      filterPAlg->setProperty("TMax", tofmax);

      filterPAlg->executeAsChildAlg();
      m_outputW = filterPAlg->getProperty("OutputWorkspace");
    } else {
      g_log.information("skipping RemovePromptPulse on empty EventWorkspace");
    }
  }
  m_progress->report();

  if (maskBinTableWS) {
    g_log.information() << "running MaskBinsFromTable started at " << Types::Core::DateAndTime::getCurrentTime()
                        << "\n";
    API::IAlgorithm_sptr alg = createChildAlgorithm("MaskBinsFromTable");
    alg->setProperty("InputWorkspace", m_outputW);
    alg->setProperty("OutputWorkspace", m_outputW);
    alg->setProperty("MaskingInformation", maskBinTableWS);
    alg->executeAsChildAlg();
    m_outputW = alg->getProperty("OutputWorkspace");
  }
  m_progress->report();

  // do a calculation to determine if compressing the un-focussed data will reduce data size
  if (shouldCompressUnfocused(compressEventsTolerance, tofmin, tofmax, !isEmpty(wallClockTolerance))) {
    compressEventsOutputWS(compressEventsTolerance, wallClockTolerance);
  }

  if (!binInDspace)
    m_outputW = rebin(m_outputW);
  m_progress->report();

  if (m_calibrationWS) {
    // ApplyDiffCal and update m_outputW
    g_log.information() << "apply calibration workspace to input workspace at "
                        << Types::Core::DateAndTime::getCurrentTime() << "\n";
    Workspace_sptr outputw = std::dynamic_pointer_cast<Workspace>(m_outputW);
    API::IAlgorithm_sptr applyDiffCalAlg = createChildAlgorithm("ApplyDiffCal");
    applyDiffCalAlg->setProperty("InstrumentWorkspace", outputw);
    applyDiffCalAlg->setProperty("CalibrationWorkspace", m_calibrationWS);
    applyDiffCalAlg->executeAsChildAlg();
    // grab and cast
    outputw = applyDiffCalAlg->getProperty("InstrumentWorkspace");
    m_outputW = std::dynamic_pointer_cast<MatrixWorkspace>(outputw);
  }

  m_progress->report();

  m_outputW = convertUnits(m_outputW, "dSpacing");
  m_progress->report();

  if (m_calibrationWS) {
    // NOTE:
    //  The conventional workflow for AlignAndFocusPowder allows users to modify the instrument so that the averaged
    //  pixel position can be used when converting back to TOF.
    //  With the recent changes in Unit.h, Mantid is using the averaged DIFC attached to workspace to convert from
    //  d-spacing to TOF by default, which unfortunately breaks the intended workflow here.
    //  To bypass this issue, we are going to remove the attached paramter map so taht Unit.h cannot perform the default
    //  conversion, which will effectively forcing Mantid to revert back to the original intended method.
    Workspace_sptr outputw = std::dynamic_pointer_cast<Workspace>(m_outputW);
    API::IAlgorithm_sptr applyDiffCalAlg = createChildAlgorithm("ApplyDiffCal");
    applyDiffCalAlg->setProperty("InstrumentWorkspace", outputw);
    applyDiffCalAlg->setProperty("ClearCalibration", true);
    applyDiffCalAlg->executeAsChildAlg();
    // grab and cast
    outputw = applyDiffCalAlg->getProperty("InstrumentWorkspace");
    m_outputW = std::dynamic_pointer_cast<MatrixWorkspace>(outputw);
  }

  // filter out absorption resonances
  if (!m_resonanceLower.empty()) {
    m_outputW = filterResonances(m_outputW);
  }
  m_progress->report(); // the step wil be really fast if the option isn't selected

  // ----------------- WACKY LORENTZ THING HERE
  // TODO should call LorentzCorrection as a sub-algorithm
  if (applyLorentz) {
    g_log.information() << "Applying Lorentz correction started at " << Types::Core::DateAndTime::getCurrentTime()
                        << "\n";

    API::IAlgorithm_sptr alg = createChildAlgorithm("LorentzCorrection");
    alg->setProperty("InputWorkspace", m_outputW);
    alg->setProperty("OutputWorkspace", m_outputW);
    alg->setPropertyValue("Type", "PowderTOF");
    alg->executeAsChildAlg();
    m_outputW = alg->getProperty("OutputWorkspace");
  }

  m_progress->report();

  // Beyond this point, low resolution TOF workspace is considered.
  if (LRef > 0.) {
    // this algorithm was originally created for POWGEN before issues in the DAS were fixed
    // it is not used in any current reduction and remains for legacy data processing
    m_outputW = convertUnits(m_outputW, "TOF");

    g_log.information() << "running UnwrapSNS(LRef=" << LRef << ",Tmin=" << tmin << ",Tmax=" << tmax << ") started at "
                        << Types::Core::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr removeAlg = createChildAlgorithm("UnwrapSNS");
    removeAlg->setProperty("InputWorkspace", m_outputW);
    removeAlg->setProperty("OutputWorkspace", m_outputW);
    removeAlg->setProperty("LRef", LRef);
    if (tmin > 0.)
      removeAlg->setProperty("Tmin", tmin);
    if (tmax > tmin)
      removeAlg->setProperty("Tmax", tmax);
    removeAlg->executeAsChildAlg();
    m_outputW = removeAlg->getProperty("OutputWorkspace");
  }
  m_progress->report();

  if (minwl > 0. || (!isEmpty(maxwl))) { // just crop the workspace
    // turn off the low res stuff
    m_processLowResTOF = false;

    if (const auto ews = std::dynamic_pointer_cast<EventWorkspace>(m_outputW))
      g_log.information() << "Number of events = " << ews->getNumberEvents() << ".\n";

    m_outputW = convertUnits(m_outputW, "Wavelength");

    g_log.information() << "running CropWorkspace(WavelengthMin=" << minwl;
    if (!isEmpty(maxwl))
      g_log.information() << ", WavelengthMax=" << maxwl;
    g_log.information() << ") started at " << Types::Core::DateAndTime::getCurrentTime() << "\n";

    API::IAlgorithm_sptr removeAlg = createChildAlgorithm("CropWorkspace");
    removeAlg->setProperty("InputWorkspace", m_outputW);
    removeAlg->setProperty("OutputWorkspace", m_outputW);
    removeAlg->setProperty("XMin", minwl);
    removeAlg->setProperty("XMax", maxwl);
    removeAlg->executeAsChildAlg();
    m_outputW = removeAlg->getProperty("OutputWorkspace");
    if (const auto ews = std::dynamic_pointer_cast<EventWorkspace>(m_outputW))
      g_log.information() << "Number of events = " << ews->getNumberEvents() << ".\n";
  } else if (DIFCref > 0.) {
    m_outputW = convertUnits(m_outputW, "TOF");
    // this correction has some assumptions on the events being compressed
    compressEventsOutputWS(compressEventsTolerance, wallClockTolerance);

    // this is a legacy way for describing the minimum wavelength to remove from the data
    // it is uncommon that it is used
    g_log.information() << "running RemoveLowResTof(RefDIFC=" << DIFCref << ",K=3.22) started at "
                        << Types::Core::DateAndTime::getCurrentTime() << "\n";
    if (const auto ews = std::dynamic_pointer_cast<EventWorkspace>(m_outputW))
      g_log.information() << "Number of events = " << ews->getNumberEvents() << ".\n";

    API::IAlgorithm_sptr removeAlg = createChildAlgorithm("RemoveLowResTOF");
    removeAlg->setProperty("InputWorkspace", m_outputW);
    removeAlg->setProperty("OutputWorkspace", m_outputW);
    removeAlg->setProperty("ReferenceDIFC", DIFCref);
    removeAlg->setProperty("K", 3.22);
    if (tmin > 0.)
      removeAlg->setProperty("Tmin", tmin);
    if (m_processLowResTOF)
      removeAlg->setProperty("LowResTOFWorkspace", m_lowResW);

    removeAlg->executeAsChildAlg();
    m_outputW = removeAlg->getProperty("OutputWorkspace");
    if (m_processLowResTOF)
      m_lowResW = removeAlg->getProperty("LowResTOFWorkspace");
  }
  m_progress->report();

  if (const auto ews = std::dynamic_pointer_cast<EventWorkspace>(m_outputW)) {
    const size_t numhighevents = ews->getNumberEvents();
    if (m_processLowResTOF) {
      EventWorkspace_sptr lowes = std::dynamic_pointer_cast<EventWorkspace>(m_lowResW);
      const size_t numlowevents = lowes->getNumberEvents();
      g_log.information() << "Number of high TOF events = " << numhighevents << "; "
                          << "Number of low TOF events = " << numlowevents << ".\n";
    }
  }
  m_progress->report();

  // Convert units
  if (LRef > 0. || minwl > 0. || DIFCref > 0. || (!isEmpty(maxwl))) {
    m_outputW = convertUnits(m_outputW, "dSpacing");
    if (m_processLowResTOF)
      m_lowResW = convertUnits(m_lowResW, "dSpacing");
  }
  m_progress->report();

  if (binInDspace) {
    m_outputW = rebin(m_outputW);
    if (m_processLowResTOF)
      m_lowResW = rebin(m_lowResW);
  }
  m_progress->report();

  // copy the output workspace just before `DiffractionFocusing`
  // this probably should be binned by callers before inspecting
  if (!isDefault("UnfocussedWorkspace")) {
    auto wkspCopy = m_outputW->clone();
    setProperty("UnfocussedWorkspace", std::move(wkspCopy));
  }

  if (binInDspace && m_resampleX == 0 && !m_delta_ragged.empty() && !m_dmins.empty() && !m_dmaxs.empty()) {
    // Special case where we can do ragged rebin within diffraction focus
    m_outputW = diffractionFocusRaggedRebinInDspace(m_outputW);
    if (m_processLowResTOF)
      m_lowResW = diffractionFocusRaggedRebinInDspace(m_lowResW);
    m_progress->report();
  } else {
    // Diffraction focus
    m_outputW = diffractionFocus(m_outputW);
    if (m_processLowResTOF)
      m_lowResW = diffractionFocus(m_lowResW);
    m_progress->report();

    // this next call should probably be in for rebin as well
    // but it changes the system tests
    if (binInDspace) {
      if (m_resampleX != 0.) {
        m_outputW = rebin(m_outputW);
        if (m_processLowResTOF)
          m_lowResW = rebin(m_lowResW);
      } else if (!m_delta_ragged.empty()) {
        m_outputW = rebinRagged(m_outputW, true);
        if (m_processLowResTOF)
          m_lowResW = rebinRagged(m_lowResW, true);
      }
    }
  }
  m_progress->report();

  // edit the instrument geometry
  if (m_groupWS && (m_l1 > 0 || !tths.empty() || !l2s.empty() || !phis.empty())) {
    size_t numreg = m_outputW->getNumberHistograms();

    try {
      // set up the vectors for doing everything
      auto specidsSplit = splitVectors(specids, numreg, "specids");
      auto tthsSplit = splitVectors(tths, numreg, "two-theta");
      auto l2sSplit = splitVectors(l2s, numreg, "L2");
      auto phisSplit = splitVectors(phis, numreg, "phi");

      // Edit instrument
      m_outputW = editInstrument(m_outputW, tthsSplit.reg, specidsSplit.reg, l2sSplit.reg, phisSplit.reg);

      if (m_processLowResTOF) {
        m_lowResW = editInstrument(m_lowResW, tthsSplit.low, specidsSplit.low, l2sSplit.low, phisSplit.low);
      }
    } catch (std::runtime_error &e) {
      g_log.warning("Not editing instrument geometry:");
      g_log.warning(e.what());
    }
  }
  m_progress->report();

  // Conjoin 2 workspaces if there is low resolution
  if (m_processLowResTOF) {
    m_outputW = conjoinWorkspaces(m_outputW, m_lowResW, m_lowResSpecOffset);
  }
  m_progress->report();

  // Convert units to TOF
  m_outputW = convertUnits(m_outputW, "TOF");
  m_progress->report();

  // compress again if appropriate
  compressEventsOutputWS(compressEventsTolerance, wallClockTolerance);
  m_progress->report();

  if (!binInDspace && !m_delta_ragged.empty()) {
    m_outputW = rebinRagged(m_outputW, false);
  }

  // return the output workspace
  setProperty("OutputWorkspace", m_outputW);
}

//----------------------------------------------------------------------------------------------
/** Call edit instrument geometry
 */
API::MatrixWorkspace_sptr AlignAndFocusPowder::editInstrument(API::MatrixWorkspace_sptr ws,
                                                              const std::vector<double> &polars,
                                                              const std::vector<specnum_t> &specids,
                                                              const std::vector<double> &l2s,
                                                              const std::vector<double> &phis) {
  g_log.information() << "running EditInstrumentGeometry started at " << Types::Core::DateAndTime::getCurrentTime()
                      << "\n";

  API::IAlgorithm_sptr editAlg = createChildAlgorithm("EditInstrumentGeometry");
  editAlg->setProperty("Workspace", ws);
  if (m_l1 > 0.)
    editAlg->setProperty("PrimaryFlightPath", m_l1);
  if (!polars.empty())
    editAlg->setProperty("Polar", polars);
  if (!specids.empty())
    editAlg->setProperty("SpectrumIDs", specids);
  if (!l2s.empty())
    editAlg->setProperty("L2", l2s);
  if (!phis.empty())
    editAlg->setProperty("Azimuthal", phis);
  editAlg->executeAsChildAlg();

  ws = editAlg->getProperty("Workspace");

  return ws;
}

//----------------------------------------------------------------------------------------------
/** Call diffraction focus to a matrix workspace.
 */
API::MatrixWorkspace_sptr AlignAndFocusPowder::diffractionFocus(API::MatrixWorkspace_sptr ws) {
  if (!m_groupWS) {
    g_log.information() << "not focussing data\n";
    return ws;
  }

  // cannot convert to histogram if running with ragged bins
  // otherwise the data is a histogram *before* the correct binning is set
  const bool preserveEvents = m_delta_ragged.empty() ? m_preserveEvents : true;

  g_log.information() << "running DiffractionFocussing(PreserveEvents=" << preserveEvents << ") started at "
                      << Types::Core::DateAndTime::getCurrentTime() << "\n";

  API::IAlgorithm_sptr focusAlg = createChildAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", ws);
  focusAlg->setProperty("OutputWorkspace", ws);
  focusAlg->setProperty("GroupingWorkspace", m_groupWS);
  focusAlg->setProperty("PreserveEvents", preserveEvents);
  focusAlg->executeAsChildAlg();
  ws = focusAlg->getProperty("OutputWorkspace");

  return ws;
}

//----------------------------------------------------------------------------------------------
/** Call diffraction focus to a matrix workspace with ragged rebin parameters
 */
API::MatrixWorkspace_sptr AlignAndFocusPowder::diffractionFocusRaggedRebinInDspace(API::MatrixWorkspace_sptr ws) {
  if (!m_groupWS) {
    g_log.information() << "not focussing data\n";
    return ws;
  }

  API::IAlgorithm_sptr focusAlg = createChildAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", ws);
  focusAlg->setProperty("OutputWorkspace", ws);
  focusAlg->setProperty("GroupingWorkspace", m_groupWS);
  focusAlg->setProperty("PreserveEvents", m_preserveEvents);
  focusAlg->setProperty("DMin", m_dmins);
  focusAlg->setProperty("DMax", m_dmaxs);
  focusAlg->setProperty("Delta", m_delta_ragged);

  g_log.information() << "running DiffractionFocussing(PreserveEvents=" << m_preserveEvents;
  g_log.information() << " XMin=" << focusAlg->getPropertyValue("DMin");
  g_log.information() << " XMax=" << focusAlg->getPropertyValue("DMax");
  g_log.information() << " Delta=" << focusAlg->getPropertyValue("Delta");
  g_log.information() << ") started at " << Types::Core::DateAndTime::getCurrentTime() << "\n";

  focusAlg->executeAsChildAlg();
  ws = focusAlg->getProperty("OutputWorkspace");

  return ws;
}
//----------------------------------------------------------------------------------------------
/** Convert units
 */
API::MatrixWorkspace_sptr AlignAndFocusPowder::convertUnits(API::MatrixWorkspace_sptr matrixws,
                                                            const std::string &target) {
  g_log.information() << "running ConvertUnits(Target=" << target << ") started at "
                      << Types::Core::DateAndTime::getCurrentTime() << "\n";

  API::IAlgorithm_sptr convert2Alg = createChildAlgorithm("ConvertUnits");
  convert2Alg->setProperty("InputWorkspace", matrixws);
  convert2Alg->setProperty("OutputWorkspace", matrixws);
  convert2Alg->setProperty("Target", target);
  convert2Alg->executeAsChildAlg();

  matrixws = convert2Alg->getProperty("OutputWorkspace");

  return matrixws;
}

API::MatrixWorkspace_sptr AlignAndFocusPowder::filterResonances(API::MatrixWorkspace_sptr matrixws) {
  // determine the previous units
  const std::string PREVIOUS_UNITS(matrixws->getAxis(0)->unit()->unitID());
  // number of resonance windows to be removed
  const size_t NUM_WINDOWS = m_resonanceLower.size();

  // convert to the units requested
  matrixws = convertUnits(matrixws, getPropertyValue(PropertyNames::RESONANCE_UNITS));

  // filter out the requested area
  API::IAlgorithm_sptr maskBinsAlg = createChildAlgorithm("MaskBins");
  for (size_t i = 0; i < NUM_WINDOWS; ++i) {
    g_log.information() << "running MaskBins(XMin=" << m_resonanceLower[i] << ", XMax=" << m_resonanceUpper[i]
                        << ") started at " << Types::Core::DateAndTime::getCurrentTime() << "\n";
    maskBinsAlg->setProperty("InputWorkspace", matrixws);
    maskBinsAlg->setProperty("OutputWorkspace", matrixws); // operate in-place
    maskBinsAlg->setProperty("XMin", m_resonanceLower[i]);
    maskBinsAlg->setProperty("XMax", m_resonanceUpper[i]);
    maskBinsAlg->executeAsChildAlg();

    matrixws = maskBinsAlg->getProperty("OutputWorkspace"); // update workspace pointer
  }

  // convert back to the original units
  matrixws = convertUnits(matrixws, PREVIOUS_UNITS);

  return matrixws;
}

//----------------------------------------------------------------------------------------------
/** Rebin
 */
API::MatrixWorkspace_sptr AlignAndFocusPowder::rebin(API::MatrixWorkspace_sptr matrixws) {
  if (!m_delta_ragged.empty()) {
    return matrixws;
  } else if (m_resampleX != 0) {
    // ResampleX
    g_log.information() << "running ResampleX(NumberBins=" << abs(m_resampleX) << ", LogBinning=" << (m_resampleX < 0)
                        << ", dMin(" << m_dmins.size() << "), dmax(" << m_dmaxs.size() << ")) started at "
                        << Types::Core::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr alg = createChildAlgorithm("ResampleX");
    alg->setProperty("InputWorkspace", matrixws);
    alg->setProperty("OutputWorkspace", matrixws);
    if ((!m_dmins.empty()) && (!m_dmaxs.empty())) {
      size_t numHist = m_outputW->getNumberHistograms();
      if ((numHist == m_dmins.size()) && (numHist == m_dmaxs.size())) {
        alg->setProperty("XMin", m_dmins);
        alg->setProperty("XMax", m_dmaxs);
      } else {
        g_log.information() << "Number of dmin and dmax values don't match the "
                            << "number of workspace indices. Ignoring the parameters.\n";
      }
    }
    alg->setProperty("NumberBins", abs(m_resampleX));
    alg->setProperty("LogBinning", (m_resampleX < 0));
    alg->executeAsChildAlg();
    matrixws = alg->getProperty("OutputWorkspace");
    return matrixws;
  } else {
    g_log.information() << "running Rebin( ";
    for (double param : m_params)
      g_log.information() << param << " ";
    g_log.information() << ") started at " << Types::Core::DateAndTime::getCurrentTime() << "\n";
    for (double param : m_params)
      if (isEmpty(param))
        g_log.warning("encountered empty binning parameter");

    API::IAlgorithm_sptr rebin3Alg = createChildAlgorithm("Rebin");
    rebin3Alg->setProperty("InputWorkspace", matrixws);
    rebin3Alg->setProperty("OutputWorkspace", matrixws);
    rebin3Alg->setProperty("Params", m_params);
    rebin3Alg->executeAsChildAlg();
    matrixws = rebin3Alg->getProperty("OutputWorkspace");
    return matrixws;
  }
}

//----------------------------------------------------------------------------------------------
/** RebinRagged this should only be done on the final focussed workspace
 */
API::MatrixWorkspace_sptr AlignAndFocusPowder::rebinRagged(API::MatrixWorkspace_sptr matrixws, const bool inDspace) {
  // local variables to control whether or not to log individual values
  bool print_xmin = false;
  bool print_xmax = false;

  // configure RebinRagged
  API::IAlgorithm_sptr alg = createChildAlgorithm("RebinRagged");
  if (inDspace) {
    if (!m_dmins.empty()) {
      print_xmin = true;
      alg->setProperty("XMin", m_dmins);
    }
    if (!m_dmaxs.empty()) {
      print_xmax = true;
      alg->setProperty("XMax", m_dmaxs);
    }
  } else { // assume time-of-flight
    if (tmin > 0.) {
      print_xmin = true;
      // wacky syntax to set a single value to an ArrayProperty
      alg->setProperty("XMin", std::vector<double>(1, tmin));
    }
    if (tmax > 0. && tmax > tmin) {
      print_xmax = true;
      // wacky syntax to set a single value to an ArrayProperty
      alg->setProperty("XMax", std::vector<double>(1, tmax));
    }
  }
  alg->setProperty("Delta", m_delta_ragged);
  alg->setProperty("InputWorkspace", matrixws);
  alg->setProperty("OutputWorkspace", matrixws);
  alg->setProperty("PreserveEvents", m_preserveEvents);

  // log the parameters used
  g_log.information() << "running RebinRagged(";
  if (print_xmin)
    g_log.information() << " XMin=" << alg->getPropertyValue("XMin");
  if (print_xmax)
    g_log.information() << " XMax=" << alg->getPropertyValue("XMax");
  g_log.information() << " Delta=" << alg->getPropertyValue("Delta");
  g_log.information() << " ) started at " << Types::Core::DateAndTime::getCurrentTime() << "\n";

  // run the algorithm and get the result back
  alg->executeAsChildAlg();
  matrixws = alg->getProperty("OutputWorkspace");
  return matrixws;
}

//----------------------------------------------------------------------------------------------
/** Add workspace2 to workspace1 by adding spectrum.
 */
MatrixWorkspace_sptr AlignAndFocusPowder::conjoinWorkspaces(const API::MatrixWorkspace_sptr &ws1,
                                                            const API::MatrixWorkspace_sptr &ws2, size_t offset) {
  // Get information from ws1: maximum spectrum number, and store original
  // spectrum Nos
  size_t nspec1 = ws1->getNumberHistograms();
  specnum_t maxspecNo1 = 0;
  std::vector<specnum_t> origspecNos;
  for (size_t i = 0; i < nspec1; ++i) {
    specnum_t tmpspecNo = ws1->getSpectrum(i).getSpectrumNo();
    origspecNos.emplace_back(tmpspecNo);
    if (tmpspecNo > maxspecNo1)
      maxspecNo1 = tmpspecNo;
  }

  g_log.information() << "[DBx536] Max spectrum number of ws1 = " << maxspecNo1 << ", Offset = " << offset << ".\n";

  size_t nspec2 = ws2->getNumberHistograms();

  // Conjoin 2 workspaces
  Algorithm_sptr alg = this->createChildAlgorithm("AppendSpectra");
  alg->initialize();
  ;

  alg->setProperty("InputWorkspace1", ws1);
  alg->setProperty("InputWorkspace2", ws2);
  alg->setProperty("OutputWorkspace", ws1);
  alg->setProperty("ValidateInputs", false);

  alg->executeAsChildAlg();

  API::MatrixWorkspace_sptr outws = alg->getProperty("OutputWorkspace");

  // FIXED : Restore the original spectrum Nos to spectra from ws1
  for (size_t i = 0; i < nspec1; ++i) {
    specnum_t tmpspecNo = outws->getSpectrum(i).getSpectrumNo();
    outws->getSpectrum(i).setSpectrumNo(origspecNos[i]);

    g_log.information() << "[DBx540] Conjoined spectrum " << i << ": restore spectrum number to "
                        << outws->getSpectrum(i).getSpectrumNo() << " from spectrum number = " << tmpspecNo << ".\n";
  }

  // Rename spectrum number
  if (offset >= 1) {
    for (size_t i = 0; i < nspec2; ++i) {
      specnum_t newspecid = maxspecNo1 + static_cast<specnum_t>((i) + offset);
      outws->getSpectrum(nspec1 + i).setSpectrumNo(newspecid);
      // ISpectrum* spec = outws->getSpectrum(nspec1+i);
      // if (spec)
      // spec->setSpectrumNo(3);
    }
  }

  return outws;
}

void AlignAndFocusPowder::convertOffsetsToCal(DataObjects::OffsetsWorkspace_sptr &offsetsWS) {
  if (!offsetsWS)
    return;

  IAlgorithm_sptr alg = createChildAlgorithm("ConvertDiffCal");
  alg->setProperty("OffsetsWorkspace", offsetsWS);
  alg->setPropertyValue("OutputWorkspace", m_instName + "_cal");
  alg->executeAsChildAlg();

  m_calibrationWS = alg->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(m_instName + "_cal", m_calibrationWS);
}

//----------------------------------------------------------------------------------------------
/**
 * Loads the .cal file if necessary.
 */
void AlignAndFocusPowder::loadCalFile(const std::string &calFilename, const std::string &groupFilename) {

  // check if the workspaces exist with their canonical names so they are not
  // reloaded for chunks
  if ((!m_groupWS) && (!calFilename.empty()) && (!groupFilename.empty())) {
    if (AnalysisDataService::Instance().doesExist(m_instName + "_group"))
      m_groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(m_instName + "_group");
  }
  if ((!m_calibrationWS) && (!calFilename.empty())) {
    OffsetsWorkspace_sptr offsetsWS = getProperty(PropertyNames::OFFSET_WKSP);
    if (offsetsWS) {
      convertOffsetsToCal(offsetsWS);
    } else {
      if (AnalysisDataService::Instance().doesExist(m_instName + "_cal"))
        m_calibrationWS = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(m_instName + "_cal");
      if (!m_calibrationWS) {
        if (AnalysisDataService::Instance().doesExist(m_instName + "_offsets")) {
          offsetsWS = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(m_instName + "_offsets");
          convertOffsetsToCal(offsetsWS);
        }
      }
    }
  }
  if ((!m_maskWS) && (!calFilename.empty())) {
    if (AnalysisDataService::Instance().doesExist(m_instName + "_mask"))
      m_maskWS = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(m_instName + "_mask");
  }

  // see if everything exists to exit early
  if (m_groupWS && m_calibrationWS && m_maskWS)
    return;

  // see if the calfile or grouping file is specified
  if (calFilename.empty() && groupFilename.empty())
    return;

  // bunch of booleans to keep track of things
  const bool loadMask = !m_maskWS;
  const bool loadGrouping = !m_groupWS;
  const bool loadCalibration = !m_calibrationWS;

  if (calFilename.empty()) { // only load the grouping file
    g_log.information() << "Loading Grouping file \"" << groupFilename << "\"\n";

    IAlgorithm_sptr alg = createChildAlgorithm("LoadDetectorsGroupingFile");
    alg->setProperty("InputFile", groupFilename);
    alg->setProperty("InputWorkspace", m_inputW);
    alg->executeAsChildAlg();

    m_groupWS = alg->getProperty("OutputWorkspace");
    const std::string groupname = m_instName + "_group";
    AnalysisDataService::Instance().addOrReplace(groupname, m_groupWS);
    this->setPropertyValue(PropertyNames::GROUP_WKSP, groupname);
  } else { // let LoadDiffCal sort out everything
    g_log.information() << "Loading Calibration file \"" << calFilename << "\"";
    if (!groupFilename.empty())
      g_log.information() << "with grouping from \"" << groupFilename << "\"";
    g_log.information("");

    IAlgorithm_sptr alg = createChildAlgorithm("LoadDiffCal");
    alg->setProperty("InputWorkspace", m_inputW);
    alg->setPropertyValue("Filename", calFilename);
    alg->setPropertyValue("GroupFilename", groupFilename);
    alg->setProperty<bool>("MakeCalWorkspace", loadCalibration);
    alg->setProperty<bool>("MakeGroupingWorkspace", loadGrouping);
    alg->setProperty<bool>("MakeMaskWorkspace", loadMask);
    alg->setProperty<double>("TofMin", getProperty("TMin"));
    alg->setProperty<double>("TofMax", getProperty("TMax"));
    alg->setPropertyValue("WorkspaceName", m_instName);
    alg->executeAsChildAlg();

    // replace workspaces as appropriate
    if (loadGrouping) {
      m_groupWS = alg->getProperty("OutputGroupingWorkspace");

      const std::string groupname = m_instName + "_group";
      AnalysisDataService::Instance().addOrReplace(groupname, m_groupWS);
      this->setPropertyValue(PropertyNames::GROUP_WKSP, groupname);
    }
    if (loadCalibration) {
      m_calibrationWS = alg->getProperty("OutputCalWorkspace");

      const std::string calname = m_instName + "_cal";
      AnalysisDataService::Instance().addOrReplace(calname, m_calibrationWS);
      this->setPropertyValue(PropertyNames::CAL_WKSP, calname);
    }
    if (loadMask) {
      m_maskWS = alg->getProperty("OutputMaskWorkspace");

      const std::string maskname = m_instName + "_mask";
      AnalysisDataService::Instance().addOrReplace(maskname, m_maskWS);
      this->setPropertyValue(PropertyNames::MASK_WKSP, maskname);
    }
  }
}

void AlignAndFocusPowder::compressEventsOutputWS(const double compressEventsTolerance,
                                                 const double wallClockTolerance) {
  if (compressEventsTolerance == 0.)
    return; // no compression is required

  if (auto outputEW = std::dynamic_pointer_cast<EventWorkspace>(m_outputW)) {
    g_log.information() << "running CompressEvents(Tolerance=" << compressEventsTolerance;
    if (!isEmpty(wallClockTolerance))
      g_log.information() << " and WallClockTolerance=" << wallClockTolerance;
    g_log.information() << ") started at " << Types::Core::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr compressAlg = createChildAlgorithm("CompressEvents");
    compressAlg->setProperty("InputWorkspace", outputEW);
    compressAlg->setProperty("OutputWorkspace", outputEW);
    compressAlg->setProperty("Tolerance", compressEventsTolerance);
    if (!isEmpty(wallClockTolerance)) {
      compressAlg->setProperty("WallClockTolerance", wallClockTolerance);
      compressAlg->setPropertyValue("StartTime", getPropertyValue(PropertyNames::COMPRESS_WALL_START));
    }
    compressAlg->executeAsChildAlg();
    outputEW = compressAlg->getProperty("OutputWorkspace");
    m_outputW = std::dynamic_pointer_cast<MatrixWorkspace>(outputEW);
  }
}

/**
 * Return true if a rough estimate suggests that the size of the events will be smaller
 * after compressing. This assumes that the events are equally spread throughout the
 * supplied time-of-flight range with a spacing of compressTolerance.
 */
bool AlignAndFocusPowder::shouldCompressUnfocused(const double compressTolerance, const double tofmin,
                                                  const double tofmax, const bool hasWallClockTolerance) {
  // compressing to WEIGHTED (w/ time) is harder to predict
  if (hasWallClockTolerance)
    return false;
  // compressing isn't an option
  if (compressTolerance == 0.)
    return false;

  if (const auto eventWS = std::dynamic_pointer_cast<const EventWorkspace>(m_outputW)) {
    // estimate the time-of-flight range for the data
    // if the parameters aren't supplied, get them from the workspace
    double tofmin_wksp = tofmin;
    double tofmax_wksp = tofmax;
    if (isEmpty(tofmin) || isEmpty(tofmax))
      getTofRange(m_outputW, tofmin_wksp, tofmax_wksp);
    const double tofRange = std::fabs(tofmax_wksp - tofmin_wksp);

    // constants estimating size difference of various events
    constexpr double TOF_EVENT_BYTE_SIZE{static_cast<double>(sizeof(Types::Event::TofEvent))};
    constexpr double WEIGHTED_EVENT_BYTE_SIZE{static_cast<double>(sizeof(WeightedEvent))};
    constexpr double WEIGHTED_NOTIME_EVENT_BYTE_SIZE{static_cast<double>(sizeof(WeightedEventNoTime))};

    // assume one frame although this is generically wrong
    // there are 3 fields in weighted events no time
    double sizeWeightedEventsEstimate;
    if (compressTolerance > 0) // linear
      sizeWeightedEventsEstimate = WEIGHTED_NOTIME_EVENT_BYTE_SIZE * tofRange / compressTolerance;
    else { // log
      if (tofmin_wksp < 0)
        return false; // log compression cannot have negative TOF values

      if (tofmin_wksp == 0)
        tofmin_wksp = compressTolerance;

      sizeWeightedEventsEstimate =
          WEIGHTED_NOTIME_EVENT_BYTE_SIZE * log(tofmax_wksp / tofmin_wksp) / log1p(abs(compressTolerance));
    }

    double numEvents = static_cast<double>(eventWS->getNumberEvents());
    const auto eventType = eventWS->getEventType();
    if (eventType == API::EventType::TOF) {
      // there are two fields in tof
      numEvents *= TOF_EVENT_BYTE_SIZE;
    } else if (eventType == API::EventType::WEIGHTED) {
      // there are four fields in weighted w/ time
      numEvents *= WEIGHTED_EVENT_BYTE_SIZE;
    } else if (eventType == API::EventType::WEIGHTED_NOTIME) {
      // it looks like things are already compressed
      return false;
    } else {
      return false; // not coded for something else
    }

    // there is an error as this doesn't account for the number of spectra, but this is meant to be
    // a rough calculation
    g_log.information() << "Calculation for compressing events early with size of events currently " << numEvents
                        << " and comparing to " << sizeWeightedEventsEstimate << "\n";
    return sizeWeightedEventsEstimate < numEvents;
  } else {
    // fall-through is to not compress
    return false;
  }
}

} // namespace Mantid::WorkflowAlgorithms
