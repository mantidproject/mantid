// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/MultipleScatteringCorrection.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;
using namespace Mantid::DataObjects;

DECLARE_ALGORITHM(MultipleScatteringCorrection)

/**
 * @brief interface initialisation method
 *
 */
void MultipleScatteringCorrection::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The X values for the input workspace must be in units of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "Output workspace name");

  auto positiveInt = std::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", static_cast<int64_t>(EMPTY_INT()), positiveInt,
                  "The number of wavelength points for which the numerical integral is\n"
                  "calculated (default: all points)");

  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, "The size of one side of an integration element cube in mm");
}

/**
 * @brief validate the inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> MultipleScatteringCorrection::validateInputs() {
  std::map<std::string, std::string> result;

  // verify that the container information is there if requested
  API::MatrixWorkspace_const_sptr wksp = getProperty("InputWorkspace");
  const auto &sample = wksp->sample();
  if (sample.hasEnvironment()) {
    const auto numComponents = sample.getEnvironment().nelements();
    // first element is assumed to be the container
    if (numComponents == 0) {
      result["InputWorkspace"] = "Sample does not have a container defined";
    }
  } else {
    result["InputWorkspace"] = "Sample does not have a container defined";
  }
  return result;
}

/**
 * @brief execute the algorithm
 *
 */
void MultipleScatteringCorrection::exec() {}

} // namespace Algorithms
} // namespace Mantid
