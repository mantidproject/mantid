#include "MantidAlgorithms/CalculateIqt.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
  constexpr int DEFAULT_ITERATIONS = 10;
  constexpr int DEFAULT_SEED = 23021997;

  MatrixWorkspace_sptr rebin(MatrixWorkspace_sptr workspace, const std::string &params) {
    IAlgorithm_sptr rebinAlgorithm = AlgorithmManager::Instance().create("Rebin");
    rebinAlgorithm->setChild(true);
    rebinAlgorithm->initialize();
    rebinAlgorithm->setProperty("InputWorkspace", workspace);
    rebinAlgorithm->setProperty("Params", params);
    rebinAlgorithm->execute();
    return rebinAlgorithm->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr integration(MatrixWorkspace_sptr workspace) {
    IAlgorithm_sptr integrationAlgorithm = AlgorithmManager::Instance().create("Integration");
    integrationAlgorithm->setChild(true);
    integrationAlgorithm->initialize();
    integrationAlgorithm->setProperty("InputWorkspace", workspace);
    integrationAlgorithm->execute();
    return integrationAlgorithm->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr convertToPointData(MatrixWorkspace_sptr workspace) {
    IAlgorithm_sptr rebinAlgorithm = AlgorithmManager::Instance().create("ConvertToPointData");
    rebinAlgorithm->setChild(true);
    rebinAlgorithm->initialize();
    rebinAlgorithm->setProperty("InputWorkspace", workspace);
    rebinAlgorithm->execute();
    return rebinAlgorithm->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr extractFFTSpectrum(MatrixWorkspace_sptr workspace) {
    IAlgorithm_sptr rebinAlgorithm = AlgorithmManager::Instance().create("ExtractFFTSpectrum");
    rebinAlgorithm->setChild(true);
    rebinAlgorithm->initialize();
    rebinAlgorithm->setProperty("InputWorkspace", workspace);
    rebinAlgorithm->setProperty("FFTPart", 2);
    rebinAlgorithm->execute();
    return rebinAlgorithm->getProperty("OutputWorkspace");
  }

  MatrixWorkspace_sptr divide(MatrixWorkspace_sptr lhsWorkspace, MatrixWorkspace_sptr rhsWorkspace) {
    IAlgorithm_sptr rebinAlgorithm = AlgorithmManager::Instance().create("ConvertToPointData");
    rebinAlgorithm->setChild(true);
    rebinAlgorithm->initialize();
    rebinAlgorithm->setProperty("LHSWorkspace", lhsWorkspace);
    rebinAlgorithm->setProperty("RHSWorkspace", rhsWorkspace);
    rebinAlgorithm->execute();
    return rebinAlgorithm->getProperty("OutputWorkspace");
  }

  std::string createRebinString(double minimum, double maximum, double width) {
    std::stringstream rebinStream;
    rebinStream.precision(14);
    rebinStream << minimum << ", " << width << ", " << maximum;
    return rebinStream.str();
  }

  struct NormalisedFourierTransform {
  public:
    explicit NormalisedFourierTransform(const std::string &rebinParams) : m_rebinParams(rebinParams) {}

    MatrixWorkspace_sptr operator()(MatrixWorkspace_sptr workspace) const {
      workspace = rebin(workspace, m_rebinParams);
      auto workspace_int = integration(workspace);
      workspace = convertToPointData(workspace);
      workspace = extractFFTSpectrum(workspace);
      return divide(workspace, workspace_int);
    }

  private:
    const std::string m_rebinParams;
  };

  MatrixWorkspace_sptr calculateIqt(MatrixWorkspace_sptr sample, MatrixWorkspace_sptr resolution,
    const NormalisedFourierTransform &fourierTransform) {
    return divide(fourierTransform(sample), resolution);
  }
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

  declareProperty("EnergyMin", -0.5, "Minimum energy for fit. Default = -.0.5.");
  declareProperty("EnergyMax", 0.5, "Maximum energy for fit. Default = 0.5.");
  declareProperty("EnergyWidth", 0.1, "Width of energy bins for fit.");

  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);

  declareProperty("NumberOfIterations", DEFAULT_ITERATIONS, positiveInt, "Number of randomised simulations within error to run.");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt, "Seed the random number generator for monte-carlo error calculation.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
    Direction::Output), "The name to use for the output workspace.");
}

void CalculateIqt::exec() {
  NormalisedFourierTransform fourierTransform(rebinParamsAsString());
  MatrixWorkspace_sptr sample = getProperty("InputWorkspace");
  MatrixWorkspace_sptr resolution = getProperty("ResolutionWorkspace");
  resolution = fourierTransform(resolution);
  calculateIqt(sample, resolution, fourierTransform);
}

std::string CalculateIqt::rebinParamsAsString() {
  const double e_min = getProperty("EnergyMin");
  const double e_max = getProperty("EnergyMax");
  const double e_width = getProperty("EnergyWidth");
  return createRebinString(e_min, e_max, e_width);
}

std::map<std::string, std::string> CalculateIqt::validateInputs() {
  std::map<std::string, std::string> emptyMap;
  return emptyMap;
}

} // namespace Algorithms
} // namespace Mantid
