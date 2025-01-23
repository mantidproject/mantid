// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"
#include "Poco/File.h"
#include "Poco/NumberFormatter.h"
#include "Poco/Path.h"
#include "Poco/String.h"

namespace Mantid::WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSensitivityCorrection)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SANSSensitivityCorrection::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional));
  const std::vector<std::string> fileExts{"_event.nxs", ".xml"};
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, fileExts),
                  "Flood field or sensitivity file.");
  declareProperty("UseSampleDC", true,
                  "If true, the dark current subtracted "
                  "from the sample data will also be "
                  "subtracted from the flood field.");
  declareProperty(std::make_unique<API::FileProperty>("DarkCurrentFile", "", API::FileProperty::OptionalLoad, fileExts),
                  "The name of the input file to load as dark current.");

  auto positiveDouble = std::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
                  "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble,
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
  declareProperty("MaskedFullComponent", "", "Component Name to fully mask according to the IDF file.");
  declareProperty(std::make_unique<ArrayProperty<int>>("MaskedEdges"),
                  "Number of pixels to mask on the edges: X-low, X-high, Y-low, Y-high");
  declareProperty("MaskedComponent", "", "Component Name to mask the edges according to the IDF file.");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output, PropertyMode::Optional));
  declareProperty("ReductionProperties", "__sans_reduction_properties");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputSensitivityWorkspace", "",
                                                                       Direction::Output, PropertyMode::Optional));
  declareProperty("OutputMessage", "", Direction::Output);
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

  // open file and make sure it has entries
  try {
    ::NeXus::File handle(filePath, NXACC_READ);
    const auto entries = handle.getEntries();
    if (entries.size() == 0) {
      g_log.error("Error no entries found in " + filePath);
      return false;
    }
    // ensure the file has an NXentry type named "mantid_workspace_1"
    auto const iter = entries.find("mantid_workspace_1");
    if (iter == entries.cend())
      return false;
    else
      return iter->second == "NXentry";
  } catch (NeXus::Exception &) {
    throw Exception::FileError("Unable to read data in File:", filePath);
  }
}

