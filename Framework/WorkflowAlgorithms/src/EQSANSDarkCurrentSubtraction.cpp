// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/EQSANSDarkCurrentSubtraction.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "Poco/Path.h"
#include "Poco/String.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSDarkCurrentSubtraction)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void EQSANSDarkCurrentSubtraction::init() {
  auto wsValidator = boost::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "", Direction::Input, wsValidator));

  declareProperty(
      std::make_unique<API::FileProperty>(
          "Filename", "", API::FileProperty::Load, "_event.nxs"),
      "The name of the input event Nexus file to load as dark current.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output));
  declareProperty("PersistentCorrection", true,
                  "If true, the algorithm will be persistent and re-used when "
                  "other data sets are processed");
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "OutputDarkCurrentWorkspace", "", Direction::Output,
      PropertyMode::Optional));
  declareProperty("OutputMessage", "", Direction::Output);
}

void EQSANSDarkCurrentSubtraction::exec() {
  std::string output_message;
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
    auto algProp = std::make_unique<AlgorithmProperty>("DarkCurrentAlgorithm");
    algProp->setValue(toString());
    reductionManager->declareProperty(std::move(algProp));
  }

  Progress progress(this, 0.0, 1.0, 10);

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  const std::string fileName = getPropertyValue("Filename");
  MatrixWorkspace_sptr darkWS;

  progress.report("Subtracting dark current");

  // Look for an entry for the dark current in the reduction table
  Poco::Path path(fileName);
  const std::string entryName = "DarkCurrent" + path.getBaseName();
  std::string darkWSName = "__dark_current_" + path.getBaseName();

  if (reductionManager->existsProperty(entryName)) {
    darkWS = reductionManager->getProperty(entryName);
    darkWSName = reductionManager->getPropertyValue(entryName);
    output_message += darkWSName + '\n';
  } else {
    // Load the dark current if we don't have it already
    IAlgorithm_sptr loadAlg;
    if (!reductionManager->existsProperty("LoadAlgorithm")) {
      loadAlg = createChildAlgorithm("EQSANSLoad", 0.1, 0.3);
      loadAlg->setProperty("Filename", fileName);
      if (loadAlg->existsProperty("LoadMonitors"))
        loadAlg->setProperty("LoadMonitors", false);
      loadAlg->executeAsChildAlg();
      darkWS = loadAlg->getProperty("OutputWorkspace");
    } else {
      // Get load algorithm as a string so that we can create a completely
      // new proxy and ensure that we don't overwrite existing properties
      IAlgorithm_sptr loadAlg0 = reductionManager->getProperty("LoadAlgorithm");
      const std::string loadString = loadAlg0->toString();
      loadAlg = Algorithm::fromString(loadString);
      loadAlg->setChild(true);
      loadAlg->setProperty("Filename", fileName);
      if (loadAlg->existsProperty("LoadMonitors"))
        loadAlg->setProperty("LoadMonitors", false);
      loadAlg->setPropertyValue("OutputWorkspace", darkWSName);
      loadAlg->execute();
      darkWS = loadAlg->getProperty("OutputWorkspace");
    }

    output_message += "\n   Loaded " + fileName + "\n";
    if (loadAlg->existsProperty("OutputMessage")) {
      std::string msg = loadAlg->getPropertyValue("OutputMessage");
      output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
    }

    std::string darkWSOutputName =
        getPropertyValue("OutputDarkCurrentWorkspace");
    if (!darkWSOutputName.empty())
      setProperty("OutputDarkCurrentWorkspace", darkWS);
    AnalysisDataService::Instance().addOrReplace(darkWSName, darkWS);
    reductionManager->declareProperty(std::make_unique<WorkspaceProperty<>>(
        entryName, "", Direction::Output));
    reductionManager->setPropertyValue(entryName, darkWSName);
    reductionManager->setProperty(entryName, darkWS);
  }
  progress.report(3, "Loaded dark current");

  // Normalize the dark current and data to counting time
  double scaling_factor = 1.0;
  if (inputWS->run().hasProperty("proton_charge")) {
    auto dp = inputWS->run().getTimeSeriesProperty<double>("proton_charge");
    double duration = dp->getStatistics().duration;

    dp = darkWS->run().getTimeSeriesProperty<double>("proton_charge");
    double dark_duration = dp->getStatistics().duration;
    scaling_factor = duration / dark_duration;
  } else if (inputWS->run().hasProperty("timer")) {
    double duration = inputWS->run().getPropertyValueAsType<double>("timer");
    double dark_duration =
        darkWS->run().getPropertyValueAsType<double>("timer");
    ;
    scaling_factor = duration / dark_duration;
  } else {
    output_message +=
        "\n   Could not find proton charge or duration in sample logs";
    g_log.error()
        << "ERROR: Could not find proton charge or duration in sample logs\n";
  };

  progress.report("Scaling dark current");

  // Scale the stored dark current by the counting time
  IAlgorithm_sptr rebinAlg = createChildAlgorithm("RebinToWorkspace", 0.4, 0.5);
  rebinAlg->setProperty("WorkspaceToRebin", darkWS);
  rebinAlg->setProperty("WorkspaceToMatch", inputWS);
  rebinAlg->setProperty("OutputWorkspace", darkWS);
  rebinAlg->executeAsChildAlg();
  MatrixWorkspace_sptr scaledDarkWS = rebinAlg->getProperty("OutputWorkspace");

  // Perform subtraction
  IAlgorithm_sptr scaleAlg = createChildAlgorithm("Scale", 0.5, 0.6);
  scaleAlg->setProperty("InputWorkspace", scaledDarkWS);
  scaleAlg->setProperty("Factor", scaling_factor);
  scaleAlg->setProperty("OutputWorkspace", scaledDarkWS);
  scaleAlg->setProperty("Operation", "Multiply");
  scaleAlg->executeAsChildAlg();
  scaledDarkWS = rebinAlg->getProperty("OutputWorkspace");

  IAlgorithm_sptr minusAlg = createChildAlgorithm("Minus", 0.6, 0.7);
  minusAlg->setProperty("LHSWorkspace", inputWS);
  minusAlg->setProperty("RHSWorkspace", scaledDarkWS);
  const std::string outputWSname = getPropertyValue("OutputWorkspace");
  minusAlg->setPropertyValue("OutputWorkspace", outputWSname);
  minusAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = minusAlg->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", outputWS);
  setProperty("OutputMessage", "Dark current subtracted: " + output_message);

  progress.report("Subtracted dark current");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
