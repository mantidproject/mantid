//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SANSBeamFluxCorrection.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "Poco/Path.h"

namespace Mantid {
namespace WorkflowAlgorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSBeamFluxCorrection)

void SANSBeamFluxCorrection::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Workspace to be corrected");
  declareProperty(
      new WorkspaceProperty<>("InputMonitorWorkspace", "", Direction::Input),
      "Workspace containing the monitor counts for the sample data");

  std::vector<std::string> exts;
  exts.push_back("_event.nxs");
  exts.push_back(".nxs");
  exts.push_back(".nxs.h5");
  declareProperty(new API::FileProperty("ReferenceFluxFilename", "",
                                        API::FileProperty::Load, exts),
                  "File containing the reference flux spectrum.");

  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Corrected workspace.");
  declareProperty("OutputMessage", "", Direction::Output);
}

void SANSBeamFluxCorrection::exec() {
  Progress progress(this, 0.0, 1.0, 10);
  progress.report("Setting up beam flux correction");

  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  m_reductionManager = getProcessProperties(reductionManagerName);

  // If the beam flux correction algorithm isn't in the reduction properties,
  // add it
  if (!m_reductionManager->existsProperty("BeamFluxAlgorithm")) {
    AlgorithmProperty *algProp = new AlgorithmProperty("BeamFluxAlgorithm");
    algProp->setValue(toString());
    m_reductionManager->declareProperty(algProp);
  }

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr monitorWS = getProperty("InputMonitorWorkspace");

  // Load reference
  progress.report("Loading reference data");
  MatrixWorkspace_sptr fluxRefWS = loadReference();

  // Rebin the reference and monitor data to the sample data workspace
  progress.report("Rebinning reference data");
  IAlgorithm_sptr convAlg = createChildAlgorithm("ConvertToHistogram");
  convAlg->setProperty("InputWorkspace", fluxRefWS);
  convAlg->executeAsChildAlg();
  fluxRefWS = convAlg->getProperty("OutputWorkspace");

  IAlgorithm_sptr rebinAlg = createChildAlgorithm("RebinToWorkspace");
  rebinAlg->setProperty("WorkspaceToRebin", fluxRefWS);
  rebinAlg->setProperty("WorkspaceToMatch", inputWS);
  rebinAlg->executeAsChildAlg();
  MatrixWorkspace_sptr scaledfluxRefWS =
      rebinAlg->getProperty("OutputWorkspace");

  progress.report("Rebinning monitor data");
  rebinAlg = createChildAlgorithm("RebinToWorkspace");
  rebinAlg->setProperty("WorkspaceToRebin", monitorWS);
  rebinAlg->setProperty("WorkspaceToMatch", inputWS);
  rebinAlg->executeAsChildAlg();
  monitorWS = rebinAlg->getProperty("OutputWorkspace");

  progress.report("Correcting input data");
  // I = I_0 / Phi_sample
  // Phi_sample = M_sample * [Phi_ref/M_ref]
  // where [Phi_ref/M_ref] is the fluxRefWS workspace
  IAlgorithm_sptr divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", inputWS);
  divideAlg->setProperty("RHSWorkspace", monitorWS);
  divideAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = divideAlg->getProperty("OutputWorkspace");

  divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", outputWS);
  divideAlg->setProperty("RHSWorkspace", scaledfluxRefWS);
  divideAlg->executeAsChildAlg();
  outputWS = divideAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWS);
  setProperty("OutputMessage", "Flux correction applied\n" + m_output_message);
}

/**
 * It's assumed that both the flux reference files are simple Nexus
 * files since they have to produced by hand by the instrument
 * scientists. A simple Load algorithm should suffice.
 */
MatrixWorkspace_sptr SANSBeamFluxCorrection::loadReference() {
  const std::string referenceFluxFile =
      getPropertyValue("ReferenceFluxFilename");
  Poco::Path path(referenceFluxFile);
  const std::string entryName = "SANSBeamFluxCorrection_" + path.getBaseName();
  std::string fluxRefWSName = "__beam_flux_reference_" + path.getBaseName();

  // Load reference flux as needed
  MatrixWorkspace_sptr fluxRefWS;
  if (m_reductionManager->existsProperty(entryName)) {
    fluxRefWS = m_reductionManager->getProperty(entryName);
    fluxRefWSName = m_reductionManager->getPropertyValue(entryName);
    m_output_message += "   | Using flux reference " + referenceFluxFile + "\n";
  } else {
    IAlgorithm_sptr loadAlg = createChildAlgorithm("Load");
    loadAlg->setProperty("Filename", referenceFluxFile);
    loadAlg->executeAsChildAlg();
    Workspace_sptr tmpWS = loadAlg->getProperty("OutputWorkspace");
    fluxRefWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tmpWS);
    m_output_message +=
        "   | Loaded flux reference " + referenceFluxFile + "\n";

    // Keep the reference data for later use
    AnalysisDataService::Instance().addOrReplace(fluxRefWSName, fluxRefWS);
    m_reductionManager->declareProperty(
        new WorkspaceProperty<>(entryName, fluxRefWSName, Direction::InOut));
    m_reductionManager->setPropertyValue(entryName, fluxRefWSName);
    m_reductionManager->setProperty(entryName, fluxRefWS);
  }

  return fluxRefWS;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
