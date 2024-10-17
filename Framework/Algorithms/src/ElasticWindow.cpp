// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ElasticWindow.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ElasticWindow)

using namespace Kernel;
using namespace API;

void ElasticWindow::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<WorkspaceUnitValidator>("DeltaE")),
                  "The input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputInQ", "", Direction::Output),
                  "The name for output workspace with the X axis in units of Q");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputInQSquared", "", Direction::Output),
                  "The name for output workspace with the X axis in units of Q^2.");
  declareProperty("IntegrationRangeStart", EMPTY_DBL(), std::make_shared<MandatoryValidator<double>>(),
                  "Start Point of Range 1");
  declareProperty("IntegrationRangeEnd", EMPTY_DBL(), std::make_shared<MandatoryValidator<double>>(),
                  "End Point of Range 1");
  declareProperty("BackgroundRangeStart", EMPTY_DBL(), "Start Point of Range 2", Direction::Input);
  declareProperty("BackgroundRangeEnd", EMPTY_DBL(), "End Point of Range 2.", Direction::Input);
}

void ElasticWindow::exec() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  double intRangeStart = getProperty("IntegrationRangeStart");
  double intRangeEnd = getProperty("IntegrationRangeEnd");
  double bgRangeStart = getProperty("BackgroundRangeStart");
  double bgRangeEnd = getProperty("BackgroundRangeEnd");

  // Create the output workspaces
  MatrixWorkspace_sptr integWS;

  MatrixWorkspace_sptr outputQ;
  MatrixWorkspace_sptr outputQSquared;

  const bool childAlgLogging(true);
  double startProgress(0.0), endProgress(0.0);

  // Determine if we are converting from spectra number (red) or Q (Sqw)
  const bool axisIsSpectrumNumber = inputWorkspace->getAxis(1)->isSpectra();
  g_log.information() << "Axis is spectrum number: " << axisIsSpectrumNumber << '\n';

  // Determine if we need to use the second time range...
  const bool backgroundSubtraction = !((bgRangeStart == bgRangeEnd) && (bgRangeStart == EMPTY_DBL()));
  g_log.information() << "Use background subtraction: " << backgroundSubtraction << '\n';

  // Calculate number of steps
  size_t numSteps = 4;
  if (backgroundSubtraction)
    numSteps += 1;
  if (axisIsSpectrumNumber)
    numSteps += 1;

  double stepProgress = 1.0 / static_cast<double>(numSteps);

  if (backgroundSubtraction) {
    // ... CalculateFlatBackground, Minus, Integration...
    auto flatBG = createChildAlgorithm("CalculateFlatBackground", startProgress, endProgress, childAlgLogging);
    flatBG->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWorkspace);
    flatBG->setProperty<double>("StartX", bgRangeStart);
    flatBG->setProperty<double>("EndX", bgRangeEnd);
    flatBG->setPropertyValue("Mode", "Mean");
    flatBG->setPropertyValue("OutputWorkspace", "flatBG");
    flatBG->execute();
    startProgress += stepProgress;
    endProgress += stepProgress;

    MatrixWorkspace_sptr flatBGws = flatBG->getProperty("OutputWorkspace");

    auto integ = createChildAlgorithm("Integration", startProgress, endProgress, childAlgLogging);
    integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", flatBGws);
    integ->setProperty<double>("RangeLower", intRangeStart);
    integ->setProperty<double>("RangeUpper", intRangeEnd);
    integ->setPropertyValue("OutputWorkspace", "integ");
    integ->execute();

    integWS = integ->getProperty("OutputWorkspace");
  } else {
    // ... Just Integration ...
    auto integ = createChildAlgorithm("Integration", startProgress, endProgress, childAlgLogging);
    integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWorkspace);
    integ->setProperty<double>("RangeLower", intRangeStart);
    integ->setProperty<double>("RangeUpper", intRangeEnd);
    integ->setPropertyValue("OutputWorkspace", "integ");
    integ->execute();

    integWS = integ->getProperty("OutputWorkspace");
  }
  startProgress += stepProgress;
  endProgress += stepProgress;

  auto const detectorCount = integWS->spectrumInfo().detectorCount();
  if (axisIsSpectrumNumber || detectorCount > 0) {
    if (!axisIsSpectrumNumber) {
      auto spectraAxis = std::make_unique<SpectraAxis>(integWS.get());
      integWS->replaceAxis(1, std::move(spectraAxis));
    }

    // Use ConvertSpectrumAxis v2 for correct result
    const int convertSpectrumAxisVersion = 2;

    // ... ConvertSpectrumAxis (Q) ...
    auto csaQ = createChildAlgorithm("ConvertSpectrumAxis", startProgress, endProgress, childAlgLogging,
                                     convertSpectrumAxisVersion);
    csaQ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integWS);
    csaQ->setPropertyValue("Target", "ElasticQ");
    csaQ->setPropertyValue("EMode", "Indirect");
    csaQ->setPropertyValue("OutputWorkspace", "csaQ");
    csaQ->execute();
    MatrixWorkspace_sptr csaQws = csaQ->getProperty("OutputWorkspace");
    startProgress += stepProgress;
    endProgress += stepProgress;

    // ... ConvertSpectrumAxis (Q2) ...
    auto csaQ2 = createChildAlgorithm("ConvertSpectrumAxis", startProgress, endProgress, childAlgLogging,
                                      convertSpectrumAxisVersion);
    csaQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integWS);
    csaQ2->setPropertyValue("Target", "ElasticQSquared");
    csaQ2->setPropertyValue("EMode", "Indirect");
    csaQ2->setPropertyValue("OutputWorkspace", "csaQ2");
    csaQ2->execute();
    MatrixWorkspace_sptr csaQ2ws = csaQ2->getProperty("OutputWorkspace");
    startProgress += stepProgress;
    endProgress += stepProgress;

    // ... Transpose (Q) ...
    auto tranQ = createChildAlgorithm("Transpose", startProgress, endProgress, childAlgLogging);
    tranQ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", csaQws);
    tranQ->setPropertyValue("OutputWorkspace", "outQ");
    tranQ->execute();
    outputQ = tranQ->getProperty("OutputWorkspace");
    startProgress += stepProgress;
    endProgress += stepProgress;

    // ... Transpose (Q2) ...
    auto tranQ2 = createChildAlgorithm("Transpose", startProgress, endProgress, childAlgLogging);
    tranQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", csaQ2ws);
    tranQ2->setPropertyValue("OutputWorkspace", "outQSquared");
    tranQ2->execute();
    outputQSquared = tranQ2->getProperty("OutputWorkspace");
  } else {
    // ... Transpose (Q) ...
    auto tranQ = createChildAlgorithm("Transpose", startProgress, endProgress, childAlgLogging);
    tranQ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integWS);
    tranQ->setPropertyValue("OutputWorkspace", "outQ");
    tranQ->execute();
    outputQ = tranQ->getProperty("OutputWorkspace");
    startProgress += stepProgress;
    endProgress += stepProgress;

    // ... Convert to Histogram (Q2) ...
    auto histQ2 = createChildAlgorithm("ConvertToHistogram", startProgress, endProgress, childAlgLogging);
    histQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputQ);
    histQ2->setPropertyValue("OutputWorkspace", "outQ");
    histQ2->execute();
    MatrixWorkspace_sptr qHistWS = histQ2->getProperty("OutputWorkspace");
    startProgress += stepProgress;
    endProgress += stepProgress;

    // ... Convert Units (Q2) ...
    auto convUnitQ2 = createChildAlgorithm("ConvertUnits", startProgress, endProgress, childAlgLogging);
    convUnitQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", qHistWS);
    convUnitQ2->setPropertyValue("Target", "QSquared");
    convUnitQ2->setPropertyValue("EMode", "Indirect");
    convUnitQ2->setPropertyValue("OutputWorkspace", "outQSquared");
    convUnitQ2->execute();
    outputQSquared = convUnitQ2->getProperty("OutputWorkspace");
  }
  auto yLabel = outputQSquared->YUnitLabel();
  outputQSquared->setYUnitLabel("ln(" + yLabel + ")");

  setProperty("OutputInQ", outputQ);
  setProperty("OutputInQSquared", outputQSquared);
}

} // namespace Mantid::Algorithms
