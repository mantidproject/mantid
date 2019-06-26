// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/EQSANSDarkCurrentSubtraction2.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "Poco/Path.h"
#include "Poco/String.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSDarkCurrentSubtraction2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void EQSANSDarkCurrentSubtraction2::init() {
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

void EQSANSDarkCurrentSubtraction2::exec() {
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

  // This version of dark current subtraction only works on histograms.
  // Users need to either make sure the EQSANSLoad algorithm produces
  // histograms, or turn off the dark current subtraction.
  EventWorkspace_const_sptr inputEventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (inputEventWS) {
    g_log.error() << "To use this version of EQSANSDarkCurrentSubtraction, "
                  << "you need to make sure EQSANSLoad produces histograms. "
                  << "You can also turn the dark current subtraction off.\n";
    throw std::invalid_argument(
        "EQSANSDarkCurrentSubtraction-v2 only works on histograms.");
  }

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
  if (inputWS->run().hasProperty("duration")) {
    double duration = inputWS->run().getPropertyValueAsType<double>("duration");
    double dark_duration =
        darkWS->run().getPropertyValueAsType<double>("duration");
    ;
    scaling_factor = duration / dark_duration;
  } else if (inputWS->run().hasProperty("proton_charge")) {
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
  // The scaling factor should account for the TOF cuts on each side of a frame
  // The EQSANSLoad algorithm cuts the beginning and end of the TOF distribution
  // so we don't need to correct the scaling factor here. When using
  // LoadEventNexus
  // we have to scale by (t_frame-t_low_cut-t_high_cut)/t_frame.

  progress.report("Scaling dark current");

  // Get the dark current counts per pixel
  IAlgorithm_sptr rebinAlg = createChildAlgorithm("Integration", 0.4, 0.5);
  rebinAlg->setProperty("InputWorkspace", darkWS);
  rebinAlg->setProperty("OutputWorkspace", darkWS);
  rebinAlg->executeAsChildAlg();
  MatrixWorkspace_sptr scaledDarkWS = rebinAlg->getProperty("OutputWorkspace");

  // Scale the dark current
  IAlgorithm_sptr scaleAlg = createChildAlgorithm("Scale", 0.5, 0.6);
  scaleAlg->setProperty("InputWorkspace", scaledDarkWS);
  scaleAlg->setProperty("Factor", scaling_factor);
  scaleAlg->setProperty("OutputWorkspace", scaledDarkWS);
  scaleAlg->setProperty("Operation", "Multiply");
  scaleAlg->executeAsChildAlg();
  scaledDarkWS = rebinAlg->getProperty("OutputWorkspace");

  // Scale the dark counts to the bin width and perform subtraction
  const int numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());
  const int numberOfDarkSpectra =
      static_cast<int>(scaledDarkWS->getNumberHistograms());
  if (numberOfSpectra != numberOfDarkSpectra) {
    g_log.error() << "Incompatible number of pixels between sample run and "
                     "dark current\n";
  }
  const int nBins = static_cast<int>(inputWS->readY(0).size());
  const int xLength = static_cast<int>(inputWS->readX(0).size());
  if (xLength != nBins + 1) {
    g_log.error() << "The input workspaces are expected to be histograms\n";
  }

  progress.report("Subtracting dark current");
  const auto &spectrumInfo = inputWS->spectrumInfo();
  // Loop over all tubes and patch as necessary
  for (int i = 0; i < numberOfSpectra; i++) {
    // If this detector is a monitor, skip to the next one
    if (spectrumInfo.isMasked(i))
      continue;

    const MantidVec &YDarkValues = scaledDarkWS->readY(i);
    const MantidVec &YDarkErrors = scaledDarkWS->readE(i);
    const MantidVec &XValues = inputWS->readX(i);
    MantidVec &YValues = inputWS->dataY(i);
    MantidVec &YErrors = inputWS->dataE(i);
    for (int j = 0; j < nBins; j++) {
      double bin_scale =
          (XValues[j + 1] - XValues[j]) / (XValues[nBins] - XValues[0]);
      YValues[j] -= YDarkValues[0] * bin_scale;
      YErrors[j] =
          sqrt(YErrors[j] * YErrors[j] +
               YDarkErrors[0] * YDarkErrors[0] * bin_scale * bin_scale);
    }
  }
  setProperty("OutputWorkspace", inputWS);
  setProperty("OutputMessage", "Dark current subtracted: " + output_message);

  progress.report("Subtracted dark current");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
