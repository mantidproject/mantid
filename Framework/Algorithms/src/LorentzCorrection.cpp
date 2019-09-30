// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/LorentzCorrection.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Unit.h"
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
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LorentzCorrection)

namespace { // anonymous
namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string TYPE("Type");
} // namespace PropertyNames

const std::string TOF_SCD("SingleCrystalTOF");
const std::string TOF_PD("PowderTOF");

inline double sinTheta(const API::SpectrumInfo &spectrumInfo, int64_t index) {
  const double twoTheta =
      spectrumInfo.isMonitor(index) ? 0.0 : spectrumInfo.twoTheta(index);
  return std::sin(0.5 * twoTheta);
}
} // namespace

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
                      PropertyNames::INPUT_WKSP, "", Direction::Input,
                      PropertyMode::Mandatory),
                  // boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Input workspace to correct in Wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      PropertyNames::OUTPUT_WKSP, "", Direction::Output),
                  "An output workspace.");
  const std::vector<std::string> correction_types{TOF_SCD, TOF_PD};
  declareProperty(PropertyNames::TYPE, correction_types[0],
                  boost::make_shared<StringListValidator>(correction_types),
                  "Type of Lorentz correction to do");
}

std::map<std::string, std::string> LorentzCorrection::validateInputs() {
  std::map<std::string, std::string> result;

  const auto processingType = this->getPropertyValue(PropertyNames::TYPE);
  // check units if the SCD option is selected
  if (processingType == TOF_SCD) {
    MatrixWorkspace_const_sptr wksp =
        this->getProperty(PropertyNames::INPUT_WKSP);
    // code is a variant of private method from WorkspaceUnitValidator
    const auto unit = wksp->getAxis(0)->unit();
    if ((!unit) || (unit->unitID().compare("Wavelength"))) {
      result[PropertyNames::INPUT_WKSP] =
          "The workspace must have units of Wavelength";
    }
  }

  return result;
}

/** Execute the algorithm.
 */
void LorentzCorrection::exec() {
  MatrixWorkspace_sptr inWS = this->getProperty(PropertyNames::INPUT_WKSP);

  // clone the workspace - does nothing for inplace operation
  auto cloneAlg = this->createChildAlgorithm("CloneWorkspace", 0, 0.1);
  cloneAlg->initialize();
  cloneAlg->setProperty("InputWorkspace", inWS);
  cloneAlg->setPropertyValue(
      "OutputWorkspace", this->getPropertyValue(PropertyNames::OUTPUT_WKSP));
  cloneAlg->execute();
  Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr outWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

  Progress prog(this, 0.1, 1.0, inWS->getNumberHistograms());

  const auto processingType = this->getPropertyValue(PropertyNames::TYPE);
  if (processingType == TOF_SCD)
    processTOF_SCD(outWS, prog);
  else if (processingType == TOF_PD)
    processTOF_PD(outWS, prog);
  else {
    std::stringstream msg;
    msg << "Do not understand know how to process Type=\"" << processingType
        << "\" - developer forgot to fill in if/else tree";
    throw std::runtime_error(msg.str());
  }

  this->setProperty(PropertyNames::OUTPUT_WKSP, outWS);
}

void LorentzCorrection::processTOF_SCD(MatrixWorkspace_sptr &wksp,
                                       Progress &prog) {
  const int64_t numHistos = static_cast<int64_t>(wksp->getNumberHistograms());
  const auto &spectrumInfo = wksp->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int64_t i = 0; i < numHistos; ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (!spectrumInfo.hasDetectors(i))
      continue;

    const double sinTheta_v = sinTheta(spectrumInfo, i);
    const double sinThetaSq = sinTheta_v * sinTheta_v;

    auto &outY = wksp->mutableY(i);
    auto &outE = wksp->mutableE(i);
    const auto points = wksp->points(i);
    const auto num_points = points.size();
    const auto pos = std::find(cbegin(points), cend(points), 0.0);
    if (pos != cend(points)) {
      std::stringstream buffer;
      buffer << "Cannot have zero values Wavelength. At workspace index: "
             << pos - cbegin(points);
      throw std::runtime_error(buffer.str());
    }
    for (size_t j = 0; j < num_points; ++j) {
      double weight =
          sinThetaSq / (points[j] * points[j] * points[j] * points[j]);
      outY[j] *= weight;
      outE[j] *= weight;
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

void LorentzCorrection::processTOF_PD(MatrixWorkspace_sptr &wksp,
                                      Progress &prog) {
  const int64_t numHistos = static_cast<int64_t>(wksp->getNumberHistograms());
  const auto &spectrumInfo = wksp->spectrumInfo();

  EventWorkspace_sptr wkspEvent =
      boost::dynamic_pointer_cast<EventWorkspace>(wksp);
  bool isEvent = bool(wkspEvent);

  PARALLEL_FOR_IF(Kernel::threadSafe(*wksp))
  for (int64_t i = 0; i < numHistos; ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (!spectrumInfo.hasDetectors(i))
      continue;

    const double sinTheta_v = sinTheta(spectrumInfo, i);

    const auto points = wksp->points(i);
    const auto pos = std::find(cbegin(points), cend(points), 0.0);
    if (pos != cend(points)) {
      std::stringstream buffer;
      buffer << "Cannot have zero values Wavelength. At workspace index: "
             << pos - cbegin(points);
      throw std::runtime_error(buffer.str());
    }

    if (isEvent) {
      wkspEvent->getSpectrum(i) *= sinTheta_v;
    } else {
      wksp->mutableY(i) *= sinTheta_v;
      wksp->mutableE(i) *= sinTheta_v;
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid
