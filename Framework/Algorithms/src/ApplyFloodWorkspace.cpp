#include "MantidAlgorithms/ApplyFloodWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
namespace Prop {
std::string const INPUT_WORKSPACE("InputWorkspace");
std::string const FLOOD_WORKSPACE("FloodWorkspace");
std::string const OUTPUT_WORKSPACE("OutputWorkspace");
} // namespace Prop
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyFloodWorkspace)

const std::string ApplyFloodWorkspace::name() const {
  return "ApplyFloodWorkspace";
}

const std::string ApplyFloodWorkspace::summary() const {
  return "Algorithm to apply a flood correction to a workspace.";
}

int ApplyFloodWorkspace::version() const { return 1; }

const std::vector<std::string> ApplyFloodWorkspace::seeAlso() const {
  return {"ReflectometryReductionOneAuto"};
}

const std::string ApplyFloodWorkspace::category() const {
  return "Reflectometry\\ISIS";
}

void ApplyFloodWorkspace::init() {

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      Prop::INPUT_WORKSPACE, "", Direction::Input),
                  "The workspace to correct.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      Prop::FLOOD_WORKSPACE, "", Direction::Input),
                  "The flood workspace.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      Prop::OUTPUT_WORKSPACE, "", Direction::Output),
                  "The corrected workspace.");
}

void ApplyFloodWorkspace::exec() {
  MatrixWorkspace_sptr input = getProperty(Prop::INPUT_WORKSPACE);
  MatrixWorkspace_sptr flood = getProperty(Prop::FLOOD_WORKSPACE);
  auto divide = createChildAlgorithm("Divide", 0, 1);
  divide->setProperty("LHSWorkspace", input);
  divide->setProperty("RHSWorkspace", flood);
  divide->setProperty("OutputWorkspace", "dummy");
  divide->execute();
  MatrixWorkspace_sptr output = divide->getProperty("OutputWorkspace");
  setProperty(Prop::OUTPUT_WORKSPACE, output);
}

} // namespace Algorithms
} // namespace Mantid
