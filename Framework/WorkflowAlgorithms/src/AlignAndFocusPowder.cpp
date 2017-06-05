#include "MantidWorkflowAlgorithms/AlignAndFocusPowder.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidAPI/FileFinder.h"

using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace WorkflowAlgorithms {
using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace;
using API::FileProperty;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(AlignAndFocusPowder)

AlignAndFocusPowder::AlignAndFocusPowder() : API::DataProcessorAlgorithm() {}

AlignAndFocusPowder::~AlignAndFocusPowder() {
  if (m_progress)
    delete m_progress;
}

const std::string AlignAndFocusPowder::name() const {
  return "AlignAndFocusPowder";
}

int AlignAndFocusPowder::version() const { return 1; }

const std::string AlignAndFocusPowder::category() const {
  return "Workflow\\Diffraction";
}

//----------------------------------------------------------------------------------------------
/** Initialisation method. Declares properties to be used in algorithm.
 */
void AlignAndFocusPowder::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The input workspace");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The result of diffraction focussing of InputWorkspace");
  // declareProperty(
  //   new WorkspaceProperty<MatrixWorkspace>("LowResTOFWorkspace", "",
  //   Direction::Output, PropertyMode::Optional),
  //   "The name of the workspace containing the filtered low resolution TOF
  //   data.");
  declareProperty(Kernel::make_unique<FileProperty>(
                      "CalFileName", "", FileProperty::OptionalLoad,
                      std::vector<std::string>{".h5", ".hd5", ".hdf", ".cal"}),
                  "The name of the calibration file with offset, masking, and "
                  "grouping data");
  declareProperty(Kernel::make_unique<FileProperty>(
                      "GroupFilename", "", FileProperty::OptionalLoad,
                      std::vector<std::string>{".xml", ".cal"}),
                  "Overrides grouping from CalFileName");
  declareProperty(
      make_unique<WorkspaceProperty<GroupingWorkspace>>(
          "GroupingWorkspace", "", Direction::InOut, PropertyMode::Optional),
      "Optional: A GroupingWorkspace giving the grouping info.");

  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(
          "CalibrationWorkspace", "", Direction::InOut, PropertyMode::Optional),
      "Optional: A Workspace containing the calibration information. Either "
      "this or CalibrationFile needs to be specified.");
  declareProperty(
      make_unique<WorkspaceProperty<OffsetsWorkspace>>(
          "OffsetsWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: An OffsetsWorkspace giving the detector calibration values.");
  declareProperty(
      make_unique<WorkspaceProperty<MaskWorkspace>>(
          "MaskWorkspace", "", Direction::InOut, PropertyMode::Optional),
      "Optional: A workspace giving which detectors are masked.");
  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>(
          "MaskBinTable", "", Direction::Input, PropertyMode::Optional),
      "Optional: A workspace giving pixels and bins to mask.");
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "Params" /*, boost::make_shared<RebinParamsValidator>()*/),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally\n"
      "this can be followed by a comma and more widths and last boundary "
      "pairs.\n"
      "Negative width values indicate logarithmic binning.");
  declareProperty("ResampleX", 0, "Number of bins in x-axis. Non-zero value "
                                  "overrides \"Params\" property. Negative "
                                  "value means logarithmic binning.");
  setPropertySettings(
      "Params", make_unique<EnabledWhenProperty>("ResampleX", IS_DEFAULT));
  declareProperty("Dspacing", true,
                  "Bin in Dspace. (True is Dspace; False is TOF)");
  declareProperty(make_unique<ArrayProperty<double>>("DMin"),
                  "Minimum for Dspace axis. (Default 0.) ");
  mapPropertyName("DMin", "d_min");
  declareProperty(make_unique<ArrayProperty<double>>("DMax"),
                  "Maximum for Dspace axis. (Default 0.) ");
  mapPropertyName("DMax", "d_max");
  declareProperty("TMin", EMPTY_DBL(), "Minimum for TOF axis. Defaults to 0. ");
  mapPropertyName("TMin", "tof_min");
  declareProperty("TMax", EMPTY_DBL(),
                  "Maximum for TOF or dspace axis. Defaults to 0. ");
  mapPropertyName("TMax", "tof_max");
  declareProperty("PreserveEvents", true, "If the InputWorkspace is an "
                                          "EventWorkspace, this will preserve "
                                          "the full event list (warning: this "
                                          "will use much more memory!).");
  declareProperty("RemovePromptPulseWidth", 0., "Width of events (in "
                                                "microseconds) near the prompt "
                                                "pulse to remove. 0 disables");
  declareProperty("CompressTolerance", 0.01, "Compress events (in "
                                             "microseconds) within this "
                                             "tolerance. (Default 0.01) ");
  declareProperty("UnwrapRef", 0., "Reference total flight path for frame "
                                   "unwrapping. Zero skips the correction");
  declareProperty(
      "LowResRef", 0.,
      "Reference DIFC for resolution removal. Zero skips the correction");
  declareProperty(
      "CropWavelengthMin", 0.,
      "Crop the data at this minimum wavelength. Overrides LowResRef.");
  mapPropertyName("CropWavelengthMin", "wavelength_min");
  declareProperty("CropWavelengthMax", EMPTY_DBL(),
                  "Crop the data at this maximum wavelength. Forces use of "
                  "CropWavelengthMin.");
  mapPropertyName("CropWavelengthMax", "wavelength_max");
  declareProperty("PrimaryFlightPath", -1.0,
                  "If positive, focus positions are changed.  (Default -1) ");
  declareProperty(make_unique<ArrayProperty<int32_t>>("SpectrumIDs"),
                  "Optional: Spectrum Nos (note that it is not detector ID or "
                  "workspace indices).");
  declareProperty(make_unique<ArrayProperty<double>>("L2"),
                  "Optional: Secondary flight (L2) paths for each detector");
  declareProperty(make_unique<ArrayProperty<double>>("Polar"),
                  "Optional: Polar angles (two thetas) for detectors");
  declareProperty(make_unique<ArrayProperty<double>>("Azimuthal"),
                  "Azimuthal angles (out-of-plain) for detectors");

  declareProperty("LowResSpectrumOffset", -1,
                  "Offset on spectrum No of low resolution spectra from high "
                  "resolution one. "
                  "If negative, then all the low resolution TOF will not be "
                  "processed.  Otherwise, low resolution TOF "
                  "will be stored in an additional set of spectra. "
                  "If offset is equal to 0, then the low resolution will have "
                  "same spectrum Nos as the normal ones.  "
                  "Otherwise, the low resolution spectra will have spectrum "
                  "IDs offset from normal ones. ");
  declareProperty("ReductionProperties", "__powdereduction", Direction::Input);
}

