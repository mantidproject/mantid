//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SetUncertainties.h"
#include "MantidAPI/MatrixWorkspace.h"
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

/// (Empty) Constructor
SetUncertainties::SetUncertainties() : API::Algorithm() {}

/// Virtual destructor
SetUncertainties::~SetUncertainties() = default;

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

namespace {
inline bool isMasked(MatrixWorkspace_const_sptr wksp, const size_t index) {
  if (!bool(wksp->getInstrument()))
    return false;

  try {
    const auto det = wksp->getDetector(index);
    if (bool(det))
      return det->isMasked();
  } catch (Kernel::Exception::NotFoundError &e) {
    UNUSED_ARG(e);
  }

  return false;
}
} // anonymous namespace

void SetUncertainties::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  std::string errorType = getProperty("SetError");
  bool zeroError = (errorType.compare(ZERO) == 0);
  bool takeSqrt =
      ((errorType.compare(SQRT) == 0) || (errorType.compare(SQRT_OR_ONE) == 0));
  bool resetOne = ((errorType.compare(ONE_IF_ZERO) == 0) ||
                   (errorType.compare(SQRT_OR_ONE) == 0));

  // Create the output workspace. This will copy many aspects from the input
  // one.
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create(inputWorkspace);

  // ...but not the data, so do that here.
  const size_t numHists = inputWorkspace->getNumberHistograms();
  Progress prog(this, 0.0, 1.0, numHists);

  PARALLEL_FOR2(inputWorkspace, outputWorkspace)
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // copy the X/Y
    outputWorkspace->setX(i, inputWorkspace->refX(i));
    outputWorkspace->dataY(i) = inputWorkspace->readY(i);
    // copy the E or set to zero depending on the mode
    if (errorType.compare(ONE_IF_ZERO) == 0) {
      outputWorkspace->dataE(i) = inputWorkspace->readE(i);
    } else {
      outputWorkspace->dataE(i) =
          std::vector<double>(inputWorkspace->readE(i).size(), 0.);
    }

    // ZERO mode doesn't calculate anything further
    if ((!zeroError) && (!isMasked(inputWorkspace, i))) {
      MantidVec &E = outputWorkspace->dataE(i);
      if (takeSqrt) {
        const MantidVec &Y = outputWorkspace->readY(i);
        std::transform(Y.begin(), Y.end(), E.begin(),
                       sqrterror(resetOne ? 1. : 0.));
      } else {
        std::for_each(E.begin(), E.end(), resetzeroerror(1.));
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
