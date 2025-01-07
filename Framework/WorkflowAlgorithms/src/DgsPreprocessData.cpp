// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/DgsPreprocessData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"
#include "Poco/Path.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace WorkflowAlgorithmHelpers;

namespace Mantid::WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DgsPreprocessData)

/// Algorithm's name for identification. @see Algorithm::name
const std::string DgsPreprocessData::name() const { return "DgsPreprocessData"; }

/// Algorithm's version for identification. @see Algorithm::version
int DgsPreprocessData::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DgsPreprocessData::category() const { return "Workflow\\Inelastic\\UsesPropertyManager"; }

/** Initialize the algorithm's properties.
 */
void DgsPreprocessData::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                        "An input workspace.");
  this->declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputMonitorWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A monitor workspace associated with the input workspace.");
  this->declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                        "The name for the output workspace.");
  this->declareProperty("TofRangeOffset", 0.0, "An addition to the TOF axis for monitor integration.");
  this->declareProperty("ReductionProperties", "__dgs_reduction_properties", Direction::Input);
}

/** Execute the algorithm.
 */
void DgsPreprocessData::exec() {
  g_log.notice() << "Starting DgsPreprocessData\n";
  // Get the reduction property manager
  const std::string reductionManagerName = this->getProperty("ReductionProperties");
  std::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    throw std::runtime_error("DgsPreprocessData cannot run without a reduction PropertyManager.");
  }

  // Log name that will indicate if the preprocessing has been done.
  const std::string doneLog = "DirectInelasticReductionNormalisedBy";

  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");

  std::string incidentBeamNorm = reductionManager->getProperty("IncidentBeamNormalisation");
  g_log.notice() << "Incident beam norm method = " << incidentBeamNorm << '\n';

  // Check to see if preprocessing has already been done.
  bool normAlreadyDone = inputWS->run().hasProperty(doneLog);

  if ("None" != incidentBeamNorm && !normAlreadyDone) {
    const std::string facility = ConfigService::Instance().getFacility().name();
    // SNS hard-wired to current normalisation
    if ("SNS" == facility) {
      incidentBeamNorm = "ByCurrent";
    }
    const std::string normAlg = "Normalise" + incidentBeamNorm;
    auto norm = createChildAlgorithm(normAlg);
    norm->setProperty("InputWorkspace", inputWS);
    norm->setProperty("OutputWorkspace", outputWS);
    if ("ToMonitor" == incidentBeamNorm) {
      // Perform extra setup for monitor normalisation
      double rangeOffset = this->getProperty("TofRangeOffset");
      double rangeMin = getDblPropOrParam("MonitorIntRangeLow", reductionManager, "norm-mon1-min", inputWS);
      rangeMin += rangeOffset;

      double rangeMax = getDblPropOrParam("MonitorIntRangeHigh", reductionManager, "norm-mon1-max", inputWS);
      rangeMax += rangeOffset;

      specnum_t monSpec = static_cast<specnum_t>(inputWS->getInstrument()->getNumberParameter("norm-mon1-spec")[0]);
      if ("ISIS" == facility) {
        norm->setProperty("MonitorSpectrum", monSpec);
      }
      // Do SNS
      else {
        MatrixWorkspace_const_sptr monitorWS = this->getProperty("MonitorWorkspace");
        if (!monitorWS) {
          throw std::runtime_error("SNS instruments require monitor workspaces "
                                   "for monitor normalisation.");
        }
        std::size_t monIndex = monitorWS->getIndexFromSpectrumNumber(monSpec);
        norm->setProperty("MonitorWorkspace", monitorWS);
        norm->setProperty("MonitorWorkspaceIndex", monIndex);
      }
      norm->setProperty("IntegrationRangeMin", rangeMin);
      norm->setProperty("IntegrationRangeMax", rangeMax);
      norm->setProperty("IncludePartialBins", true);
    }
    norm->executeAsChildAlg();

    outputWS = norm->getProperty("OutputWorkspace");

    auto addLog = createChildAlgorithm("AddSampleLog");
    addLog->setProperty("Workspace", outputWS);
    addLog->setProperty("LogName", doneLog);
    addLog->setProperty("LogText", normAlg);
    addLog->executeAsChildAlg();
  } else {
    if (normAlreadyDone) {
      g_log.information() << "Preprocessing already done on " << inputWS->getName() << '\n';
    }
    outputWS = inputWS;
  }

  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::WorkflowAlgorithms
