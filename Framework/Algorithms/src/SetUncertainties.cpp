//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SetUncertainties.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
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

struct oneifzeroerror {
  double operator()(const double error) {
    if (error < TOLERANCE) {
      return 1.;
    } else {
      return error;
    }
  }
};

struct sqrterror {
  double operator()(const double intensity) {
    const double localIntensity = fabs(intensity);
    if (localIntensity > TOLERANCE) {
      return sqrt(localIntensity);
    } else {
      return 0.;
    }
  }
};
}

/// (Empty) Constructor
SetUncertainties::SetUncertainties() : API::Algorithm() {}

/// Virtual destructor
SetUncertainties::~SetUncertainties() {}

/// Algorithm's name
const std::string SetUncertainties::name() const { return "SetUncertainties"; }

/// Algorithm's version
int SetUncertainties::version() const { return (1); }

const std::string ZERO("zero");
const std::string SQRT("sqrt");
const std::string ONEIFZERO("oneifzero");

void SetUncertainties::init() {
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
      "InputWorkspace", "", Direction::Input));
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
      "OutputWorkspace", "", Direction::Output));
  std::vector<std::string> errorTypes = {ZERO, SQRT, ONEIFZERO};
  declareProperty("SetError", ZERO,
                  boost::make_shared<StringListValidator>(errorTypes),
                  "How to reset the uncertainties");
}

void SetUncertainties::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  std::string errorType = getProperty("SetError");
  bool zeroError = (errorType.compare(ZERO) == 0);
  bool oneIfZeroError = (errorType.compare(ONEIFZERO) == 0);

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
    if (oneIfZeroError) {
      outputWorkspace->dataE(i) = inputWorkspace->readE(i);
    } else {
      outputWorkspace->dataE(i) =
          std::vector<double>(inputWorkspace->readE(i).size(), 0.);
    }

    if (!zeroError) {
      MantidVec &E = outputWorkspace->dataE(i);
      if (oneIfZeroError) {
        std::for_each(E.begin(), E.end(), oneifzeroerror());
      } else {
        const MantidVec &Y = outputWorkspace->readY(i);
        std::transform(Y.begin(), Y.end(), E.begin(), sqrterror());
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