void SANSSensitivityCorrection::exec() {
  // Output log
  m_output_message = "";

  Progress progress(this, 0.0, 1.0, 10);

  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  std::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    reductionManager = std::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);
  }

  if (!reductionManager->existsProperty("SensitivityAlgorithm")) {
    auto algProp = std::make_unique<AlgorithmProperty>("SensitivityAlgorithm");
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
    floodWS =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    m_output_message += "   |Using " + wsName + "\n";
    g_log.debug() << "SANSSensitivityCorrection :: Using sensitivity workspace: " << wsName << "\n";
  } else {
    // Load the flood field if we don't have it already
    // First, try to determine whether we need to load data or a sensitivity
    // workspace...
    if (fileCheck(fileName)) {
      g_log.debug() << "SANSSensitivityCorrection :: Loading sensitivity file: " << fileName << "\n";
      auto loadAlg = createChildAlgorithm("Load", 0.1, 0.3);
      loadAlg->setProperty("Filename", fileName);
      loadAlg->executeAsChildAlg();
      Workspace_sptr floodWS_ws = loadAlg->getProperty("OutputWorkspace");
      floodWS = std::dynamic_pointer_cast<MatrixWorkspace>(floodWS_ws);

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
      // Read in default beam center
      double center_x = getProperty("BeamCenterX");
      double center_y = getProperty("BeamCenterY");
      if (isEmpty(center_x) || isEmpty(center_y)) {
        if (reductionManager->existsProperty("LatestBeamCenterX") &&
            reductionManager->existsProperty("LatestBeamCenterY")) {
          center_x = reductionManager->getProperty("LatestBeamCenterX");
          center_y = reductionManager->getProperty("LatestBeamCenterY");
          m_output_message += "   |Setting beam center to [" + Poco::NumberFormatter::format(center_x, 1) + ", " +
                              Poco::NumberFormatter::format(center_y, 1) + "]\n";
        } else
          m_output_message += "   |No beam center provided: skipping!\n";
      }

      const std::string rawFloodWSName = "__flood_data_" + path.getBaseName();
      MatrixWorkspace_sptr rawFloodWS;
      if (!reductionManager->existsProperty("LoadAlgorithm")) {
        auto loadAlg = createChildAlgorithm("Load", 0.1, 0.3);
        loadAlg->setProperty("Filename", fileName);
        if (!isEmpty(center_x) && loadAlg->existsProperty("BeamCenterX"))
          loadAlg->setProperty("BeamCenterX", center_x);
        if (!isEmpty(center_y) && loadAlg->existsProperty("BeamCenterY"))
          loadAlg->setProperty("BeamCenterY", center_y);
        loadAlg->setPropertyValue("OutputWorkspace", rawFloodWSName);
        loadAlg->executeAsChildAlg();
        Workspace_sptr tmpWS = loadAlg->getProperty("OutputWorkspace");
        rawFloodWS = std::dynamic_pointer_cast<MatrixWorkspace>(tmpWS);
        m_output_message += "   | Loaded " + fileName + " (Load algorithm)\n";
      } else {
        // Get load algorithm as a string so that we can create a completely
        // new proxy and ensure that we don't overwrite existing properties
        IAlgorithm_sptr loadAlg0 = reductionManager->getProperty("LoadAlgorithm");
        const std::string loadString = loadAlg0->toString();
        auto loadAlg = Algorithm::fromString(loadString);
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

      // Check whether we just loaded a flood field data set, or the actual
      // sensitivity
      if (!rawFloodWS->run().hasProperty("is_sensitivity")) {
        const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");

        // Look for a dark current subtraction algorithm
        std::string dark_result;
        if (reductionManager->existsProperty("DarkCurrentAlgorithm")) {
          IAlgorithm_sptr darkAlg = reductionManager->getProperty("DarkCurrentAlgorithm");
          darkAlg->setChild(true);
          darkAlg->setProperty("InputWorkspace", rawFloodWS);
          darkAlg->setProperty("OutputWorkspace", rawFloodWS);

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
        } else if (!darkCurrentFile.empty()) {
          // We need to subtract the dark current for the flood field but no
          // dark
          // current subtraction was set for the sample! Use the default dark
          // current algorithm if we can find it.
          if (reductionManager->existsProperty("DefaultDarkCurrentAlgorithm")) {
            IAlgorithm_sptr darkAlg = reductionManager->getProperty("DefaultDarkCurrentAlgorithm");
            darkAlg->setChild(true);
            darkAlg->setProperty("InputWorkspace", rawFloodWS);
            darkAlg->setProperty("OutputWorkspace", rawFloodWS);
            darkAlg->setProperty("Filename", darkCurrentFile);
            darkAlg->setProperty("PersistentCorrection", false);
            darkAlg->execute();
            if (darkAlg->existsProperty("OutputMessage"))
              dark_result = darkAlg->getPropertyValue("OutputMessage");
          } else {
            // We are running out of options
            g_log.error() << "No dark current algorithm provided to load [" << getPropertyValue("DarkCurrentFile")
                          << "]: skipped!\n";
            dark_result = "   No dark current algorithm provided: skipped\n";
          }
        }
        m_output_message += "   |" + Poco::replace(dark_result, "\n", "\n   |") + "\n";

        // Look for solid angle correction algorithm
        if (reductionManager->existsProperty("SANSSolidAngleCorrection")) {
          IAlgorithm_sptr solidAlg = reductionManager->getProperty("SANSSolidAngleCorrection");
          solidAlg->setChild(true);
          solidAlg->setProperty("InputWorkspace", rawFloodWS);
          solidAlg->setProperty("OutputWorkspace", rawFloodWS);
          solidAlg->execute();
          std::string msg = "Solid angle correction applied\n";
          if (solidAlg->existsProperty("OutputMessage"))
            msg = solidAlg->getPropertyValue("OutputMessage");
          m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
        }

        // Apply transmission correction as needed
        double floodTransmissionValue = getProperty("FloodTransmissionValue");
        double floodTransmissionError = getProperty("FloodTransmissionError");

        if (!isEmpty(floodTransmissionValue)) {
          g_log.debug() << "SANSSensitivityCorrection :: Applying transmission "
                           "to flood field\n";
          auto transAlg = createChildAlgorithm("ApplyTransmissionCorrection");
          transAlg->setProperty("InputWorkspace", rawFloodWS);
          transAlg->setProperty("OutputWorkspace", rawFloodWS);
          transAlg->setProperty("TransmissionValue", floodTransmissionValue);
          transAlg->setProperty("TransmissionError", floodTransmissionError);
          transAlg->setProperty("ThetaDependent", true);
          transAlg->execute();
          rawFloodWS = transAlg->getProperty("OutputWorkspace");
          m_output_message += "   |Applied transmission to flood field\n";
        }

        // Calculate detector sensitivity
        auto effAlg = createChildAlgorithm("CalculateEfficiency", -1, -1, true, 1);
        effAlg->setProperty("InputWorkspace", rawFloodWS);

        const double minEff = getProperty("MinEfficiency");
        const double maxEff = getProperty("MaxEfficiency");
        const std::string maskFullComponent = getPropertyValue("MaskedFullComponent");
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
        IAlgorithm_sptr patchAlg = reductionManager->getProperty("SensitivityPatchAlgorithm");
        patchAlg->setChild(true);
        patchAlg->setProperty("Workspace", floodWS);
        patchAlg->execute();
        m_output_message += "   |Sensitivity patch applied\n";
      }

      floodWS->mutableRun().addProperty("is_sensitivity", 1, "", true);
    }
    std::string floodWSOutputName = getPropertyValue("OutputSensitivityWorkspace");
    if (floodWSOutputName.empty()) {
      setPropertyValue("OutputSensitivityWorkspace", floodWSName);
      AnalysisDataService::Instance().addOrReplace(floodWSName, floodWS);
      reductionManager->declareProperty(
          std::make_unique<WorkspaceProperty<>>(entryName, floodWSName, Direction::InOut));
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
    auto divideAlg = createChildAlgorithm("Divide", 0.6, 0.7);
    divideAlg->setProperty("LHSWorkspace", inputWS);
    divideAlg->setProperty("RHSWorkspace", floodWS);
    divideAlg->executeAsChildAlg();
    MatrixWorkspace_sptr outputWS = divideAlg->getProperty("OutputWorkspace");

    // Copy over the efficiency's masked pixels to the reduced workspace
    auto maskAlg = createChildAlgorithm("MaskDetectors", 0.75, 0.85);
    maskAlg->setProperty("Workspace", outputWS);
    maskAlg->setProperty("MaskedWorkspace", floodWS);
    maskAlg->executeAsChildAlg();

    setProperty("OutputWorkspace", outputWS);
  }
  setProperty("OutputMessage", "Sensitivity correction computed\n" + m_output_message);

  progress.report("Performed sensitivity correction");
}

} // namespace Mantid::WorkflowAlgorithms
