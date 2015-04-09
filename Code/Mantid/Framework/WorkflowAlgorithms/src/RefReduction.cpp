//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/RefReduction.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include <MantidAPI/FileFinder.h>
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/ListValidator.h"
#include "Poco/File.h"
#include "Poco/String.h"
#include "Poco/NumberFormatter.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RefReduction)

const std::string RefReduction::PolStateOffOff("entry-Off_Off");
const std::string RefReduction::PolStateOnOff("entry-On_Off");
const std::string RefReduction::PolStateOffOn("entry-Off_On");
const std::string RefReduction::PolStateOnOn("entry-On_On");
const std::string RefReduction::PolStateNone("entry");

const int RefReduction::NX_PIXELS(304);
const int RefReduction::NY_PIXELS(256);
const double RefReduction::PIXEL_SIZE(0.0007);

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void RefReduction::init() {
  declareProperty("DataRun", "", "Run number of the data set to be reduced");
  declareProperty(new ArrayProperty<int>("SignalPeakPixelRange"),
                  "Pixel range for the signal peak");

  declareProperty(
      "SubtractSignalBackground", false,
      "If true, the background will be subtracted from the signal peak");
  declareProperty(new ArrayProperty<int>("SignalBackgroundPixelRange"),
                  "Pixel range for background around the signal peak");

  declareProperty(
      "CropLowResDataAxis", false,
      "If true, the low-resolution pixel range will be limited to the"
      " range given by the LowResDataAxisPixelRange property");
  declareProperty(new ArrayProperty<int>("LowResDataAxisPixelRange"),
                  "Pixel range for the signal peak in the low-res direction");

  declareProperty("PerformNormalization", true,
                  "If true, the normalization will be performed");
  declareProperty("NormalizationRun", "",
                  "Run number of the normalization data set");
  declareProperty(new ArrayProperty<int>("NormPeakPixelRange"),
                  "Pixel range for the normalization peak");

  declareProperty("SubtractNormBackground", false,
                  "It true, the background will be subtracted"
                  " from the normalization peak");
  declareProperty(new ArrayProperty<int>("NormBackgroundPixelRange"),
                  "Pixel range for background around the normalization peak");

  declareProperty("CropLowResNormAxis", false,
                  "If true, the low-resolution pixel range"
                  " will be limited to be the range given by the "
                  "LowResNormAxisPixelRange property");
  declareProperty(
      new ArrayProperty<int>("LowResNormAxisPixelRange"),
      "Pixel range for the normalization peak in the low-res direction");

  declareProperty("Theta", EMPTY_DBL(),
                  "Scattering angle (takes precedence over meta data)");
  declareProperty("TOFMin", EMPTY_DBL(), "Minimum TOF cut");
  declareProperty("TOFMax", EMPTY_DBL(), "Maximum TOF cut");

  declareProperty("TOFStep", 400.0, "Step size of TOF histogram");
  declareProperty("NBins", EMPTY_INT(), "Number of bins in TOF histogram "
                                        "(takes precedence over TOFStep if "
                                        "given)");

  declareProperty("ReflectivityPixel", EMPTY_DBL());
  declareProperty("DetectorAngle", EMPTY_DBL());
  declareProperty("DetectorAngle0", EMPTY_DBL());
  declareProperty("DirectPixel", EMPTY_DBL());
  declareProperty("PolarizedData", true, "If true, the algorithm will look for "
                                         "polarization states in the data set");
  setPropertySettings(
      "ReflectivityPixel",
      new VisibleWhenProperty("Instrument", IS_EQUAL_TO, "REF_M"));
  setPropertySettings("DetectorAngle", new VisibleWhenProperty(
                                           "Instrument", IS_EQUAL_TO, "REF_M"));
  setPropertySettings(
      "DetectorAngle0",
      new VisibleWhenProperty("Instrument", IS_EQUAL_TO, "REF_M"));
  setPropertySettings("DirectPixel", new VisibleWhenProperty(
                                         "Instrument", IS_EQUAL_TO, "REF_M"));

  declareProperty("AngleOffset", EMPTY_DBL(),
                  "Scattering angle offset in degrees");
  setPropertySettings("AngleOffset", new VisibleWhenProperty(
                                         "Instrument", IS_EQUAL_TO, "REF_L"));

  std::vector<std::string> instrOptions;
  instrOptions.push_back("REF_L");
  instrOptions.push_back("REF_M");
  declareProperty("Instrument", "REF_M",
                  boost::make_shared<StringListValidator>(instrOptions),
                  "Instrument to reduce for");
  declareProperty("OutputWorkspacePrefix", "reflectivity",
                  "Prefix to give the output workspaces");
  declareProperty("OutputMessage", "", Direction::Output);
}

