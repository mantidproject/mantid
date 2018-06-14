#include "MantidAlgorithms/CalculateIqt.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
  constexpr int DEFAULT_ITERATIONS = 10;
  constexpr int DEFAULT_SEED = 23021997;
}


namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CalculateIqt)

const std::string CalculateIqt::name() const { return "CalculateIqt"; }
int CalculateIqt::version() const { return 1; }
const std::vector<std::string> CalculateIqt::seeAlso() const { return{ "TransformToIqt" }; }
const std::string CalculateIqt::category() const { return "Inelastic\\Indirect"; }
const std::string CalculateIqt::summary() const {
  return "Calculates I(Q,t) from S(Q,w) and computes the errors using a monte-carlo routine.";
}

void CalculateIqt::init() {
  
  declareProperty(make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
    "The name of the sample workspace.");

  declareProperty(make_unique<WorkspaceProperty<>>("ResolutionWorkspace", "", Direction::Input),
    "The name of the resolution workspace.");

  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);

  declareProperty("EnergyMin", -0.5, "Minimum energy for fit. Default = -.0.5.");
  declareProperty("EnergyMax", 0.5, "Maximum energy for fit. Default = 0.5.");
  declareProperty("EnergyWidth", 1, "Width of energy bins for fit."); // sensible default value?

  declareProperty("NumberOfIterations", DEFAULT_ITERATIONS, positiveInt, "Number of randomised simulations within error to run.");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt, "Seed the random number generator for monte-carlo error calculation.");
}

void CalculateIqt::exec() {
  return;
}

std::map<std::string, std::string> CalculateIqt::validateInputs() {
  std::map<std::string, std::string> emptyMap;
  return emptyMap;
}

} // namespace Algorithms
} // namespace Mantid
