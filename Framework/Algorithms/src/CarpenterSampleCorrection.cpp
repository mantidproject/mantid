#include "MantidAlgorithms/CarpenterSampleCorrection.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CompositeValidator.h"

#include <stdexcept>

namespace Mantid {
namespace Algorithms {
DECLARE_ALGORITHM(CarpenterSampleCorrection) // Register the class
                                             // into the algorithm
                                             // factory

using namespace Kernel;
using namespace API;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using std::vector;
using namespace Geometry;

const std::string CarpenterSampleCorrection::name() const {
  return "CarpenterSampleCorrection";
}

int CarpenterSampleCorrection::version() const { return 1; }

const std::string CarpenterSampleCorrection::category() const {
  return "CorrectionFunctions\\AbsorptionCorrections";
}

/**
 * Initialize the properties to default values
 */
void CarpenterSampleCorrection::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  auto algCalcCarpenter = AlgorithmManager::Instance().createUnmanaged(
      "CalculateCarpenterSampleCorrection");
  algCalcCarpenter->initialize();

  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.");
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");

  copyProperty(algCalcCarpenter, "AttenuationXSection");
  copyProperty(algCalcCarpenter, "ScatteringXSection");
  copyProperty(algCalcCarpenter, "SampleNumberDensity");
  copyProperty(algCalcCarpenter, "CylinderSampleRadius");
}

/**
 * Execute the algorithm
 */
void CarpenterSampleCorrection::exec() {
  // common information
  MatrixWorkspace_sptr inputWksp = getProperty("InputWorkspace");
  double radius = getProperty("CylinderSampleRadius");
  double coeff1 = getProperty("AttenuationXSection");
  double coeff2 = getProperty("SampleNumberDensity");
  double coeff3 = getProperty("ScatteringXSection");

  // Calculate the absorption and multiple scattering corrections
  WorkspaceGroup_sptr calcOutput = calculateCorrection(
      inputWksp, radius, coeff1, coeff2, coeff3, true, true);
  Workspace_sptr absPtr = calcOutput->getItem(0);
  Workspace_sptr msPtr = calcOutput->getItem(1);
  auto absWksp = boost::dynamic_pointer_cast<MatrixWorkspace>(absPtr);
  auto msWksp = boost::dynamic_pointer_cast<MatrixWorkspace>(msPtr);

  // Clone input -> to apply corrections
  MatrixWorkspace_sptr outputWksp = getProperty("OutputWorkspace");

  EventWorkspace_sptr inputWkspEvent =
      boost::dynamic_pointer_cast<EventWorkspace>(inputWksp);

  outputWksp = inputWksp->clone();

  // Apply the absorption correction to the sample workspace
  outputWksp = divide(outputWksp, absWksp);

  // Apply the multiple scattering correction to the sample workspace
  auto factorWksp = multiply(inputWksp, msWksp);
  outputWksp = minus(outputWksp, factorWksp);

  // Output workspace
  if (inputWkspEvent) {
    auto outputWkspEvent =
        boost::dynamic_pointer_cast<EventWorkspace>(outputWksp);
    setProperty("OutputWorkspace", outputWkspEvent);
  }
  setProperty("OutputWorkspace", outputWksp);
}

WorkspaceGroup_sptr CarpenterSampleCorrection::calculateCorrection(
    MatrixWorkspace_sptr &inputWksp, double radius, double coeff1,
    double coeff2, double coeff3, bool doAbs, bool doMS) {
  auto calculate = this->createChildAlgorithm(
      "CalculateCarpenterSampleCorrection", 0.0, 0.25);
  calculate->setProperty("InputWorkspace", inputWksp);
  calculate->setProperty("CylinderSampleRadius", radius);
  calculate->setProperty("AttenuationXSection", coeff1);
  calculate->setProperty("SampleNumberDensity", coeff2);
  calculate->setProperty("ScatteringXSection", coeff3);
  calculate->setProperty("Absorption", doAbs);
  calculate->setProperty("MultipleScattering", doMS);
  calculate->execute();
  WorkspaceGroup_sptr calcOutput =
      calculate->getProperty("OutputWorkspaceBaseName");
  return calcOutput;
}

MatrixWorkspace_sptr
CarpenterSampleCorrection::divide(const MatrixWorkspace_sptr lhsWS,
                                  const MatrixWorkspace_sptr rhsWS) {
  IAlgorithm_sptr divide = this->createChildAlgorithm("Divide", 0.25, 0.5);
  divide->setProperty("LHSWorkspace", lhsWS);
  divide->setProperty("RHSWorkspace", rhsWS);
  divide->execute();
  MatrixWorkspace_sptr outWS = divide->getProperty("OutputWorkspace");
  return outWS;
}

MatrixWorkspace_sptr
CarpenterSampleCorrection::multiply(const MatrixWorkspace_sptr lhsWS,
                                    const MatrixWorkspace_sptr rhsWS) {
  auto multiply = this->createChildAlgorithm("Multiply", 0.5, 0.75);
  multiply->setProperty("LHSWorkspace", lhsWS);
  multiply->setProperty("RHSWorkspace", rhsWS);
  multiply->execute();
  MatrixWorkspace_sptr outWS = multiply->getProperty("OutputWorkspace");
  return outWS;
}

MatrixWorkspace_sptr
CarpenterSampleCorrection::minus(const MatrixWorkspace_sptr lhsWS,
                                 const MatrixWorkspace_sptr rhsWS) {
  auto minus = this->createChildAlgorithm("Minus", 0.75, 1.0);
  minus->setProperty("LHSWorkspace", lhsWS);
  minus->setProperty("RHSWorkspace", rhsWS);
  minus->execute();
  MatrixWorkspace_sptr outWS = minus->getProperty("OutputWorkspace");
  return outWS;
}

} // namespace Algorithm
} // namespace Mantid