/// Execute algorithm
void RefReduction::exec() {
  const std::string instrument = getProperty("Instrument");
  m_output_message = "------ " + instrument + " reduction ------\n";

  // Process each polarization state
  if (getProperty("PolarizedData")) {
    processData(PolStateOffOff);
    processData(PolStateOnOff);
    processData(PolStateOffOn);
    processData(PolStateOnOn);
  } else {
    processData(PolStateNone);
  }
  setPropertyValue("OutputMessage", m_output_message);
}

MatrixWorkspace_sptr RefReduction::processData(const std::string polarization) {
  m_output_message += "Processing " + polarization + '\n';
  const std::string dataRun = getPropertyValue("DataRun");
  IEventWorkspace_sptr evtWS = loadData(dataRun, polarization);
  MatrixWorkspace_sptr dataWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(evtWS);
  MatrixWorkspace_sptr dataWSTof =
      boost::dynamic_pointer_cast<MatrixWorkspace>(evtWS);

  // If we have no events, stop here
  if (evtWS->getNumberEvents() == 0)
    return dataWS;

  // Get low-res pixel range
  int low_res_min = 0;
  int low_res_max = 0;
  const bool cropLowRes = getProperty("CropLowResDataAxis");
  const std::vector<int> lowResRange = getProperty("LowResDataAxisPixelRange");
  if (cropLowRes) {
    if (lowResRange.size() < 2) {
      g_log.error() << "LowResDataAxisPixelRange parameter should be a vector "
                       "of two values" << std::endl;
      throw std::invalid_argument("LowResDataAxisPixelRange parameter should "
                                  "be a vector of two values");
    }
    low_res_min = lowResRange[0];
    low_res_max = lowResRange[1];
    m_output_message += "    |Cropping low-res axis: [" +
                        Poco::NumberFormatter::format(low_res_min) + ", " +
                        Poco::NumberFormatter::format(low_res_max) + "]\n";
  }

  // Get peak range
  const std::vector<int> peakRange = getProperty("SignalPeakPixelRange");
  if (peakRange.size() < 2) {
    g_log.error()
        << "SignalPeakPixelRange parameter should be a vector of two values"
        << std::endl;
    throw std::invalid_argument(
        "SignalPeakPixelRange parameter should be a vector of two values");
  }

  // Get scattering angle in degrees
  double theta = getProperty("Theta");
  const std::string instrument = getProperty("Instrument");
  const bool integrateY = instrument.compare("REF_M") == 0;

  // Get pixel ranges in real pixels
  int xmin = 0;
  int xmax = 0;
  int ymin = 0;
  int ymax = 0;
  if (integrateY) {
    if (isEmpty(theta))
      theta = calculateAngleREFM(dataWS);
    if (!cropLowRes)
      low_res_max = NY_PIXELS - 1;
    xmin = 0;
    xmax = NX_PIXELS - 1;
    ymin = low_res_min;
    ymax = low_res_max;
  } else {
    if (isEmpty(theta))
      theta = calculateAngleREFL(dataWS);
    if (!cropLowRes)
      low_res_max = NX_PIXELS - 1;
    ymin = 0;
    ymax = NY_PIXELS - 1;
    xmin = low_res_min;
    xmax = low_res_max;
  }
  m_output_message += "    |Scattering angle: " +
                      Poco::NumberFormatter::format(theta, 6) + " deg\n";

  // Subtract background
  if (getProperty("SubtractSignalBackground")) {
    // Get background range
    const std::vector<int> bckRange = getProperty("SignalBackgroundPixelRange");
    if (bckRange.size() < 2) {
      g_log.error() << "SignalBackgroundPixelRange parameter should be a "
                       "vector of two values" << std::endl;
      throw std::invalid_argument("SignalBackgroundPixelRange parameter should "
                                  "be a vector of two values");
    }

    IAlgorithm_sptr convAlg =
        createChildAlgorithm("ConvertToMatrixWorkspace", 0.50, 0.55);
    convAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
    convAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
    convAlg->executeAsChildAlg();

    dataWS =
        subtractBackground(dataWS, dataWS, peakRange[0], peakRange[1],
                           bckRange[0], bckRange[1], low_res_min, low_res_max);
    m_output_message += "    |Subtracted background [" +
                        Poco::NumberFormatter::format(bckRange[0]) + ", " +
                        Poco::NumberFormatter::format(bckRange[1]) + "]\n";
  }

  // Process normalization run
  if (getProperty("PerformNormalization")) {
    MatrixWorkspace_sptr normWS = processNormalization();
    IAlgorithm_sptr rebinAlg =
        createChildAlgorithm("RebinToWorkspace", 0.50, 0.55);
    rebinAlg->setProperty<MatrixWorkspace_sptr>("WorkspaceToRebin", normWS);
    rebinAlg->setProperty<MatrixWorkspace_sptr>("WorkspaceToMatch", dataWS);
    rebinAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", normWS);
    rebinAlg->executeAsChildAlg();
    normWS = rebinAlg->getProperty("OutputWorkspace");

    IAlgorithm_sptr divAlg = createChildAlgorithm("Divide", 0.55, 0.65);
    divAlg->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", dataWS);
    divAlg->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", normWS);
    divAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
    divAlg->executeAsChildAlg();

    IAlgorithm_sptr repAlg =
        createChildAlgorithm("ReplaceSpecialValues", 0.55, 0.65);
    repAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
    repAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
    repAlg->setProperty("NaNValue", 0.0);
    repAlg->setProperty("NaNError", 0.0);
    repAlg->setProperty("InfinityValue", 0.0);
    repAlg->setProperty("InfinityError", 0.0);
    repAlg->executeAsChildAlg();
    m_output_message += "Normalization completed\n";
  }

  //    // Integrate over Y
  //    IAlgorithm_sptr refAlg = createChildAlgorithm("RefRoi", 0.90, 0.95);
  //    refAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
  //    refAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
  //    refAlg->setProperty("NXPixel", NX_PIXELS);
  //    refAlg->setProperty("NYPixel", NY_PIXELS);
  //    refAlg->setProperty("YPixelMin", ymin);
  //    refAlg->setProperty("YPixelMax", ymax);
  //    refAlg->setProperty("XPixelMin", xmin);
  //    refAlg->setProperty("XPixelMax", xmax);
  //    refAlg->setProperty("IntegrateY", integrateY);
  //    refAlg->setProperty("ScatteringAngle", theta);
  //    refAlg->executeAsChildAlg();
  //
  //    // Convert back to TOF
  //    IAlgorithm_sptr convAlgToTof = createChildAlgorithm("ConvertUnits",
  //    0.85, 0.90);
  //    convAlgToTof->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
  //    dataWS);
  //    convAlgToTof->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
  //    dataWSTof);
  //    convAlgToTof->setProperty("Target", "TOF");
  //    convAlgToTof->executeAsChildAlg();
  //
  //    MatrixWorkspace_sptr outputWS2 =
  //    convAlgToTof->getProperty("OutputWorkspace");
  //    declareProperty(new WorkspaceProperty<>("OutputWorkspace_jc_" +
  //    polarization, "TOF_"+polarization, Direction::Output));
  //    setProperty("OutputWorkspace_jc_" + polarization, outputWS2);

  // integrated over Y and keep in lambda scale
  IAlgorithm_sptr refAlg1 = createChildAlgorithm("RefRoi", 0.90, 0.95);
  refAlg1->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
  refAlg1->setProperty("NXPixel", NX_PIXELS);
  refAlg1->setProperty("NYPixel", NY_PIXELS);
  refAlg1->setProperty("ConvertToQ", false);
  refAlg1->setProperty("YPixelMin", ymin);
  refAlg1->setProperty("YPixelMax", ymax);
  refAlg1->setProperty("XPixelMin", xmin);
  refAlg1->setProperty("XPixelMax", xmax);
  refAlg1->setProperty("IntegrateY", integrateY);
  refAlg1->setProperty("ScatteringAngle", theta);
  refAlg1->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS2 = refAlg1->getProperty("OutputWorkspace");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace_jc_" + polarization,
                                          "Lambda_" + polarization,
                                          Direction::Output));
  setProperty("OutputWorkspace_jc_" + polarization, outputWS2);

  // Conversion to Q
  IAlgorithm_sptr refAlg = createChildAlgorithm("RefRoi", 0.90, 0.95);
  refAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
  refAlg->setProperty("NXPixel", NX_PIXELS);
  refAlg->setProperty("NYPixel", NY_PIXELS);
  refAlg->setProperty("ConvertToQ", true);

  refAlg->setProperty("YPixelMin", ymin);
  refAlg->setProperty("YPixelMax", ymax);
  refAlg->setProperty("XPixelMin", xmin);
  refAlg->setProperty("XPixelMax", xmax);
  refAlg->setProperty("IntegrateY", integrateY);
  refAlg->setProperty("ScatteringAngle", theta);
  refAlg->executeAsChildAlg();

  MatrixWorkspace_sptr output2DWS = refAlg->getProperty("OutputWorkspace");
  std::vector<int> spectra;
  for (int i = peakRange[0]; i < peakRange[1] + 1; i++)
    spectra.push_back(i);

  IAlgorithm_sptr grpAlg = createChildAlgorithm("GroupDetectors", 0.95, 0.99);
  grpAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", output2DWS);
  grpAlg->setProperty("SpectraList", spectra);
  grpAlg->executeAsChildAlg();

  MatrixWorkspace_sptr outputWS = grpAlg->getProperty("OutputWorkspace");

  const std::string prefix = getPropertyValue("OutputWorkspacePrefix");
  if (polarization.compare(PolStateNone) == 0) {
    declareProperty(
        new WorkspaceProperty<>("OutputWorkspace", prefix, Direction::Output));
    setProperty("OutputWorkspace", outputWS);
    declareProperty(new WorkspaceProperty<>("OutputWorkspace2D", "2D_" + prefix,
                                            Direction::Output));
    setProperty("OutputWorkspace2D", output2DWS);
  } else {
    std::string wsName = prefix + polarization;
    Poco::replaceInPlace(wsName, "entry", "");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace_" + polarization,
                                            wsName, Direction::Output));
    setProperty("OutputWorkspace_" + polarization, outputWS);
    declareProperty(new WorkspaceProperty<>("OutputWorkspace2D_" + polarization,
                                            "2D_" + wsName, Direction::Output));
    setProperty("OutputWorkspace2D_" + polarization, output2DWS);
  }
  m_output_message += "Reflectivity calculation completed\n";
  return outputWS;
}

