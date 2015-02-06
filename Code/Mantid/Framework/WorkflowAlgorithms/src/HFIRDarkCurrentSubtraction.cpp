//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/HFIRDarkCurrentSubtraction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FileProperty.h"
#include "Poco/Path.h"
#include "Poco/String.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HFIRDarkCurrentSubtraction)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void HFIRDarkCurrentSubtraction::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator));

  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, ".xml"),
      "The name of the input event Nexus file to load as dark current.");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
  declareProperty("PersistentCorrection", true,
                  "If true, the algorithm will be persistent and re-used when "
                  "other data sets are processed");
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
      "OutputDarkCurrentWorkspace", "", Direction::Output,
      PropertyMode::Optional));
  declareProperty("OutputMessage", "", Direction::Output);
}

void HFIRDarkCurrentSubtraction::exec() {
  std::string output_message = "";
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

  // If the load algorithm isn't in the reduction properties, add it
  const bool persistent = getProperty("PersistentCorrection");
  if (!reductionManager->existsProperty("DarkCurrentAlgorithm") && persistent) {
    AlgorithmProperty *algProp = new AlgorithmProperty("DarkCurrentAlgorithm");
    algProp->setValue(toString());
    reductionManager->declareProperty(algProp);
  }

  Progress progress(this, 0.0, 1.0, 10);

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string fileName = getPropertyValue("Filename");
  MatrixWorkspace_sptr darkWS;
  std::string darkWSName = getPropertyValue("OutputDarkCurrentWorkspace");

  progress.report("Subtracting dark current");

  // Look for an entry for the dark current in the reduction table
  Poco::Path path(fileName);
  const std::string entryName = "DarkCurrent" + path.getBaseName();

  if (reductionManager->existsProperty(entryName)) {
    darkWS = reductionManager->getProperty(entryName);
    darkWSName = reductionManager->getPropertyValue(entryName);
    output_message += darkWSName + '\n';
  } else {
    // Load the dark current if we don't have it already
    if (darkWSName.size() == 0) {
      darkWSName = "__dark_current_" + path.getBaseName();
      setPropertyValue("OutputDarkCurrentWorkspace", darkWSName);
    }

    IAlgorithm_sptr loadAlg;
    if (!reductionManager->existsProperty("LoadAlgorithm")) {
      loadAlg = createChildAlgorithm("HFIRLoad", 0.1, 0.3);
      loadAlg->setProperty("Filename", fileName);
      loadAlg->setProperty("ReductionProperties", reductionManagerName);
      loadAlg->executeAsChildAlg();
    } else {
      IAlgorithm_sptr loadAlg0 = reductionManager->getProperty("LoadAlgorithm");
      const std::string loadString = loadAlg0->toString();
      loadAlg = Algorithm::fromString(loadString);
      loadAlg->setChild(true);
      loadAlg->setProperty("Filename", fileName);
      loadAlg->setProperty("ReductionProperties", reductionManagerName);
      loadAlg->setPropertyValue("OutputWorkspace", darkWSName);
      loadAlg->execute();
    }
    darkWS = loadAlg->getProperty("OutputWorkspace");
    output_message += "\n   Loaded " + fileName + "\n";
    if (loadAlg->existsProperty("OutputMessage")) {
      std::string msg = loadAlg->getPropertyValue("OutputMessage");
      output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
    }

    setProperty("OutputDarkCurrentWorkspace", darkWS);
    reductionManager->declareProperty(
        new WorkspaceProperty<>(entryName, "", Direction::Output));
    reductionManager->setPropertyValue(entryName, darkWSName);
    reductionManager->setProperty(entryName, darkWS);
  }
  progress.report(3, "Loaded dark current");

  // Perform subtraction
  double darkTimer = getCountingTime(darkWS);
  double dataTimer = getCountingTime(inputWS);
  IAlgorithm_sptr scaleAlg = createChildAlgorithm("Scale", 0.3, 0.5);
  scaleAlg->setProperty("InputWorkspace", darkWS);
  scaleAlg->setProperty("Factor", dataTimer / darkTimer);
  scaleAlg->setProperty("Operation", "Multiply");
  scaleAlg->executeAsChildAlg();
  MatrixWorkspace_sptr scaledDarkWS = scaleAlg->getProperty("OutputWorkspace");

  // Zero out timer and monitor so that we don't subtract them out
  for (size_t i = 0; i < scaledDarkWS->dataY(0).size(); i++) {
    scaledDarkWS->dataY(DEFAULT_TIMER_ID)[i] = 0.0;
    scaledDarkWS->dataE(DEFAULT_TIMER_ID)[i] = 0.0;
    scaledDarkWS->dataY(DEFAULT_MONITOR_ID)[i] = 0.0;
    scaledDarkWS->dataE(DEFAULT_MONITOR_ID)[i] = 0.0;
  }
  IAlgorithm_sptr minusAlg = createChildAlgorithm("Minus", 0.5, 0.7);
  minusAlg->setProperty("LHSWorkspace", inputWS);
  minusAlg->setProperty("RHSWorkspace", scaledDarkWS);
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  minusAlg->setProperty("OutputWorkspace", outputWS);
  minusAlg->executeAsChildAlg();
  MatrixWorkspace_sptr correctedWS = minusAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", correctedWS);
  setProperty("OutputMessage", "Dark current subtracted: " + output_message);

  progress.report("Subtracted dark current");
}

/// Get the counting time from a workspace
/// @param inputWS :: workspace to read the counting time from
double
HFIRDarkCurrentSubtraction::getCountingTime(MatrixWorkspace_sptr inputWS) {
  // First, look whether we have the information in the log
  if (inputWS->run().hasProperty("timer")) {
    Mantid::Kernel::Property *prop = inputWS->run().getProperty("timer");
    Mantid::Kernel::PropertyWithValue<double> *dp =
        dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    return *dp;
  } else {
    // If we don't have the information in the log, use the default timer
    // spectrum
    MantidVec &timer = inputWS->dataY(DEFAULT_TIMER_ID);
    return timer[0];
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
