#include "MantidAlgorithms/SetUncertainties.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <vector>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetUncertainties)

using namespace Kernel;
using namespace API;

namespace {
/// Used to compare signal to zero
const double TOLERANCE = 1.e-10;

const std::string ZERO("zero");
const std::string SQRT("sqrt");
const std::string ONE_IF_ZERO("oneIfZero");
const std::string SQRT_OR_ONE("sqrtOrOne");

struct resetzeroerror {
  explicit resetzeroerror(const double constant) : zeroErrorValue(constant) {}

  double operator()(const double error) {
    if (error < TOLERANCE) {
      return zeroErrorValue;
    } else {
      return error;
    }
  }

  double zeroErrorValue;
};

struct sqrterror {
  explicit sqrterror(const double constant) : zeroSqrtValue(constant) {}

  double operator()(const double intensity) {
    const double localIntensity = fabs(intensity);
    if (localIntensity > TOLERANCE) {
      return sqrt(localIntensity);
    } else {
      return zeroSqrtValue;
    }
  }

  double zeroSqrtValue;
};
}

/// Algorithm's name
const std::string SetUncertainties::name() const { return "SetUncertainties"; }

/// Algorithm's version
int SetUncertainties::version() const { return (1); }

void SetUncertainties::init() {
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
      "InputWorkspace", "", Direction::Input));
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
      "OutputWorkspace", "", Direction::Output));
  std::vector<std::string> errorTypes = {ZERO, SQRT, SQRT_OR_ONE, ONE_IF_ZERO};
  declareProperty("SetError", ZERO,
                  boost::make_shared<StringListValidator>(errorTypes),
                  "How to reset the uncertainties");
}

void SetUncertainties::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  std::string errorType = getProperty("SetError");
  bool zeroError = (errorType == ZERO);
  bool takeSqrt = ((errorType == SQRT) || (errorType == SQRT_OR_ONE));
  bool resetOne = ((errorType == ONE_IF_ZERO) || (errorType == SQRT_OR_ONE));

  // Create the output workspace. This will copy many aspects from the input
  // one.
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(inputWorkspace);

  // ...but not the data, so do that here.
  const auto &spectrumInfo = inputWorkspace->spectrumInfo();
  const size_t numHists = inputWorkspace->getNumberHistograms();
  Progress prog(this, 0.0, 1.0, numHists);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // copy the X/Y
    outputWorkspace->setSharedX(i, inputWorkspace->sharedX(i));
    outputWorkspace->setSharedY(i, inputWorkspace->sharedY(i));
    // copy the E or set to zero depending on the mode
    if (errorType == ONE_IF_ZERO) {
      outputWorkspace->setSharedE(i, inputWorkspace->sharedE(i));
    } else {
      outputWorkspace->mutableE(i) = 0.0;
    }

    // ZERO mode doesn't calculate anything further
    if ((!zeroError) &&
        (!(spectrumInfo.hasDetectors(i) && spectrumInfo.isMasked(i)))) {
      auto &E = outputWorkspace->mutableE(i);
      if (takeSqrt) {
        const auto &Y = outputWorkspace->y(i);
        std::transform(Y.begin(), Y.end(), E.begin(),
                       sqrterror(resetOne ? 1. : 0.));
      } else {
        std::transform(E.begin(), E.end(), E.begin(), resetzeroerror(1.));
      }
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