MatrixWorkspace_sptr RefReduction::processNormalization() {
  m_output_message += "Processing normalization\n";

  const std::string normRun = getPropertyValue("NormalizationRun");
  IEventWorkspace_sptr evtWS = loadData(normRun, PolStateNone);
  MatrixWorkspace_sptr normWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(evtWS);

  const std::vector<int> peakRange = getProperty("NormPeakPixelRange");

  int low_res_min = 0;
  int low_res_max = 0;
  int xmin = 0;
  int xmax = 0;
  int ymin = 0;
  int ymax = 0;

  const bool cropLowRes = getProperty("CropLowResNormAxis");
  const std::vector<int> lowResRange = getProperty("LowResNormAxisPixelRange");
  if (cropLowRes) {
    if (lowResRange.size() < 2) {
      g_log.error() << "LowResNormAxisPixelRange parameter should be a vector "
                       "of two values" << std::endl;
      throw std::invalid_argument("LowResNormAxisPixelRange parameter should "
                                  "be a vector of two values");
    }
    low_res_min = lowResRange[0];
    low_res_max = lowResRange[1];
    m_output_message + "    |Cropping low-res axis: [" +
        Poco::NumberFormatter::format(low_res_min) + ", " +
        Poco::NumberFormatter::format(low_res_max) + "]\n";
  }

  const std::string instrument = getProperty("Instrument");
  const bool integrateY = instrument.compare("REF_M") == 0;
  if (integrateY) {
    if (!cropLowRes)
      low_res_max = NY_PIXELS - 1;
    xmin = peakRange[0];
    xmax = peakRange[1];
    ymin = low_res_min;
    ymax = low_res_max;
  } else {
    if (!cropLowRes)
      low_res_max = NX_PIXELS - 1;
    ymin = peakRange[0];
    ymax = peakRange[1];
    xmin = low_res_min;
    xmax = low_res_max;
  }

  if (getProperty("SubtractNormBackground")) {
    // Get background range
    const std::vector<int> bckRange = getProperty("NormBackgroundPixelRange");
    if (bckRange.size() < 2) {
      g_log.error() << "NormBackgroundPixelRange parameter should be a vector "
                       "of two values" << std::endl;
      throw std::invalid_argument("NormBackgroundPixelRange parameter should "
                                  "be a vector of two values");
    }

    IAlgorithm_sptr convAlg =
        createChildAlgorithm("ConvertToMatrixWorkspace", 0.50, 0.55);
    convAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", normWS);
    convAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", normWS);
    convAlg->executeAsChildAlg();

    normWS =
        subtractBackground(normWS, normWS, peakRange[0], peakRange[1],
                           bckRange[0], bckRange[1], low_res_min, low_res_max);
    m_output_message += "    |Subtracted background [" +
                        Poco::NumberFormatter::format(bckRange[0]) + ", " +
                        Poco::NumberFormatter::format(bckRange[1]) + "]\n";
  }
  IAlgorithm_sptr refAlg = createChildAlgorithm("RefRoi", 0.6, 0.65);
  refAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", normWS);
  refAlg->setProperty("NXPixel", NX_PIXELS);
  refAlg->setProperty("NYPixel", NY_PIXELS);
  refAlg->setProperty("ConvertToQ", false);
  refAlg->setProperty("SumPixels", true);
  refAlg->setProperty("NormalizeSum", true);
  refAlg->setProperty("AverageOverIntegratedAxis", integrateY);
  refAlg->setProperty("YPixelMin", ymin);
  refAlg->setProperty("YPixelMax", ymax);
  refAlg->setProperty("XPixelMin", xmin);
  refAlg->setProperty("XPixelMax", xmax);
  refAlg->setProperty("IntegrateY", integrateY);
  refAlg->executeAsChildAlg();

  MatrixWorkspace_sptr outputNormWS = refAlg->getProperty("OutputWorkspace");
  return outputNormWS;
}

