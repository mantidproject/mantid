// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/LorentzCorrection.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>

namespace Mantid {
namespace Algorithms {
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LorentzCorrection)

/// Algorithm's version for identification. @see Algorithm::version
int LorentzCorrection::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LorentzCorrection::category() const {
  return "Crystal\\Corrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LorentzCorrection::summary() const {
  return "Performs a white beam Lorentz Correction";
}

const std::string LorentzCorrection::name() const {
  return "LorentzCorrection";
}

/** Initialize the algorithm's properties.
 */
void LorentzCorrection::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      PropertyMode::Mandatory,
                      boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Input workspace to correct in Wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/** Execute the algorithm.
 */
void LorentzCorrection::exec() {
  MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");

  auto cloneAlg = this->createChildAlgorithm("CloneWorkspace", 0, 0.1);
  cloneAlg->initialize();
  cloneAlg->setProperty("InputWorkspace", inWS);
  cloneAlg->execute();
  Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr outWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

  const auto numHistos = inWS->getNumberHistograms();
  const auto &spectrumInfo = inWS->spectrumInfo();
  Progress prog(this, 0.0, 1.0, numHistos);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS))
  for (int64_t i = 0; i < int64_t(numHistos); ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (!spectrumInfo.hasDetectors(i))
      continue;

    const double twoTheta =
        spectrumInfo.isMonitor(i) ? 0.0 : spectrumInfo.twoTheta(i);
    const double sinTheta = std::sin(twoTheta / 2);
    double sinThetaSq = sinTheta * sinTheta;

    auto &inY = inWS->y(i);
    auto &outY = outWS->mutableY(i);
    auto &outE = outWS->mutableE(i);
    const auto points = inWS->points(i);
    const auto pos = std::find(cbegin(points), cend(points), 0.0);
    if (pos != cend(points)) {
      std::stringstream buffer;
      buffer << "Cannot have zero values Wavelength. At workspace index: "
             << pos - cbegin(points);
      throw std::runtime_error(buffer.str());
    }
    for (size_t j = 0; j < inY.size(); ++j) {
      double weight =
          sinThetaSq / (points[j] * points[j] * points[j] * points[j]);
      outY[j] *= weight;
      outE[j] *= weight;
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
