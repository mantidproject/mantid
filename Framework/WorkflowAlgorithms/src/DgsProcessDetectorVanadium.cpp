// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/DgsProcessDetectorVanadium.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace WorkflowAlgorithmHelpers;

namespace Mantid::WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DgsProcessDetectorVanadium)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string DgsProcessDetectorVanadium::name() const { return "DgsProcessDetectorVanadium"; }

/// Algorithm's version for identification. @see Algorithm::version
int DgsProcessDetectorVanadium::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DgsProcessDetectorVanadium::category() const { return "Workflow\\Inelastic\\UsesPropertyManager"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DgsProcessDetectorVanadium::init() {
  // auto wsValidator = std::make_shared<CompositeValidator>();
  // wsValidator->add<WorkspaceUnitValidator>("TOF");
  this->declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                        "An input workspace containing the detector vanadium data in TOF units.");
  this->declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputMonitorWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A monitor workspace associated with the input workspace.");
  this->declareProperty(
      std::make_unique<WorkspaceProperty<>>("MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A mask workspace");
  this->declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                        "The name for the output workspace.");
  this->declareProperty("ReductionProperties", "__dgs_reduction_properties", Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DgsProcessDetectorVanadium::exec() {
  g_log.notice() << "Starting DgsProcessDetectorVanadium\n";
  // Get the reduction property manager
  const std::string reductionManagerName = this->getProperty("ReductionProperties");
  std::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    throw std::runtime_error("DgsProcessDetectorVanadium cannot run without a "
                             "reduction PropertyManager.");
  }

  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr monWS = this->getProperty("InputMonitorWorkspace");

  // Normalise result workspace to incident beam parameter
  auto norm = createChildAlgorithm("DgsPreprocessData");
  norm->setProperty("InputWorkspace", inputWS);
  norm->setProperty("OutputWorkspace", inputWS);
  norm->setProperty("InputMonitorWorkspace", monWS);
  norm->executeAsChildAlg();
  inputWS.reset();
  inputWS = norm->getProperty("OutputWorkspace");

  double detVanIntRangeLow = getDblPropOrParam("DetVanIntRangeLow", reductionManager, "wb-integr-min", inputWS);

  double detVanIntRangeHigh = getDblPropOrParam("DetVanIntRangeHigh", reductionManager, "wb-integr-max", inputWS);

  const std::string detVanIntRangeUnits = reductionManager->getProperty("DetVanIntRangeUnits");

  if ("TOF" != detVanIntRangeUnits) {
    // Convert the data to the appropriate units
    auto cnvun = createChildAlgorithm("ConvertUnits");
    cnvun->setProperty("InputWorkspace", inputWS);
    cnvun->setProperty("OutputWorkspace", inputWS);
    cnvun->setProperty("Target", detVanIntRangeUnits);
    cnvun->setProperty("EMode", "Elastic");
    cnvun->executeAsChildAlg();
    inputWS = cnvun->getProperty("OutputWorkspace");
  }

  // Rebin the data (not Integration !?!?!?)
  std::vector<double> binning{detVanIntRangeLow, detVanIntRangeHigh - detVanIntRangeLow, detVanIntRangeHigh};

  auto rebin = createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", inputWS);
  rebin->setProperty("OutputWorkspace", outputWS);
  rebin->setProperty("PreserveEvents", false);
  rebin->setProperty("Params", binning);
  rebin->executeAsChildAlg();
  outputWS = rebin->getProperty("OutputWorkspace");

  // Mask and group workspace if necessary.
  MatrixWorkspace_sptr maskWS = this->getProperty("MaskWorkspace");
  //!!! I see masks here but where is the map workspace used for vanadium
  // grouping (In ISIS)?
  auto remap = createChildAlgorithm("DgsRemap");
  remap->setProperty("InputWorkspace", outputWS);
  remap->setProperty("OutputWorkspace", outputWS);
  remap->setProperty("MaskWorkspace", maskWS);
  remap->executeAsChildAlg();
  outputWS = remap->getProperty("OutputWorkspace");

  const std::string facility = ConfigService::Instance().getFacility().name();
  if ("ISIS" == facility) {
    // Scale results by a constant
    double wbScaleFactor = inputWS->getInstrument()->getNumberParameter("wb-scale-factor")[0];
    outputWS *= wbScaleFactor;
  }

  if (reductionManager->existsProperty("SaveProcessedDetVan")) {
    bool saveProc = reductionManager->getProperty("SaveProcessedDetVan");
    if (saveProc) {
      std::string outputFile;
      if (reductionManager->existsProperty("SaveProcDetVanFilename")) {
        outputFile = reductionManager->getPropertyValue("SaveProcDetVanFilename");
      }
      if (outputFile.empty()) {
        outputFile = this->getPropertyValue("OutputWorkspace");
        outputFile += ".nxs";
      }

      // Don't save private calculation workspaces
      if (!outputFile.empty() && !outputFile.starts_with("ChildAlgOutput") && !outputFile.starts_with("__")) {
        auto save = createChildAlgorithm("SaveNexus");
        save->setProperty("InputWorkspace", outputWS);
        save->setProperty("FileName", outputFile);
        save->execute();
      }
    }
  }

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::WorkflowAlgorithms