IEventWorkspace_sptr RefReduction::loadData(const std::string dataRun,
                                            const std::string polarization) {
  const std::string instrument = getProperty("Instrument");

  // Check whether dataRun refers to an existing workspace
  // Create a good name for the raw workspace
  std::string ws_name = "__ref_" + dataRun + "-" + polarization + "_raw";
  IEventWorkspace_sptr rawWS;
  if (AnalysisDataService::Instance().doesExist(dataRun)) {
    rawWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(dataRun);
    g_log.notice() << "Found workspace: " << dataRun << std::endl;
    m_output_message += "    |Input data run is a workspace: " + dataRun + "\n";
  } else if (AnalysisDataService::Instance().doesExist(ws_name)) {
    rawWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(ws_name);
    g_log.notice() << "Using existing workspace: " << ws_name << std::endl;
    m_output_message +=
        "    |Found workspace from previous reduction: " + ws_name + "\n";
  } else {
    // If we can't find a workspace, find a file to load
    std::string path = FileFinder::Instance().getFullPath(dataRun);

    if (path.size() == 0 || !Poco::File(path).exists()) {
      try {
        std::vector<std::string> paths =
            FileFinder::Instance().findRuns(instrument + dataRun);
        path = paths[0];
      } catch (Exception::NotFoundError &) { /* Pass. We report the missing file
                                                later. */
      }
    }

    if (path.size() == 0 || !Poco::File(path).exists()) {
      try {
        std::vector<std::string> paths =
            FileFinder::Instance().findRuns(dataRun);
        path = paths[0];
      } catch (Exception::NotFoundError &) { /* Pass. We report the missing file
                                                later. */
      }
    }

    if (Poco::File(path).exists()) {
      g_log.notice() << "Found: " << path << std::endl;
      m_output_message += "    |Loading from " + path + "\n";
      IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadEventNexus", 0, 0.2);
      loadAlg->setProperty("Filename", path);
      if (polarization.compare(PolStateNone) != 0)
        loadAlg->setProperty("NXentryName", polarization);
      loadAlg->executeAsChildAlg();
      rawWS = loadAlg->getProperty("OutputWorkspace");
      if (rawWS->getNumberEvents() == 0) {
        g_log.notice() << "No data in " << polarization << std::endl;
        m_output_message += "    |No data for " + polarization + "\n";
        return rawWS;
      }

      // Move the detector to the right position
      if (instrument.compare("REF_M") == 0) {
        double det_distance =
            rawWS->getInstrument()->getDetector(0)->getPos().Z();
        Mantid::Kernel::Property *prop =
            rawWS->run().getProperty("SampleDetDis");
        Mantid::Kernel::TimeSeriesProperty<double> *dp =
            dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
        double sdd = dp->getStatistics().mean / 1000.0;
        IAlgorithm_sptr mvAlg =
            createChildAlgorithm("MoveInstrumentComponent", 0.2, 0.25);
        mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", rawWS);
        mvAlg->setProperty("ComponentName", "detector1");
        mvAlg->setProperty("Z", sdd - det_distance);
        mvAlg->setProperty("RelativePosition", true);
        mvAlg->executeAsChildAlg();
        g_log.notice() << "Ensuring correct Z position: Correction = "
                       << Poco::NumberFormatter::format(sdd - det_distance)
                       << " m" << std::endl;
      }
      AnalysisDataService::Instance().addOrReplace(ws_name, rawWS);
    } else {
      g_log.error() << "Could not find a data file for " << dataRun
                    << std::endl;
      throw std::invalid_argument(
          "Could not find a data file for the given input");
    }
  }

  // Crop TOF as needed and set binning
  double tofMin = getProperty("TOFMin");
  double tofMax = getProperty("TOFMax");
  if (isEmpty(tofMin) || isEmpty(tofMax)) {
    const MantidVec &x = rawWS->readX(0);
    if (isEmpty(tofMin))
      tofMin = *std::min_element(x.begin(), x.end());
    if (isEmpty(tofMax))
      tofMax = *std::max_element(x.begin(), x.end());
  }

  int nBins = getProperty("NBins");
  double tofStep = getProperty("TOFStep");
  if (!isEmpty(nBins))
    tofStep = (tofMax - tofMin) / nBins;
  else
    nBins = (int)floor((tofMax - tofMin) / tofStep);

  std::vector<double> params;
  params.push_back(tofMin);
  params.push_back(tofStep);
  params.push_back(tofMax);

  IAlgorithm_sptr rebinAlg = createChildAlgorithm("Rebin", 0.25, 0.3);
  rebinAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", rawWS);
  rebinAlg->setProperty("Params", params);
  rebinAlg->setProperty("PreserveEvents", true);
  rebinAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = rebinAlg->getProperty("OutputWorkspace");
  m_output_message += "    |TOF binning: " +
                      Poco::NumberFormatter::format(tofMin) + " to " +
                      Poco::NumberFormatter::format(tofMax) + " in steps of " +
                      Poco::NumberFormatter::format(tofStep) + " microsecs\n";

  // Normalise by current
  IAlgorithm_sptr normAlg =
      createChildAlgorithm("NormaliseByCurrent", 0.3, 0.35);
  normAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
  // normAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
  normAlg->executeAsChildAlg();
  outputWS = normAlg->getProperty("OutputWorkspace");

  // Convert to wavelength
  IAlgorithm_sptr convAlg = createChildAlgorithm("ConvertUnits", 0.35, 0.4);
  convAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
  convAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
  convAlg->setProperty("Target", "Wavelength");
  convAlg->executeAsChildAlg();

  // Rebin in wavelength
  const MantidVec &x = outputWS->readX(0);
  double wlMin = *std::min_element(x.begin(), x.end());
  double wlMax = *std::max_element(x.begin(), x.end());

  std::vector<double> wl_params;
  wl_params.push_back(wlMin);
  wl_params.push_back((wlMax - wlMin) / nBins);
  wl_params.push_back(wlMax);

  IAlgorithm_sptr rebinAlg2 = createChildAlgorithm("Rebin", 0.25, 0.3);
  rebinAlg2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
  rebinAlg2->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWS);
  rebinAlg2->setProperty("Params", wl_params);
  rebinAlg2->setProperty("PreserveEvents", true);
  rebinAlg2->executeAsChildAlg();

  IEventWorkspace_sptr outputEvtWS =
      boost::dynamic_pointer_cast<IEventWorkspace>(outputWS);
  return outputEvtWS;
}