template <typename NumT> struct RegLowVectorPair {
  std::vector<NumT> reg;
  std::vector<NumT> low;
};

template <typename NumT>
RegLowVectorPair<NumT> splitVectors(const std::vector<NumT> &orig,
                                    const size_t numVal,
                                    const std::string &label) {
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
          << "the number of histograms or empty (" << orig.size() << " != 0 or "
          << numVal << " or " << (2 * numVal) << ")";
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
double
AlignAndFocusPowder::getVecPropertyFromPmOrSelf(const std::string &name,
                                                std::vector<double> &avec) {
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
  m_inputW = getProperty("InputWorkspace");
  m_inputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_inputW);
  m_instName = m_inputW->getInstrument()->getName();
  m_instName =
      Kernel::ConfigService::Instance().getInstrument(m_instName).shortName();
  std::string calFilename = getPropertyValue("CalFileName");
  std::string groupFilename = getPropertyValue("GroupFilename");
  m_calibrationWS = getProperty("CalibrationWorkspace");
  m_maskWS = getProperty("MaskWorkspace");
  m_groupWS = getProperty("GroupingWorkspace");
  DataObjects::TableWorkspace_sptr maskBinTableWS = getProperty("MaskBinTable");
  m_l1 = getProperty("PrimaryFlightPath");
  specids = getProperty("SpectrumIDs");
  l2s = getProperty("L2");
  tths = getProperty("Polar");
  phis = getProperty("Azimuthal");
  m_params = getProperty("Params");
  dspace = getProperty("DSpacing");
  auto dmin = getVecPropertyFromPmOrSelf("DMin", m_dmins);
  auto dmax = getVecPropertyFromPmOrSelf("DMax", m_dmaxs);
  LRef = getProperty("UnwrapRef");
  DIFCref = getProperty("LowResRef");
  minwl = getProperty("CropWavelengthMin");
  maxwl = getProperty("CropWavelengthMax");
  if (maxwl == 0.)
    maxwl = EMPTY_DBL(); // python can only specify 0 for unused
  tmin = getProperty("TMin");
  tmax = getProperty("TMax");
  m_preserveEvents = getProperty("PreserveEvents");
  m_resampleX = getProperty("ResampleX");
  // determine some bits about d-space and binning
  if (m_resampleX != 0) {
    m_params.clear(); // ignore the normal rebin parameters
  } else if (m_params.size() == 1) {
    if (dmax > 0.)
      dspace = true;
    else
      dspace = false;
  }
  if (dspace) {
    if (m_params.size() == 1 && dmax > 0) {
      double step = m_params[0];
      m_params.clear();
      if (step > 0 || dmin > 0) {
        m_params.push_back(dmin);
        m_params.push_back(step);
        m_params.push_back(dmax);
        g_log.information() << "d-Spacing Binning: " << m_params[0] << "  "
                            << m_params[1] << "  " << m_params[2] << "\n";
      }
    }
  } else {
    if (m_params.size() == 1 && tmax > 0) {
      double step = m_params[0];
      if (step > 0 || tmin > 0) {
        m_params[0] = tmin;
        m_params.push_back(step);
        m_params.push_back(tmax);
        g_log.information() << "TOF Binning: " << m_params[0] << "  "
                            << m_params[1] << "  " << m_params[2] << "\n";
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
  if (!dspace && m_params.size() == 3) {
    xmin = m_params[0];
    xmax = m_params[2];
  }

  // Low resolution
  int lowresoffset = getProperty("LowResSpectrumOffset");
  if (lowresoffset < 0) {
    m_processLowResTOF = false;
  } else {
    m_processLowResTOF = true;
    m_lowResSpecOffset = static_cast<size_t>(lowresoffset);
  }

  loadCalFile(calFilename, groupFilename);

  // Now setup the output workspace
  m_outputW = getProperty("OutputWorkspace");
  if (m_inputEW) {
    // event workspace
    if (m_outputW != m_inputW) {
      // out-of-place: clone the input EventWorkspace
      m_outputEW = m_inputEW->clone();
      m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEW);
    } else {
      // in-place
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    }
  } else {
    // workspace2D
    if (m_outputW != m_inputW) {
      m_outputW = WorkspaceFactory::Instance().create(m_inputW);
    }
  }

  if (m_processLowResTOF) {
    if (!m_inputEW) {
      throw std::runtime_error(
          "Input workspace is not EventWorkspace.  It is not supported now.");
    } else {
      // Make a brand new EventWorkspace
      m_lowResEW = boost::dynamic_pointer_cast<EventWorkspace>(
          WorkspaceFactory::Instance().create(
              "EventWorkspace", m_inputEW->getNumberHistograms(), 2, 1));

      // Cast to the matrixOutputWS and save it
      m_lowResW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_lowResEW);
      // m_lowResW->setName(lowreswsname);
    }
  }

  // set up a progress bar with the "correct" number of steps
  m_progress = new Progress(this, 0., 1., 22);

  if (m_inputEW) {
    double tolerance = getProperty("CompressTolerance");
    if (tolerance > 0.) {
      g_log.information() << "running CompressEvents(Tolerance=" << tolerance
                          << ") started at "
                          << Kernel::DateAndTime::getCurrentTime() << "\n";
      API::IAlgorithm_sptr compressAlg = createChildAlgorithm("CompressEvents");
      compressAlg->setProperty("InputWorkspace", m_outputEW);
      compressAlg->setProperty("OutputWorkspace", m_outputEW);
      compressAlg->setProperty("OutputWorkspace", m_outputEW);
      compressAlg->setProperty("Tolerance", tolerance);
      compressAlg->executeAsChildAlg();
      m_outputEW = compressAlg->getProperty("OutputWorkspace");
      m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEW);
    } else {
      g_log.information() << "Not compressing event list\n";
      doSortEvents(m_outputW); // still sort to help some thing out
    }
  }
  m_progress->report();

  if (xmin > 0. || xmax > 0.) {
    double tempmin;
    double tempmax;
    m_outputW->getXMinMax(tempmin, tempmax);

    g_log.information() << "running CropWorkspace(TOFmin=" << xmin
                        << ", TOFmax=" << xmax << ") started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr cropAlg = createChildAlgorithm("CropWorkspace");
    cropAlg->setProperty("InputWorkspace", m_outputW);
    cropAlg->setProperty("OutputWorkspace", m_outputW);
    if ((xmin > 0.) && (xmin > tempmin))
      cropAlg->setProperty("Xmin", xmin);
    if ((xmax > 0.) && (xmax < tempmax))
      cropAlg->setProperty("Xmax", xmax);
    cropAlg->executeAsChildAlg();
    m_outputW = cropAlg->getProperty("OutputWorkspace");
    m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
  }
  m_progress->report();

  // filter the input events if appropriate
  double removePromptPulseWidth = getProperty("RemovePromptPulseWidth");
  if (removePromptPulseWidth > 0.) {
    m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    if (m_outputEW->getNumberEvents() > 0) {
      g_log.information() << "running RemovePromptPulse(Width="
                          << removePromptPulseWidth << ") started at "
                          << Kernel::DateAndTime::getCurrentTime() << "\n";
      API::IAlgorithm_sptr filterPAlg =
          createChildAlgorithm("RemovePromptPulse");
      filterPAlg->setProperty("InputWorkspace", m_outputW);
      filterPAlg->setProperty("OutputWorkspace", m_outputW);
      filterPAlg->setProperty("Width", removePromptPulseWidth);
      filterPAlg->executeAsChildAlg();
      m_outputW = filterPAlg->getProperty("OutputWorkspace");
      m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    } else {
      g_log.information("skipping RemovePromptPulse on empty EventWorkspace");
    }
  }
  m_progress->report();

  if (maskBinTableWS) {
    g_log.information() << "running MaskBinsFromTable started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr alg = createChildAlgorithm("MaskBinsFromTable");
    alg->setProperty("InputWorkspace", m_outputW);
    alg->setProperty("OutputWorkspace", m_outputW);
    alg->setProperty("MaskingInformation", maskBinTableWS);
    alg->executeAsChildAlg();
    m_outputW = alg->getProperty("OutputWorkspace");
    m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
  }
  m_progress->report();

  if (m_maskWS) {
    g_log.information() << "running MaskDetectors started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr maskAlg = createChildAlgorithm("MaskDetectors");
    maskAlg->setProperty("Workspace", m_outputW);
    maskAlg->setProperty("MaskedWorkspace", m_maskWS);
    maskAlg->executeAsChildAlg();
    Workspace_sptr tmpW = maskAlg->getProperty("Workspace");
    m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(tmpW);
    m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
  }
  m_progress->report();

  if (!dspace)
    m_outputW = rebin(m_outputW);
  m_progress->report();

  if (m_calibrationWS) {
    g_log.information() << "running AlignDetectors started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr alignAlg = createChildAlgorithm("AlignDetectors");
    alignAlg->setProperty("InputWorkspace", m_outputW);
    alignAlg->setProperty("OutputWorkspace", m_outputW);
    alignAlg->setProperty("CalibrationWorkspace", m_calibrationWS);
    alignAlg->executeAsChildAlg();
    m_outputW = alignAlg->getProperty("OutputWorkspace");
  } else {
    m_outputW = convertUnits(m_outputW, "dSpacing");
  }
  m_progress->report();

  if (LRef > 0. || minwl > 0. || DIFCref > 0. || (!isEmpty(maxwl))) {
    m_outputW = convertUnits(m_outputW, "TOF");
  }
  m_progress->report();

  // Beyond this point, low resolution TOF workspace is considered.
  if (LRef > 0.) {
    g_log.information() << "running UnwrapSNS(LRef=" << LRef << ",Tmin=" << tmin
                        << ",Tmax=" << tmax << ") started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
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

  if (minwl > 0. || (!isEmpty(maxwl))) { // just crop the worksapce
    // turn off the low res stuff
    m_processLowResTOF = false;

    EventWorkspace_sptr ews =
        boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    if (ews)
      g_log.information() << "Number of events = " << ews->getNumberEvents()
                          << ". ";
    g_log.information("\n");

    m_outputW = convertUnits(m_outputW, "Wavelength");

    g_log.information() << "running CropWorkspace(WavelengthMin=" << minwl;
    if (!isEmpty(maxwl))
      g_log.information() << ", WavelengthMax=" << maxwl;
    g_log.information() << ") started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";

    API::IAlgorithm_sptr removeAlg = createChildAlgorithm("CropWorkspace");
    removeAlg->setProperty("InputWorkspace", m_outputW);
    removeAlg->setProperty("OutputWorkspace", m_outputW);
    removeAlg->setProperty("XMin", minwl);
    removeAlg->setProperty("XMax", maxwl);
    removeAlg->executeAsChildAlg();
    m_outputW = removeAlg->getProperty("OutputWorkspace");
    if (ews)
      g_log.information() << "Number of events = " << ews->getNumberEvents()
                          << ".\n";
  } else if (DIFCref > 0.) {
    g_log.information() << "running RemoveLowResTof(RefDIFC=" << DIFCref
                        << ",K=3.22) started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
    EventWorkspace_sptr ews =
        boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
    if (ews)
      g_log.information() << "Number of events = " << ews->getNumberEvents()
                          << ". ";
    g_log.information("\n");

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

  EventWorkspace_sptr ews =
      boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
  if (ews) {
    size_t numhighevents = ews->getNumberEvents();
    if (m_processLowResTOF) {
      EventWorkspace_sptr lowes =
          boost::dynamic_pointer_cast<EventWorkspace>(m_lowResW);
      size_t numlowevents = lowes->getNumberEvents();
      g_log.information() << "Number of high TOF events = " << numhighevents
                          << "; "
                          << "Number of low TOF events = " << numlowevents
                          << ".\n";
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

  if (dspace) {
    m_outputW = rebin(m_outputW);
    if (m_processLowResTOF)
      m_lowResW = rebin(m_lowResW);
  }
  m_progress->report();

  doSortEvents(m_outputW);
  if (m_processLowResTOF)
    doSortEvents(m_lowResW);
  m_progress->report();

  // Diffraction focus
  m_outputW = diffractionFocus(m_outputW);
  if (m_processLowResTOF)
    m_lowResW = diffractionFocus(m_lowResW);
  m_progress->report();

  doSortEvents(m_outputW);
  if (m_processLowResTOF)
    doSortEvents(m_lowResW);
  m_progress->report();

  // this next call should probably be in for rebin as well
  // but it changes the system tests
  if (dspace && m_resampleX != 0) {
    m_outputW = rebin(m_outputW);
    if (m_processLowResTOF)
      m_lowResW = rebin(m_lowResW);
  }
  m_progress->report();

  // edit the instrument geometry
  if (m_groupWS &&
      (m_l1 > 0 || !tths.empty() || !l2s.empty() || !phis.empty())) {
    size_t numreg = m_outputW->getNumberHistograms();

    try {
      // set up the vectors for doing everything
      auto specidsSplit = splitVectors(specids, numreg, "specids");
      auto tthsSplit = splitVectors(tths, numreg, "two-theta");
      auto l2sSplit = splitVectors(l2s, numreg, "L2");
      auto phisSplit = splitVectors(phis, numreg, "phi");

      // Edit instrument
      m_outputW = editInstrument(m_outputW, tthsSplit.reg, specidsSplit.reg,
                                 l2sSplit.reg, phisSplit.reg);

      if (m_processLowResTOF) {
        m_lowResW = editInstrument(m_lowResW, tthsSplit.low, specidsSplit.low,
                                   l2sSplit.low, phisSplit.low);
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
  double tolerance = getProperty("CompressTolerance");
  m_outputEW = boost::dynamic_pointer_cast<EventWorkspace>(m_outputW);
  if ((m_outputEW) && (tolerance > 0.)) {
    g_log.information() << "running CompressEvents(Tolerance=" << tolerance
                        << ") started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr compressAlg = createChildAlgorithm("CompressEvents");
    compressAlg->setProperty("InputWorkspace", m_outputEW);
    compressAlg->setProperty("OutputWorkspace", m_outputEW);
    compressAlg->setProperty("OutputWorkspace", m_outputEW);
    compressAlg->setProperty("Tolerance", tolerance);
    compressAlg->executeAsChildAlg();
    m_outputEW = compressAlg->getProperty("OutputWorkspace");
    m_outputW = boost::dynamic_pointer_cast<MatrixWorkspace>(m_outputEW);
  }
  m_progress->report();

  if ((!m_params.empty()) && (m_params.size() != 1)) {
    m_params.erase(m_params.begin());
    m_params.pop_back();
  }
  if (!m_dmins.empty())
    m_dmins.clear();
  if (!m_dmaxs.empty())
    m_dmaxs.clear();

  m_outputW = rebin(m_outputW);
  m_progress->report();

  // return the output workspace
  setProperty("OutputWorkspace", m_outputW);
}

//----------------------------------------------------------------------------------------------
/** Call edit instrument geometry
  */
API::MatrixWorkspace_sptr AlignAndFocusPowder::editInstrument(
    API::MatrixWorkspace_sptr ws, std::vector<double> polars,
    std::vector<specnum_t> specids, std::vector<double> l2s,
    std::vector<double> phis) {
  g_log.information() << "running EditInstrumentGeometry started at "
                      << Kernel::DateAndTime::getCurrentTime() << "\n";

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
API::MatrixWorkspace_sptr
AlignAndFocusPowder::diffractionFocus(API::MatrixWorkspace_sptr ws) {
  if (!m_groupWS) {
    g_log.information() << "not focussing data\n";
    return ws;
  }

  g_log.information() << "running DiffractionFocussing started at "
                      << Kernel::DateAndTime::getCurrentTime() << "\n";

  API::IAlgorithm_sptr focusAlg = createChildAlgorithm("DiffractionFocussing");
  focusAlg->setProperty("InputWorkspace", ws);
  focusAlg->setProperty("OutputWorkspace", ws);
  focusAlg->setProperty("GroupingWorkspace", m_groupWS);
  focusAlg->setProperty("PreserveEvents", m_preserveEvents);
  focusAlg->executeAsChildAlg();
  ws = focusAlg->getProperty("OutputWorkspace");

  return ws;
}

//----------------------------------------------------------------------------------------------
/** Convert units
  */
API::MatrixWorkspace_sptr
AlignAndFocusPowder::convertUnits(API::MatrixWorkspace_sptr matrixws,
                                  std::string target) {
  g_log.information() << "running ConvertUnits(Target=" << target
                      << ") started at "
                      << Kernel::DateAndTime::getCurrentTime() << "\n";

  API::IAlgorithm_sptr convert2Alg = createChildAlgorithm("ConvertUnits");
  convert2Alg->setProperty("InputWorkspace", matrixws);
  convert2Alg->setProperty("OutputWorkspace", matrixws);
  convert2Alg->setProperty("Target", target);
  convert2Alg->executeAsChildAlg();

  matrixws = convert2Alg->getProperty("OutputWorkspace");

  return matrixws;
}

//----------------------------------------------------------------------------------------------
/** Rebin
*/
API::MatrixWorkspace_sptr
AlignAndFocusPowder::rebin(API::MatrixWorkspace_sptr matrixws) {
  if (m_resampleX != 0) {
    // ResampleX
    g_log.information() << "running ResampleX(NumberBins=" << abs(m_resampleX)
                        << ", LogBinning=" << (m_resampleX < 0) << ", dMin("
                        << m_dmins.size() << "), dmax(" << m_dmaxs.size()
                        << ")) started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
    API::IAlgorithm_sptr alg = createChildAlgorithm("ResampleX");
    alg->setProperty("InputWorkspace", matrixws);
    alg->setProperty("OutputWorkspace", matrixws);
    if ((!m_dmins.empty()) && (!m_dmaxs.empty())) {
      size_t numHist = m_outputW->getNumberHistograms();
      if ((numHist == m_dmins.size()) && (numHist == m_dmaxs.size())) {
        alg->setProperty("XMin", m_dmins);
        alg->setProperty("XMax", m_dmaxs);
      } else {
        g_log.information()
            << "Number of dmin and dmax values don't match the "
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
    g_log.information() << ") started at "
                        << Kernel::DateAndTime::getCurrentTime() << "\n";
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
/** Add workspace2 to workspace1 by adding spectrum.
  */
MatrixWorkspace_sptr
AlignAndFocusPowder::conjoinWorkspaces(API::MatrixWorkspace_sptr ws1,
                                       API::MatrixWorkspace_sptr ws2,
                                       size_t offset) {
  // Get information from ws1: maximum spectrum number, and store original
  // spectrum Nos
  size_t nspec1 = ws1->getNumberHistograms();
  specnum_t maxspecNo1 = 0;
  std::vector<specnum_t> origspecNos;
  for (size_t i = 0; i < nspec1; ++i) {
    specnum_t tmpspecNo = ws1->getSpectrum(i).getSpectrumNo();
    origspecNos.push_back(tmpspecNo);
    if (tmpspecNo > maxspecNo1)
      maxspecNo1 = tmpspecNo;
  }

  g_log.information() << "[DBx536] Max spectrum number of ws1 = " << maxspecNo1
                      << ", Offset = " << offset << ".\n";

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

    g_log.information() << "[DBx540] Conjoined spectrum " << i
                        << ": restore spectrum number to "
                        << outws->getSpectrum(i).getSpectrumNo()
                        << " from spectrum number = " << tmpspecNo << ".\n";
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

void AlignAndFocusPowder::convertOffsetsToCal(
    DataObjects::OffsetsWorkspace_sptr &offsetsWS) {
  if (!offsetsWS)
    return;

  IAlgorithm_sptr alg = createChildAlgorithm("ConvertDiffCal");
  alg->setProperty("OffsetsWorkspace", offsetsWS);
  alg->setPropertyValue("OutputWorkspace", m_instName + "_cal");
  alg->executeAsChildAlg();

  m_calibrationWS = alg->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(m_instName + "_cal",
                                               m_calibrationWS);
}

//----------------------------------------------------------------------------------------------
/**
 * Loads the .cal file if necessary.
 */
void AlignAndFocusPowder::loadCalFile(const std::string &calFilename,
                                      const std::string &groupFilename) {

  // check if the workspaces exist with their canonical names so they are not
  // reloaded for chunks
  if ((!m_groupWS) && (!calFilename.empty()) && (!groupFilename.empty())) {
    try {
      m_groupWS = AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(
          m_instName + "_group");
    } catch (Exception::NotFoundError &) {
      ; // not noteworthy
    }
  }
  if ((!m_calibrationWS) && (!calFilename.empty())) {
    OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
    if (offsetsWS) {
      convertOffsetsToCal(offsetsWS);
    } else {
      try {
        m_calibrationWS =
            AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
                m_instName + "_cal");
      } catch (Exception::NotFoundError &) {
        ; // not noteworthy
      }
      if (!m_calibrationWS) {
        try {
          OffsetsWorkspace_sptr offsetsWS =
              AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(
                  m_instName + "_offsets");
          convertOffsetsToCal(offsetsWS);
        } catch (Exception::NotFoundError &) {
          ; // not noteworthy
        }
      }
    }
  }
  if ((!m_maskWS) && (!calFilename.empty())) {
    try {
      m_maskWS = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(
          m_instName + "_mask");
    } catch (Exception::NotFoundError &) {
      ; // not noteworthy
    }
  }

  // see if everything exists to exit early
  if (m_groupWS && m_calibrationWS && m_maskWS)
    return;

  // see if the calfile or grouping file is specified
  if (calFilename.empty() && groupFilename.empty())
    return;

  // load grouping file if it was already specified
  bool loadMask = !m_maskWS;
  if (loadMask && !groupFilename.empty()) {
    g_log.information() << "Loading Grouping file \"" << groupFilename
                        << "\"\n";
    if (groupFilename.find(".cal") != std::string::npos) {
      IAlgorithm_sptr alg = createChildAlgorithm("LoadDiffCal");
      alg->setProperty("InputWorkspace", m_inputW);
      alg->setPropertyValue("Filename", groupFilename);
      alg->setProperty<bool>("MakeCalWorkspace", false);
      alg->setProperty<bool>("MakeGroupingWorkspace", true);
      alg->setProperty<bool>("MakeMaskWorkspace", false);
      alg->setPropertyValue("WorkspaceName", m_instName);
      alg->executeAsChildAlg();

      // get the workspace
      m_groupWS = alg->getProperty("OutputGroupingWorkspace");
    } else {
      IAlgorithm_sptr alg = createChildAlgorithm("LoadDetectorsGroupingFile");
      alg->setProperty("InputFile", groupFilename);
      alg->executeAsChildAlg();

      // get the workspace
      m_groupWS = alg->getProperty("OutputWorkspace");
    }

    // register the workspace with the ADS
    const std::string name = m_instName + "_group";
    AnalysisDataService::Instance().addOrReplace(name, m_groupWS);
    this->setPropertyValue("GroupingWorkspace", name);
  }

  g_log.information() << "Loading Calibration file \"" << calFilename << "\"\n";

  // bunch of booleans to keep track of things
  bool loadGrouping = !m_groupWS;
  bool loadCalibration = !m_calibrationWS;

  IAlgorithm_sptr alg = createChildAlgorithm("LoadDiffCal");
  alg->setProperty("InputWorkspace", m_inputW);
  alg->setPropertyValue("Filename", calFilename);
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

    const std::string name = m_instName + "_group";
    AnalysisDataService::Instance().addOrReplace(name, m_groupWS);
    this->setPropertyValue("GroupingWorkspace", name);
  }
  if (loadCalibration) {
    m_calibrationWS = alg->getProperty("OutputCalWorkspace");

    const std::string name = m_instName + "_cal";
    AnalysisDataService::Instance().addOrReplace(name, m_calibrationWS);
    this->setPropertyValue("CalibrationWorkspace", name);
  }
  if (loadMask) {
    m_maskWS = alg->getProperty("OutputMaskWorkspace");

    const std::string name = m_instName + "_mask";
    AnalysisDataService::Instance().addOrReplace(name, m_maskWS);
    this->setPropertyValue("MaskWorkspace", name);
  }
}

//----------------------------------------------------------------------------------------------
/** Perform SortEvents on the output workspaces
 * but only if they are EventWorkspaces.
 *
 * @param ws :: any Workspace. Does nothing if not EventWorkspace.
 */
void AlignAndFocusPowder::doSortEvents(Mantid::API::Workspace_sptr ws) {
  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
  if (!eventWS)
    return;
  Algorithm_sptr alg = this->createChildAlgorithm("SortEvents");
  alg->setProperty("InputWorkspace", eventWS);
  alg->setPropertyValue("SortBy", "X Value");
  alg->executeAsChildAlg();
}

} // namespace WorkflowAlgorithm
} // namespace Mantid
