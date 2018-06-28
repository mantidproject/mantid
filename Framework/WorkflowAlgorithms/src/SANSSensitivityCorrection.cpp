#include "MantidWorkflowAlgorithms/SANSSensitivityCorrection.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidNexus/NexusFileIO.h"

#include <Poco/File.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Path.h>
#include <Poco/String.h>

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSensitivityCorrection)

using namespace API;
using namespace DataObjects;
using namespace Geometry;
using namespace Kernel;

void SANSSensitivityCorrection::init() {
  declareProperty(make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "", Direction::Input, PropertyMode::Optional));
  const std::vector<std::string> fileExts{"_event.nxs", ".xml", ".nxs"};
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Load, fileExts),
                  "Flood field or sensitivity file.");
  declareProperty("UseSampleDC", true, "If true, the dark current subtracted "
                                       "from the sample data will also be "
                                       "subtracted from the flood field.");
  declareProperty(
      Kernel::make_unique<API::FileProperty>(
          "DarkCurrentFile", "", API::FileProperty::OptionalLoad, fileExts),
      "The name of the input file to load as dark current.");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty(
      "MinEfficiency", EMPTY_DBL(), positiveDouble,
      "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty(
      "MaxEfficiency", EMPTY_DBL(), positiveDouble,
      "Maximum efficiency for a pixel to be considered (default: no maximum).");
  declareProperty("FloodTransmissionValue", EMPTY_DBL(), positiveDouble,
                  "Transmission value for the flood field material "
                  "(default: no transmission).");
  declareProperty("FloodTransmissionError", 0.0, positiveDouble,
                  "Transmission error for the flood field material "
                  "(default: no transmission).");
  declareProperty("BeamCenterX", EMPTY_DBL(),
                  "Beam position in X pixel coordinates (optional: otherwise "
                  "sample beam center is used)");
  declareProperty("BeamCenterY", EMPTY_DBL(),
                  "Beam position in Y pixel coordinates (optional: otherwise "
                  "sample beam center is used)");
  declareProperty("MaskedFullComponent", "",
                  "Component Name to fully mask according to the IDF file.");
  declareProperty(
      make_unique<ArrayProperty<int>>("MaskedEdges"),
      "Number of pixels to mask on the edges: X-low, X-high, Y-low, Y-high");
  declareProperty(
      "MaskedComponent", "",
      "Component Name to mask the edges according to the IDF file.");

  declareProperty(make_unique<WorkspaceProperty<>>(
      "OutputWorkspace", "", Direction::Output, PropertyMode::Optional));
  declareProperty("ReductionProperties", "__sans_reduction_properties");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "OutputSensitivityWorkspace", "", Direction::Output,
      PropertyMode::Optional));
  declareProperty("OutputMessage", "", Direction::Output);

  // Normalisation
  const std::string norm_grp = "Normalisation";
  std::vector<std::string> incidentBeamNormOptions;
  incidentBeamNormOptions.emplace_back("None");
  // The data will be normalised to the monitor counts
  incidentBeamNormOptions.emplace_back("Monitor");
  // The data will be normalised to the acquisition time
  incidentBeamNormOptions.emplace_back("Timer");
  declareProperty(
      "Normalisation", "None",
      boost::make_shared<StringListValidator>(incidentBeamNormOptions),
      "Options for data normalisation");
  setPropertyGroup("Normalisation", norm_grp);

  // Transmission
  const std::string trans_grp = "Transmission";
  std::vector<std::string> transOptions{"None", "Value", "DirectBeam"};
  declareProperty("TransmissionMethod", "Value",
                  boost::make_shared<StringListValidator>(transOptions),
                  "Transmission determination method");

  // - Direct beam method transmission calculation
  declareProperty(
      "TransmissionBeamRadius", 3.0,
      "Radius of the beam area used to compute the transmission [pixels]");
  setPropertySettings("TransmissionBeamRadius",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("TransmissionEmptyDataFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "Empty data file for transmission calculation");
  setPropertySettings("TransmissionEmptyDataFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      make_unique<API::FileProperty>("TransmissionSampleDataFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "Sample data file for transmission calculation");
  setPropertySettings("TransmissionSampleDataFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  // Transmission beam center
  const std::vector<std::string> centerOptions{"None", "Value", "DirectBeam"};
  declareProperty("TransmissionBeamCenterMethod", "None",
                  boost::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the transmission data beam center");
  setPropertySettings("TransmissionBeamCenterMethod",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  //    Option 1: Set beam center by hand
  declareProperty("TransmissionBeamCenterX", EMPTY_DBL(),
                  "Transmission beam center location in X [pixels]");
  setPropertySettings("TransmissionBeamCenterX",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("TransmissionBeamCenterY", EMPTY_DBL(),
                  "Transmission beam center location in Y [pixels]");
  setPropertySettings("TransmissionBeamCenterY",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(
      make_unique<API::FileProperty>("TransmissionBeamCenterFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "The name of the input data file to load");
  setPropertySettings("TransmissionBeamCenterFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      make_unique<API::FileProperty>("TransmissionDarkCurrentFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "The name of the input data file to load as transmission dark current.");
  setPropertySettings("TransmissionDarkCurrentFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      "TransmissionUseSampleDC", true,
      "If true, the sample dark current will be used IF a dark current file is"
      "not set.");
  setPropertySettings("TransmissionUseSampleDC",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      "ThetaDependentTransmission", true,
      "If true, a theta-dependent transmission correction will be applied.");

  setPropertySettings("ThetaDependentTransmission",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_NOT_EQUAL_TO, "None"));

  setPropertySettings("FloodTransmissionValue",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "Value"));

  setPropertySettings("FloodTransmissionError",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "Value"));

  // -- Define group --
  setPropertyGroup("TransmissionMethod", trans_grp);
  setPropertyGroup("TransmissionBeamRadius", trans_grp);
  setPropertyGroup("TransmissionEmptyDataFile", trans_grp);
  setPropertyGroup("TransmissionSampleDataFile", trans_grp);
  setPropertyGroup("TransmissionBeamCenterMethod", trans_grp);
  setPropertyGroup("TransmissionBeamCenterX", trans_grp);
  setPropertyGroup("TransmissionBeamCenterY", trans_grp);
  setPropertyGroup("TransmissionBeamCenterFile", trans_grp);
  setPropertyGroup("TransmissionDarkCurrentFile", trans_grp);
  setPropertyGroup("TransmissionUseSampleDC", trans_grp);
  setPropertyGroup("ThetaDependentTransmission", trans_grp);

  // Background options
  const std::string bck_grp = "Background";
  declareProperty("BackgroundFiles", "", "Background data files");
  declareProperty("BckTransmissionMethod", "None",
                  boost::make_shared<StringListValidator>(transOptions),
                  "Transmission determination method");

  // - Transmission value entered by hand
  declareProperty("BckTransmissionValue", EMPTY_DBL(), positiveDouble,
                  "Transmission value.");
  setPropertySettings("BckTransmissionValue",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_EQUAL_TO, "Value"));

  declareProperty("BckTransmissionError", EMPTY_DBL(), positiveDouble,
                  "Transmission error.");
  setPropertySettings("BckTransmissionError",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_EQUAL_TO, "Value"));

  // - Direct beam method transmission calculation
  declareProperty(
      "BckTransmissionBeamRadius", 3.0,
      "Radius of the beam area used to compute the transmission [pixels]");
  setPropertySettings("BckTransmissionBeamRadius",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionSampleDataFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "Sample data file for transmission calculation");
  setPropertySettings("BckTransmissionSampleDataFile",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionEmptyDataFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "Empty data file for transmission calculation");
  setPropertySettings("BckTransmissionEmptyDataFile",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  // - transmission beam center
  declareProperty("BckTransmissionBeamCenterMethod", "None",
                  boost::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the transmission data beam center");
  setPropertySettings("BckTransmissionBeamCenterMethod",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  //    Option 1: Set beam center by hand
  declareProperty("BckTransmissionBeamCenterX", EMPTY_DBL(),
                  "Transmission beam center location in X [pixels]");
  setPropertySettings("BckTransmissionBeamCenterX",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("BckTransmissionBeamCenterY", EMPTY_DBL(),
                  "Transmission beam center location in Y [pixels]");
  //    Option 2: Find it (expose properties from FindCenterOfMass)
  setPropertySettings("BckTransmissionBeamCenterY",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionBeamCenterFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "The name of the input data file to load");
  setPropertySettings("BckTransmissionBeamCenterFile",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionDarkCurrentFile", "",
                                     API::FileProperty::OptionalLoad, fileExts),
      "The name of the input data file to load as background "
      "transmission dark current.");
  setPropertySettings("BckTransmissionDarkCurrentFile",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_EQUAL_TO,
                                                       "DirectBeam"));
  setPropertySettings("BckTransmissionDarkCurrentFile",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_EQUAL_TO,
                                                       "DirectBeam"));
  declareProperty(
      "BckTransmissionUseSampleDC", true,
      "If true, the sample dark current will be used IF a dark current file is"
      "not set.");
  setPropertySettings("BckTransmissionUseSampleDC",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      "BckThetaDependentTransmission", true,
      "If true, a theta-dependent transmission correction will be applied.");
  setPropertySettings("BckThetaDependentTransmission",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_NOT_EQUAL_TO,
                                                       "None"));
  setPropertyGroup("BackgroundFiles", bck_grp);
  setPropertyGroup("BckTransmissionMethod", bck_grp);
  setPropertyGroup("BckTransmissionValue", bck_grp);
  setPropertyGroup("BckTransmissionError", bck_grp);
  setPropertyGroup("BckTransmissionBeamRadius", bck_grp);
  setPropertyGroup("BckTransmissionSampleDataFile", bck_grp);
  setPropertyGroup("BckTransmissionEmptyDataFile", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterMethod", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterX", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterY", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterFile", bck_grp);
  setPropertyGroup("BckTransmissionDarkCurrentFile", bck_grp);
  setPropertyGroup("BckTransmissionUseSampleDC", bck_grp);
  setPropertyGroup("BckThetaDependentTransmission", bck_grp);
}

/**checks the file by opening it and reading few lines
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file
 */
bool SANSSensitivityCorrection::fileCheck(const std::string &filePath) {
  // Check the file extension
  Poco::Path path(filePath);
  const std::string extn = path.getExtension();
  const std::string nxs("nxs");
  const std::string nx5("nx5");
  if (!(Poco::icompare(nxs, extn) == 0 || Poco::icompare(nx5, extn) == 0))
    return false;

  // If we have a Nexus file, check that is comes from Mantid
  std::vector<std::string> entryName, definition;
  int count = NeXus::getNexusEntryTypes(filePath, entryName, definition);
  if (count <= -1) {
    g_log.error("Error reading file " + filePath);
    throw Exception::FileError("Unable to read data in File:", filePath);
  } else if (count == 0) {
    // this is normal for ILL SANS nexus files, so do not log as error or
    // warning
    g_log.information("No entries found in " + filePath);
    return false;
  }

  return entryName[0] == "mantid_workspace_1";
}

void SANSSensitivityCorrection::process(
    MatrixWorkspace_sptr rawFloodWS, const std::string reductionManagerName,
    boost::shared_ptr<PropertyManager> reductionManager,
    const std::string propertyPrefix) {
  const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");

  // Look for a dark current subtraction algorithm
  std::string dark_result;
  if (reductionManager->existsProperty("DarkCurrentAlgorithm")) {
    IAlgorithm_sptr darkAlg =
        reductionManager->getProperty("DarkCurrentAlgorithm");
    darkAlg->setChild(true);
    darkAlg->setProperty("InputWorkspace", rawFloodWS);
    darkAlg->setPropertyValue("OutputWorkspace", "__unused");

    // Execute as-is if we use the sample dark current, otherwise check
    // whether a dark current file was provided.
    // Otherwise do nothing
    if (getProperty("UseSampleDC")) {
      darkAlg->execute();
      if (darkAlg->existsProperty("OutputMessage"))
        dark_result = darkAlg->getPropertyValue("OutputMessage");
    } else if (!darkCurrentFile.empty()) {
      darkAlg->setProperty("Filename", darkCurrentFile);
      darkAlg->setProperty("PersistentCorrection", false);
      darkAlg->execute();
      if (darkAlg->existsProperty("OutputMessage"))
        dark_result = darkAlg->getPropertyValue("OutputMessage");
      else
        dark_result = "   Dark current subtracted\n";
    }
    rawFloodWS = darkAlg->getProperty("OutputWorkspace");
  } else if (!darkCurrentFile.empty()) {
    // We need to subtract the dark current for the flood field but no
    // dark
    // current subtraction was set for the sample! Use the default dark
    // current algorithm if we can find it.
    if (reductionManager->existsProperty("DefaultDarkCurrentAlgorithm")) {
      IAlgorithm_sptr darkAlg =
          reductionManager->getProperty("DefaultDarkCurrentAlgorithm");
      darkAlg->setChild(true);
      darkAlg->setProperty("InputWorkspace", rawFloodWS);
      darkAlg->setPropertyValue("OutputWorkspace", "__unused");
      darkAlg->setProperty("Filename", darkCurrentFile);
      darkAlg->setProperty("PersistentCorrection", false);
      darkAlg->execute();
      rawFloodWS = darkAlg->getProperty("OutputWorkspace");
      if (darkAlg->existsProperty("OutputMessage"))
        dark_result = darkAlg->getPropertyValue("OutputMessage");
    } else {
      // We are running out of options
      g_log.error() << "No dark current algorithm provided to load ["
                    << getPropertyValue("DarkCurrentFile") << "]: skipped!\n";
      dark_result = "   No dark current algorithm provided: skipped\n";
    }
  }
  m_output_message +=
      "   |" + Poco::replace(dark_result, "\n", "\n   |") + "\n";

  // Normalise the flood run if requested
  const std::string normalization = getProperty("Normalisation");
  if (!boost::contains(normalization, "None")) {
    m_output_message += "     |Normalising the flood run by " + normalization;
    IAlgorithm_sptr normAlg;
    if (reductionManager->existsProperty("NormaliseAlgorithm")) {
      normAlg = reductionManager->getProperty("NormaliseAlgorithm");
    } else {
      normAlg = createChildAlgorithm("HFIRSANSNormalise");
    }
    normAlg->setProperty("NormalisationType", normalization);
    normAlg->setProperty("InputWorkspace", rawFloodWS);
    normAlg->setPropertyValue("OutputWorkspace", "__unused");
    normAlg->setChild(true);
    normAlg->execute();
    rawFloodWS =normAlg->getProperty("OutputWorkspace");
  }

  // Look for solid angle correction algorithm
  if (reductionManager->existsProperty("SANSSolidAngleCorrection")) {
    IAlgorithm_sptr solidAlg =
        reductionManager->getProperty("SANSSolidAngleCorrection");
    solidAlg->setChild(true);
    solidAlg->setProperty("InputWorkspace", rawFloodWS);
    solidAlg->setPropertyValue("OutputWorkspace", "__unused");
    solidAlg->execute();
    rawFloodWS = solidAlg->getProperty("OutputWorkspace");
    std::string msg = "Solid angle correction applied\n";
    if (solidAlg->existsProperty("OutputMessage"))
      msg = solidAlg->getPropertyValue("OutputMessage");
    m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
  }

  // Apply transmission correction as needed
  const std::string transmissionMethod =
      getPropertyValue(propertyPrefix + "TransmissionMethod");
  if (transmissionMethod == "Value") {
    std::string trValue = "TransmissionValue";
    std::string trError = "TransmissionError";
    // this is not to break the backwards compatibility
    if (propertyPrefix.empty()) {
      trValue = "Flood" + trValue;
      trError = "Flood" + trError;
    }

    double floodTransmissionValue = getProperty(trValue);
    double floodTransmissionError = getProperty(trError);

    if (!isEmpty(floodTransmissionValue)) {
      g_log.debug() << "SANSSensitivityCorrection :: Applying transmission "
                       "to flood field\n";
      IAlgorithm_sptr transAlg =
          createChildAlgorithm("ApplyTransmissionCorrection");
      transAlg->setProperty("InputWorkspace", rawFloodWS);
      transAlg->setPropertyValue("OutputWorkspace", "__unused");
      transAlg->setProperty("TransmissionValue", floodTransmissionValue);
      transAlg->setProperty("TransmissionError", floodTransmissionError);
      const bool thetaDependent =
          getProperty(propertyPrefix + "ThetaDependentTransmission");
      transAlg->setProperty("ThetaDependent", thetaDependent);
      transAlg->execute();
      rawFloodWS = transAlg->getProperty("OutputWorkspace");
      m_output_message += "   |Applied transmission to flood field\n";
    }
  } else if (transmissionMethod == "DirectBeam") {
    g_log.debug() << "SANSSensitivityCorrection :: Calculating flood "
                     "transmission with direct beam method\n";
    const std::string emptyFilename =
        getPropertyValue(propertyPrefix + "TransmissionEmptyDataFile");
    const std::string sampleFilename =
        getPropertyValue(propertyPrefix + "TransmissionSampleDataFile");
    const bool thetaDependentTrans =
        getProperty(propertyPrefix + "ThetaDependentTransmission");
    const double beamRadius =
        getProperty(propertyPrefix + "TransmissionBeamRadius");
    const double beamX =
        getProperty(propertyPrefix + "TransmissionBeamCenterX");
    const double beamY =
        getProperty(propertyPrefix + "TransmissionBeamCenterY");
    const std::string darkCurrent =
        getPropertyValue(propertyPrefix + "TransmissionDarkCurrentFile");
    const bool useSampleDC =
        getProperty(propertyPrefix + "TransmissionUseSampleDC");
    IAlgorithm_sptr transAlg =
        createChildAlgorithm("SANSDirectBeamTransmission");
    transAlg->setProperty("InputWorkspace", rawFloodWS);
    transAlg->setPropertyValue("OutputWorkspace", "__unused");
    transAlg->setProperty("EmptyDataFilename", emptyFilename);
    transAlg->setProperty("SampleDataFilename", sampleFilename);
    transAlg->setProperty("BeamRadius", beamRadius);
    transAlg->setProperty("DarkCurrentFilename", darkCurrent);
    transAlg->setProperty("UseSampleDarkCurrent", useSampleDC);
    transAlg->setProperty("ThetaDependent", thetaDependentTrans);
    transAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    const std::string transmissionBeamCenterMethod =
        getPropertyValue(propertyPrefix + "TransmissionBeamCenterMethod");
    // Beam center option for transmission data
    if (boost::iequals(transmissionBeamCenterMethod, "Value") &&
        !isEmpty(beamX) && !isEmpty(beamY)) {
      transAlg->setProperty("BeamCenterX", beamX);
      transAlg->setProperty("BeamCenterY", beamY);
    } else if (boost::iequals(transmissionBeamCenterMethod, "DirectBeam")) {
      const std::string beamCenterFile =
          getProperty(propertyPrefix + "TransmissionBeamCenterFile");
      if (!beamCenterFile.empty()) {
        IAlgorithm_sptr ctrAlg = createChildAlgorithm("SANSBeamFinder");
        ctrAlg->setProperty("Filename", beamCenterFile);
        ctrAlg->setProperty("UseDirectBeamMethod", true);
        ctrAlg->setProperty("PersistentCorrection", false);
        ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);
        ctrAlg->execute();
        const double foundBeamCenterX = ctrAlg->getProperty("FoundBeamCenterX");
        const double foundBeamCenterY = ctrAlg->getProperty("FoundBeamCenterY");
        transAlg->setProperty("BeamCenterX", foundBeamCenterX);
        transAlg->setProperty("BeamCenterY", foundBeamCenterY);
        m_output_message +=
            "    |Found beam center for the flood transmission X=" +
            std::to_string(foundBeamCenterX) + ", Y=" +
            std::to_string(foundBeamCenterY) + "\n";
      } else {
        throw std::runtime_error(
            "ERROR: Transmission beam center determination was required"
            " but no file was provided\n");
      }
    }
    m_output_message +=
        "   |Calculated and applied direct beam transmission to flood field\n";
    transAlg->execute();
    rawFloodWS = transAlg->getProperty("OutputWorkspace");
  }
}

MatrixWorkspace_sptr SANSSensitivityCorrection::load(
    boost::shared_ptr<PropertyManager> reductionManager,
    const std::string fileName, Poco::Path path) {
  double center_x = getProperty("BeamCenterX");
  double center_y = getProperty("BeamCenterY");
  if (isEmpty(center_x) || isEmpty(center_y)) {
    if (reductionManager->existsProperty("LatestBeamCenterX") &&
        reductionManager->existsProperty("LatestBeamCenterY")) {
      center_x = reductionManager->getProperty("LatestBeamCenterX");
      center_y = reductionManager->getProperty("LatestBeamCenterY");
      m_output_message += "   |Setting beam center to [" +
                          Poco::NumberFormatter::format(center_x, 1) + ", " +
                          Poco::NumberFormatter::format(center_y, 1) + "]\n";
    } else
      m_output_message += "   |No beam center provided: skipping!\n";
  }

  const std::string rawFloodWSName = "__flood_data_" + path.getBaseName();
  MatrixWorkspace_sptr rawFloodWS;
  if (!reductionManager->existsProperty("LoadAlgorithm")) {
    IAlgorithm_sptr loadAlg = createChildAlgorithm("Load", 0.1, 0.3);
    loadAlg->setProperty("Filename", fileName);
    if (!isEmpty(center_x) && loadAlg->existsProperty("BeamCenterX"))
      loadAlg->setProperty("BeamCenterX", center_x);
    if (!isEmpty(center_y) && loadAlg->existsProperty("BeamCenterY"))
      loadAlg->setProperty("BeamCenterY", center_y);
    loadAlg->setPropertyValue("OutputWorkspace", rawFloodWSName);
    loadAlg->setChild(true);
    loadAlg->execute();
    Workspace_sptr tmpWS = loadAlg->getProperty("OutputWorkspace");
    rawFloodWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tmpWS);
    m_output_message += "   | Loaded " + fileName + " (Load algorithm)\n";
  } else {
    // Get load algorithm as a string so that we can create a completely
    // new proxy and ensure that we don't overwrite existing properties
    IAlgorithm_sptr loadAlg0 = reductionManager->getProperty("LoadAlgorithm");
    const std::string loadString = loadAlg0->toString();
    IAlgorithm_sptr loadAlg = Algorithm::fromString(loadString);
    loadAlg->setChild(true);
    loadAlg->setProperty("Filename", fileName);
    loadAlg->setPropertyValue("OutputWorkspace", rawFloodWSName);
    if (!isEmpty(center_x) && loadAlg->existsProperty("BeamCenterX"))
      loadAlg->setProperty("BeamCenterX", center_x);
    if (!isEmpty(center_y) && loadAlg->existsProperty("BeamCenterY"))
      loadAlg->setProperty("BeamCenterY", center_y);
    loadAlg->execute();
    rawFloodWS = loadAlg->getProperty("OutputWorkspace");
    m_output_message += "   |Loaded " + fileName + "\n";
    if (loadAlg->existsProperty("OutputMessage")) {
      std::string msg = loadAlg->getPropertyValue("OutputMessage");
      m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
    }
  }
  return rawFloodWS;
}

void SANSSensitivityCorrection::exec() {
  // Output log
  m_output_message = "";

  Progress progress(this, 0.0, 1.0, 10);

  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName,
                                                        reductionManager);
  }

  if (!reductionManager->existsProperty("SensitivityAlgorithm")) {
    auto algProp = make_unique<AlgorithmProperty>("SensitivityAlgorithm");
    algProp->setValue(toString());
    reductionManager->declareProperty(std::move(algProp));
  }

  progress.report("Loading sensitivity file");
  const std::string fileName = getPropertyValue("Filename");

  // Look for an entry for the dark current in the reduction table
  Poco::Path path(fileName);
  const std::string entryName = "Sensitivity" + path.getBaseName();
  MatrixWorkspace_sptr floodWS;
  std::string floodWSName = "__sensitivity_" + path.getBaseName();

  if (reductionManager->existsProperty(entryName)) {
    std::string wsName = reductionManager->getPropertyValue(entryName);
    floodWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    m_output_message += "   |Using " + wsName + "\n";
    g_log.debug()
        << "SANSSensitivityCorrection :: Using sensitivity workspace: "
        << wsName << "\n";
  } else {
    // Load the flood field if we don't have it already
    // First, try to determine whether we need to load data or a sensitivity
    // workspace...
    if (!floodWS && fileCheck(fileName)) {
      g_log.debug() << "SANSSensitivityCorrection :: Loading sensitivity file: "
                    << fileName << "\n";
      IAlgorithm_sptr loadAlg = createChildAlgorithm("Load", 0.1, 0.3);
      loadAlg->setProperty("Filename", fileName);
      loadAlg->executeAsChildAlg();
      Workspace_sptr floodWS_ws = loadAlg->getProperty("OutputWorkspace");
      floodWS = boost::dynamic_pointer_cast<MatrixWorkspace>(floodWS_ws);

      // Check that it's really a sensitivity file
      if (!floodWS->run().hasProperty("is_sensitivity")) {
        // Reset pointer
        floodWS.reset();
        g_log.error() << "A processed Mantid workspace was loaded but it "
                         "wasn't a sensitivity file!\n";
      }
    }

    // ... if we don't, just load the data and process it
    if (!floodWS) {
      // load the flood data
      MatrixWorkspace_sptr rawFloodWS = load(reductionManager, fileName, path);
      // Check whether we just loaded a flood field data set, or the actual
      // sensitivity
      if (!rawFloodWS->run().hasProperty("is_sensitivity")) {
        // process the flood data
        process(rawFloodWS, reductionManagerName, reductionManager);

        // load and process background, if requested
        const std::string bckFileName = getPropertyValue("BackgroundFiles");
        if (!bckFileName.empty()) {
          MatrixWorkspace_sptr rawBckWS =
              load(reductionManager, bckFileName, Poco::Path(bckFileName));
          process(rawBckWS, reductionManagerName, reductionManager, "Bck");
          Algorithm_sptr rebinAlg = createChildAlgorithm("RebinToWorkspace");
          rebinAlg->setProperty("WorkspaceToRebin", rawBckWS);
          rebinAlg->setProperty("WorkspaceToMatch", rawFloodWS);
          rebinAlg->setProperty("OutputWorkspace", "__unused");
          rebinAlg->execute();
          rawBckWS = rebinAlg->getProperty("OutputWorkspace");
          Algorithm_sptr minusAlg = createChildAlgorithm("Minus");
          minusAlg->setProperty("LHSWorkspace", rawFloodWS);
          minusAlg->setProperty("RHSWorkspace", rawBckWS);
          minusAlg->setProperty("OutputWorkspace", "__unused");
          minusAlg->execute();
          rawFloodWS = minusAlg->getProperty("OutputWorkspace");
        }

        // Calculate detector sensitivity
        IAlgorithm_sptr effAlg = createChildAlgorithm("CalculateEfficiency");
        effAlg->setProperty("InputWorkspace", rawFloodWS);

        const double minEff = getProperty("MinEfficiency");
        const double maxEff = getProperty("MaxEfficiency");
        const std::string maskFullComponent =
            getPropertyValue("MaskedFullComponent");
        const std::string maskEdges = getPropertyValue("MaskedEdges");
        const std::string maskComponent = getPropertyValue("MaskedComponent");

        effAlg->setProperty("MinEfficiency", minEff);
        effAlg->setProperty("MaxEfficiency", maxEff);
        effAlg->setProperty("MaskedFullComponent", maskFullComponent);
        effAlg->setProperty("MaskedEdges", maskEdges);
        effAlg->setProperty("MaskedComponent", maskComponent);
        effAlg->execute();
        floodWS = effAlg->getProperty("OutputWorkspace");
      } else {
        floodWS = rawFloodWS;
      }
      // Patch as needed
      if (reductionManager->existsProperty("SensitivityPatchAlgorithm")) {
        IAlgorithm_sptr patchAlg =
            reductionManager->getProperty("SensitivityPatchAlgorithm");
        patchAlg->setChild(true);
        patchAlg->setProperty("Workspace", floodWS);
        patchAlg->execute();
        m_output_message += "   |Sensitivity patch applied\n";
      }

      floodWS->mutableRun().addProperty("is_sensitivity", 1, "", true);
    }
    std::string floodWSOutputName =
        getPropertyValue("OutputSensitivityWorkspace");
    if (floodWSOutputName.empty()) {
      setPropertyValue("OutputSensitivityWorkspace", floodWSName);
      AnalysisDataService::Instance().addOrReplace(floodWSName, floodWS);
      reductionManager->declareProperty(
          Kernel::make_unique<WorkspaceProperty<>>(entryName, floodWSName,
                                                   Direction::InOut));
      reductionManager->setPropertyValue(entryName, floodWSName);
      reductionManager->setProperty(entryName, floodWS);
    }
    setProperty("OutputSensitivityWorkspace", floodWS);
  }

  progress.report(3, "Loaded flood field");

  // Check whether we need to apply the correction to a workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (inputWS) {
    // Divide sample data by detector efficiency
    IAlgorithm_sptr divideAlg = createChildAlgorithm("Divide", 0.6, 0.7);
    divideAlg->setProperty("LHSWorkspace", inputWS);
    divideAlg->setProperty("RHSWorkspace", floodWS);
    divideAlg->executeAsChildAlg();
    MatrixWorkspace_sptr outputWS = divideAlg->getProperty("OutputWorkspace");

    // Copy over the efficiency's masked pixels to the reduced workspace
    IAlgorithm_sptr maskAlg = createChildAlgorithm("MaskDetectors", 0.75, 0.85);
    maskAlg->setProperty("Workspace", outputWS);
    maskAlg->setProperty("MaskedWorkspace", floodWS);
    maskAlg->executeAsChildAlg();

    setProperty("OutputWorkspace", outputWS);
  }
  setProperty("OutputMessage",
              "Sensitivity correction computed\n" + m_output_message);

  progress.report("Performed sensitivity correction");
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