double RefReduction::calculateAngleREFM(MatrixWorkspace_sptr workspace) {
  double dangle = getProperty("DetectorAngle");
  if (isEmpty(dangle)) {
    Mantid::Kernel::Property *prop = workspace->run().getProperty("DANGLE");
    Mantid::Kernel::TimeSeriesProperty<double> *dp =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
    dangle = dp->getStatistics().mean;
  }

  double dangle0 = getProperty("DetectorAngle0");
  if (isEmpty(dangle0)) {
    Mantid::Kernel::Property *prop = workspace->run().getProperty("DANGLE0");
    Mantid::Kernel::TimeSeriesProperty<double> *dp =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
    dangle0 = dp->getStatistics().mean;
  }

  Mantid::Kernel::Property *prop = workspace->run().getProperty("SampleDetDis");
  Mantid::Kernel::TimeSeriesProperty<double> *dp =
      dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
  const double det_distance = dp->getStatistics().mean / 1000.0;

  double direct_beam_pix = getProperty("DirectPixel");
  if (isEmpty(direct_beam_pix)) {
    Mantid::Kernel::Property *prop = workspace->run().getProperty("DIRPIX");
    Mantid::Kernel::TimeSeriesProperty<double> *dp =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
    direct_beam_pix = dp->getStatistics().mean;
  }

  double ref_pix = getProperty("ReflectivityPixel");
  if (ref_pix == 0 || isEmpty(ref_pix)) {
    const std::vector<int> peakRange = getProperty("SignalPeakPixelRange");
    if (peakRange.size() < 2) {
      g_log.error()
          << "SignalPeakPixelRange parameter should be a vector of two values"
          << std::endl;
      throw std::invalid_argument(
          "SignalPeakPixelRange parameter should be a vector of two values");
    }
    ref_pix = (peakRange[0] + peakRange[1]) / 2.0;
  }

  double theta =
      (dangle - dangle0) * M_PI / 180.0 / 2.0 +
      ((direct_beam_pix - ref_pix) * PIXEL_SIZE) / (2.0 * det_distance);

  return theta * 180.0 / M_PI;
}

