#include "MantidWorkflowAlgorithms/DgsAbsoluteUnitsReduction.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PhysicalConstants;
using namespace WorkflowAlgorithmHelpers;

namespace Mantid {
namespace WorkflowAlgorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DgsAbsoluteUnitsReduction)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DgsAbsoluteUnitsReduction::DgsAbsoluteUnitsReduction() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
DgsAbsoluteUnitsReduction::~DgsAbsoluteUnitsReduction() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string DgsAbsoluteUnitsReduction::name() const {
  return "DgsAbsoluteUnitsReduction";
};

/// Algorithm's version for identification. @see Algorithm::version
int DgsAbsoluteUnitsReduction::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string DgsAbsoluteUnitsReduction::category() const {
  return "Workflow\\Inelastic\\UsesPropertyManager";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DgsAbsoluteUnitsReduction::init() {
  this->declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The absolute units sample workspace.");
  this->declareProperty(new WorkspaceProperty<>("InputMonitorWorkspace", "",
                                                Direction::Input,
                                                PropertyMode::Optional),
                        "A monitor workspace associated with the absolute "
                        "units sample workspace");
  this->declareProperty(new WorkspaceProperty<>("DetectorVanadiumWorkspace", "",
                                                Direction::Input,
                                                PropertyMode::Optional),
                        "An absolute units detector vanadium workspace.");
  this->declareProperty(
      new WorkspaceProperty<>("DetectorVanadiumMonitorWorkspace", "",
                              Direction::Input, PropertyMode::Optional),
      "A monitor workspace associated with the absolute units detector "
      "vanadium workspace.");
  this->declareProperty(new WorkspaceProperty<>("MaskWorkspace", "",
                                                Direction::Input,
                                                PropertyMode::Optional),
                        "A masking workspace to apply to the data.");
  this->declareProperty(new WorkspaceProperty<>("GroupingWorkspace", "",
                                                Direction::Input,
                                                PropertyMode::Optional),
                        "A grouping workspace for the absolute units data.");
  this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
                        Direction::Input);
  this->declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The integrated absolute units workspace.");
  this->declareProperty(
      new WorkspaceProperty<>("OutputMaskWorkspace", "", Direction::Output),
      "The diagnostic mask from the absolute units workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DgsAbsoluteUnitsReduction::exec() {
  g_log.notice() << "Starting DgsAbsoluteUnitsReduction" << std::endl;
  // Get the reduction property manager
  const std::string reductionManagerName =
      this->getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    throw std::runtime_error("DgsAbsoluteUnitsReduction cannot run without a "
                             "reduction PropertyManager.");
  }

  // Gather all of the input workspaces
  MatrixWorkspace_sptr absSampleWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr absSampleMonWS =
      this->getProperty("InputMonitorWorkspace");
  MatrixWorkspace_sptr absDetVanWS =
      this->getProperty("DetectorVanadiumWorkspace");
  MatrixWorkspace_sptr absDetVanMonWS =
      this->getProperty("DetectorVanadiumMonitorWorkspace");
  MatrixWorkspace_sptr absGroupingWS = this->getProperty("GroupingWorkspace");
  MatrixWorkspace_sptr maskWS = this->getProperty("MaskWorkspace");

  // Process absolute units detector vanadium if necessary
  MatrixWorkspace_sptr absIdetVanWS;
  if (absDetVanWS) {
    IAlgorithm_sptr detVan =
        this->createChildAlgorithm("DgsProcessDetectorVanadium");
    detVan->setProperty("InputWorkspace", absDetVanWS);
    detVan->setProperty("InputMonitorWorkspace", absDetVanMonWS);
    detVan->setProperty("ReductionProperties", reductionManagerName);
    if (maskWS) {
      detVan->setProperty("MaskWorkspace", maskWS);
    }
    detVan->executeAsChildAlg();
    absIdetVanWS = detVan->getProperty("OutputWorkspace");
  } else {
    absIdetVanWS = absDetVanWS;
  }

  const std::string absWsName = absSampleWS->getName() + "_absunits";
  IAlgorithm_sptr etConv =
      this->createChildAlgorithm("DgsConvertToEnergyTransfer");
  etConv->setProperty("InputWorkspace", absSampleWS);
  etConv->setProperty("InputMonitorWorkspace", absSampleMonWS);
  etConv->setProperty("OutputWorkspace", absWsName);
  const double ei = reductionManager->getProperty("AbsUnitsIncidentEnergy");
  etConv->setProperty("IncidentEnergyGuess", ei);
  etConv->setProperty("IntegratedDetectorVanadium", absIdetVanWS);
  etConv->setProperty("ReductionProperties", reductionManagerName);
  if (maskWS) {
    etConv->setProperty("MaskWorkspace", maskWS);
  }
  if (absGroupingWS) {
    etConv->setProperty("GroupingWorkspace", absGroupingWS);
  }
  etConv->setProperty("AlternateGroupingTag", "AbsUnits");
  etConv->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = etConv->getProperty("OutputWorkspace");

  const double vanadiumMass = getDblPropOrParam(
      "VanadiumMass", reductionManager, "vanadium-mass", outputWS);

  // Get the vanadium mass from the Mantid physical constants
  Atom vanadium = getAtom("V");
  const double vanadiumRmm = vanadium.mass;

  outputWS /= (vanadiumMass / vanadiumRmm);

  // Set integration range for absolute units sample
  double eMin = getDblPropOrParam("AbsUnitsMinimumEnergy", reductionManager,
                                  "monovan-integr-min", outputWS);
  double eMax = getDblPropOrParam("AbsUnitsMaximumEnergy", reductionManager,
                                  "monovan-integr-max", outputWS);
  std::vector<double> params;
  params.push_back(eMin);
  params.push_back(eMax - eMin);
  params.push_back(eMax);

  IAlgorithm_sptr rebin = this->createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", outputWS);
  rebin->setProperty("OutputWorkspace", outputWS);
  rebin->setProperty("Params", params);
  rebin->executeAsChildAlg();
  outputWS = rebin->getProperty("OutputWorkspace");

  IAlgorithm_sptr cToMWs =
      this->createChildAlgorithm("ConvertToMatrixWorkspace");
  cToMWs->setProperty("InputWorkspace", outputWS);
  cToMWs->setProperty("OutputWorkspace", outputWS);
  outputWS = cToMWs->getProperty("OutputWorkspace");

  // Run diagnostics
  const double huge =
      getDblPropOrParam("HighCounts", reductionManager, "diag_huge", outputWS);
  const double tiny =
      getDblPropOrParam("LowCounts", reductionManager, "diag_tiny", outputWS);
  const double vanOutLo = getDblPropOrParam(
      "AbsUnitsLowOutlier", reductionManager, "monovan_lo_bound", outputWS);
  const double vanOutHi = getDblPropOrParam(
      "AbsUnitsHighOutlier", reductionManager, "monovan_hi_bound", outputWS);
  const double vanLo = getDblPropOrParam(
      "AbsUnitsMedianTestLow", reductionManager, "monovan_lo_frac", outputWS);
  const double vanHi = getDblPropOrParam(
      "AbsUnitsMedianTestHigh", reductionManager, "monovan_hi_frac", outputWS);
  const double vanSigma = getDblPropOrParam(
      "AbsUnitsErrorBarCriterion", reductionManager, "diag_samp_sig", outputWS);

  IAlgorithm_sptr diag = this->createChildAlgorithm("DetectorDiagnostic");
  diag->setProperty("InputWorkspace", outputWS);
  diag->setProperty("OutputWorkspace", "absUnitsDiagMask");
  diag->setProperty("LowThreshold", tiny);
  diag->setProperty("HighThreshold", huge);
  diag->setProperty("LowOutlier", vanOutLo);
  diag->setProperty("HighOutlier", vanOutHi);
  diag->setProperty("LowThresholdFraction", vanLo);
  diag->setProperty("HighThresholdFraction", vanHi);
  diag->setProperty("SignificanceTest", vanSigma);
  diag->executeAsChildAlg();
  MatrixWorkspace_sptr absMaskWS = diag->getProperty("OutputWorkspace");

  IAlgorithm_sptr mask = this->createChildAlgorithm("MaskDetectors");
  mask->setProperty("Workspace", outputWS);
  mask->setProperty("MaskedWorkspace", absMaskWS);
  mask->executeAsChildAlg();
  outputWS = mask->getProperty("Workspace");

  IAlgorithm_sptr cFrmDist =
      this->createChildAlgorithm("ConvertFromDistribution");
  cFrmDist->setProperty("Workspace", outputWS);
  cFrmDist->executeAsChildAlg();
  outputWS = cFrmDist->getProperty("Workspace");

  IAlgorithm_sptr wMean = this->createChildAlgorithm("WeightedMeanOfWorkspace");
  wMean->setProperty("InputWorkspace", outputWS);
  wMean->setProperty("OutputWorkspace", outputWS);
  wMean->executeAsChildAlg();
  outputWS = wMean->getProperty("OutputWorkspace");

  // If the absolute units detector vanadium is used, do extra correction.
  if (absIdetVanWS) {
    NeutronAtom neutronVanadium = getNeutronAtom(vanadium.z_number);
    double xsection =
        (neutronVanadium.inc_scatt_xs + neutronVanadium.coh_scatt_xs) * 1e3 /
        4. / M_PI; // cross section per steradian in millibarns

    outputWS /= xsection;
    const double sampleMass = reductionManager->getProperty("SampleMass");
    const double sampleRmm = reductionManager->getProperty("SampleRmm");
    outputWS *= (sampleMass / sampleRmm);
  }

  this->setProperty("OutputMaskWorkspace", absMaskWS);
  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
