// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ConvertSingleSpectrumLambdaToQ.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include <MantidAPI/WorkspaceUnitValidator.h>
#include <MantidKernel/CompositeValidator.h>

namespace Mantid {
namespace Reflectometry {

// Register with the algorithm factory
DECLARE_ALGORITHM(ConvertSingleSpectrumLambdaToQ)

using namespace Kernel;
using namespace API;

/// Initialisation method
void ConvertSingleSpectrumLambdaToQ::init() {
  auto inputWsValidator = std::make_shared<CompositeValidator>();
  const std::string inputUnit = std::string("Wavelength");
  inputWsValidator->add<WorkspaceUnitValidator>(inputUnit);
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, inputWsValidator),
                  "Name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "ThetaIn", Mantid::EMPTY_DBL(), Direction::Input),
                  "Angle in degrees");
  std::vector<std::string> targetUnitOptions{"MomentumTransfer"};
  declareProperty(
      "Target", "", std::make_shared<StringListValidator>(targetUnitOptions),
      "The name of the units to convert to (must be MomentumTransfer)");
}

/// Execute the algorithm
void ConvertSingleSpectrumLambdaToQ::exec() {
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  setupMemberVariables(inputWs);
  try {
    checkSingleSpectrumLambda();
  } catch (std::invalid_argument &e) {
    std::cerr << e.what();
    return;
  }
  MatrixWorkspace_sptr outputWs = executeUnitConversion(inputWs);
  setProperty("OutputWorkspace", outputWs);
}

/** Initialise the member variables
 *  @param inputWs The input workspace
 */
void ConvertSingleSpectrumLambdaToQ::setupMemberVariables(
    const MatrixWorkspace_const_sptr inputWs) {
  m_numberOfSpectra = inputWs->getNumberHistograms();
  m_theta = getProperty("ThetaIn");
  m_inputUnit = inputWs->getAxis(0)->unit();
  const std::string targetUnit = getPropertyValue("Target");
  m_outputUnit = UnitFactory::Instance().create(targetUnit);
}

/** Check that the workspace satisfies the condition for this algorithm
 *  @param inputWs The input workspace
 */
void ConvertSingleSpectrumLambdaToQ::checkSingleSpectrumLambda() {
  if (m_numberOfSpectra > 1) {
    throw std::invalid_argument("Expected a single group in "
                                "ProcessingInstructions to be able to "
                                "perform angle correction, found " +
                                std::to_string(m_numberOfSpectra));
  } else
    return;
}

/**Executes the main part of the algorithm that handles the conversion of the
 * units
 * @param inputWs :: the input workspace that will be converted
 * @return A pointer to a MatrixWorkspace_sptr that contains the converted units
 */
MatrixWorkspace_sptr ConvertSingleSpectrumLambdaToQ::executeUnitConversion(
    const MatrixWorkspace_sptr inputWs) {
  // transform workspace in lambda to workspace in Q
  auto IvsQ = transform(inputWs);
  // unit conversion has flipped the ascending direction of Y and E,
  // so reverse the vectors
  reverse(IvsQ);
  return IvsQ;
}

/** Perform the unit conversion on a workspace in lambda to a workspace in Q
 *  @param inputWs The workspace to operate on
 */
MatrixWorkspace_sptr
ConvertSingleSpectrumLambdaToQ::transform(const MatrixWorkspace_sptr inputWs) {
  MatrixWorkspace_sptr IvsQ = inputWs->clone();
  auto &XOut0 = IvsQ->mutableX(0);
  const auto &XIn0 = inputWs->x(0);
  double const factor = 4.0 * M_PI * sin(m_theta * M_PI / 180.0);
  std::transform(XIn0.rbegin(), XIn0.rend(), XOut0.begin(),
                 [factor](double x) { return factor / x; });

  return IvsQ;
}

/** Reverses the workspace if Y and E values are in descending order
 *  @param inputWs The workspace to operate on
 */
void ConvertSingleSpectrumLambdaToQ::reverse(MatrixWorkspace_sptr inputWs) {
  auto &Y0 = inputWs->mutableY(0);
  auto &E0 = inputWs->mutableE(0);
  std::reverse(Y0.begin(), Y0.end());
  std::reverse(E0.begin(), E0.end());
  inputWs->getAxis(0)->unit() = m_outputUnit;
}

} // namespace Reflectometry
} // namespace Mantid