double RefReduction::calculateAngleREFL(MatrixWorkspace_sptr workspace) {
  Mantid::Kernel::Property *prop = workspace->run().getProperty("ths");
  Mantid::Kernel::TimeSeriesProperty<double> *dp =
      dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
  const double ths = dp->getStatistics().mean;

  prop = workspace->run().getProperty("tthd");
  dp = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
  const double tthd = dp->getStatistics().mean;

  double offset = getProperty("AngleOffset");
  if (isEmpty(offset))
    offset = 0.0;
  return tthd - ths + offset;
}

MatrixWorkspace_sptr RefReduction::subtractBackground(
    MatrixWorkspace_sptr dataWS, MatrixWorkspace_sptr rawWS, int peakMin,
    int peakMax, int bckMin, int bckMax, int lowResMin, int lowResMax) {
  const std::string instrument = getProperty("Instrument");
  const bool integrateY = instrument.compare("REF_M") == 0;

  int xmin = 0;
  int xmax = 0;
  int ymin = 0;
  int ymax = 0;
  if (integrateY) {
    ymin = lowResMin;
    ymax = lowResMax;
  } else {
    xmin = lowResMin;
    xmax = lowResMax;
  }

  // Look for overlap with data peak
  if (bckMin < peakMin && bckMax > peakMax) {
    // Background on the left
    if (integrateY) {
      xmin = bckMin;
      xmax = peakMin - 1;
    } else {
      ymin = bckMin;
      ymax = peakMin - 1;
    }
    IAlgorithm_sptr leftAlg = createChildAlgorithm("RefRoi", 0.6, 0.65);
    leftAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", rawWS);
    leftAlg->setProperty("NXPixel", NX_PIXELS);
    leftAlg->setProperty("NYPixel", NY_PIXELS);
    leftAlg->setProperty("ConvertToQ", false);
    leftAlg->setProperty("SumPixels", true);
    leftAlg->setProperty("NormalizeSum", true);
    leftAlg->setProperty("AverageOverIntegratedAxis", integrateY);
    leftAlg->setProperty("YPixelMin", ymin);
    leftAlg->setProperty("YPixelMax", ymax);
    leftAlg->setProperty("XPixelMin", xmin);
    leftAlg->setProperty("XPixelMax", xmax);
    leftAlg->setProperty("IntegrateY", integrateY);
    leftAlg->executeAsChildAlg();

    MatrixWorkspace_sptr leftWS = leftAlg->getProperty("OutputWorkspace");

    // Background on the right
    if (integrateY) {
      xmin = peakMax + 1;
      xmax = bckMax;
    } else {
      ymin = peakMax + 1;
      ymax = bckMax;
    }
    IAlgorithm_sptr rightAlg = createChildAlgorithm("RefRoi", 0.6, 0.65);
    rightAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", rawWS);
    rightAlg->setProperty("NXPixel", NX_PIXELS);
    rightAlg->setProperty("NYPixel", NY_PIXELS);
    rightAlg->setProperty("ConvertToQ", false);
    rightAlg->setProperty("SumPixels", true);
    rightAlg->setProperty("NormalizeSum", true);
    rightAlg->setProperty("AverageOverIntegratedAxis", integrateY);
    rightAlg->setProperty("YPixelMin", ymin);
    rightAlg->setProperty("YPixelMax", ymax);
    rightAlg->setProperty("XPixelMin", xmin);
    rightAlg->setProperty("XPixelMax", xmax);
    rightAlg->setProperty("IntegrateY", integrateY);
    rightAlg->executeAsChildAlg();

    MatrixWorkspace_sptr rightWS = rightAlg->getProperty("OutputWorkspace");

    // Average the two sides and subtract from peak
    dataWS = dataWS - (leftWS + rightWS) / 2.0;
    return dataWS;

  } else {

    // Check for overlaps
    if (bckMax > peakMin && bckMax < peakMax) {
      g_log.notice() << "Background range overlaps with peak" << std::endl;
      bckMax = peakMin - 1;
    }
    if (bckMin < peakMax && bckMin > peakMin) {
      g_log.notice() << "Background range overlaps with peak" << std::endl;
      bckMin = peakMax + 1;
    }

    if (integrateY) {
      xmin = bckMin;
      xmax = bckMax;
    } else {
      ymin = bckMin;
      ymax = bckMax;
    }

    IAlgorithm_sptr refAlg = createChildAlgorithm("RefRoi", 0.6, 0.65);
    refAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", rawWS);
    refAlg->setProperty("NXPixel", NX_PIXELS);
    refAlg->setProperty("NYPixel", NY_PIXELS);
    refAlg->setProperty("ConvertToQ", false);
    refAlg->setProperty("SumPixels", true);
    refAlg->setProperty("NormalizeSum", true);
    refAlg->setProperty("AverageOverIntegratedAxis", integrateY);
    refAlg->setProperty("YPixelMin", ymin);
    refAlg->setProperty("YPixelMax", ymax);
    refAlg->setProperty("XPixelMin", xmin);
    refAlg->setProperty("XPixelMax", xmax);
    refAlg->setProperty("IntegrateY", integrateY);
    refAlg->executeAsChildAlg();

    MatrixWorkspace_sptr cropWS = refAlg->getProperty("OutputWorkspace");

    dataWS = dataWS - cropWS;
    return dataWS;
  }
}

} // namespace Algorithms
} // namespace Mantid
