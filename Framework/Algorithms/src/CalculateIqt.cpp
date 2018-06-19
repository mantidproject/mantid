#include "MantidAlgorithms/CalculateIqt.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidHistogramData\HistogramY.h"
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

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
    IAlgorithm_sptr rebinAlgorithm = AlgorithmManager::Instance().create("Integration");
    rebinAlgorithm->setChild(true);
    rebinAlgorithm->initialize();
    rebinAlgorithm->setProperty("InputWorkspace", workspace);
    rebinAlgorithm->execute();
    return rebinAlgorithm->getProperty("OutputWorkspace");
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

  MatrixWorkspace_sptr randomizeYWithinError(MatrixWorkspace_sptr workspace,
  const int seed, const int errorMin, const int errorMax) {
    MersenneTwister rng(seed, errorMin, errorMax);
    const double randomValue = rng.nextValue();
    for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
      workspace->mutableY(i) += randomValue;
    return workspace;
    }

  double standardDeviation(const std::vector<double> &inputValues) {
    double mean = std::accumulate(inputValues.begin(), inputValues.end(), 0.0) / inputValues.size();
    double sumOfXMinusMeanSquared = 0;
    for (auto &x : inputValues) {
      sumOfXMinusMeanSquared += pow(x - mean, 2);
    }
    return sqrt(sumOfXMinusMeanSquared / (inputValues.size() - 1));
  }

  std::vector<double> standardDeviationArray(const std::vector<std::vector<double>> &yValues) {
    std::vector<double> standardDeviations;
    auto outputSize = yValues[0].size();
    standardDeviations.reserve(outputSize);
    std::vector<double> currentRow;
    currentRow.reserve(yValues.size());

    for (auto i = 0u; i < outputSize; ++i) {
      currentRow.clear();
      for (auto &yValueArray : yValues)
        currentRow.emplace_back(yValueArray[i]);
      standardDeviations.emplace_back(standardDeviation(currentRow));
    }
    return standardDeviations;
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
      const auto rebinParams = rebinParamsAsString();
      const MatrixWorkspace_sptr sampleWorkspace = getProperty("InputWorkspace");
      MatrixWorkspace_sptr resolutionWorkspace = getProperty("ResolutionWorkspace");
      resolutionWorkspace = normalizedFourierTransform(resolutionWorkspace, rebinParams);

      //create function which only needs sampleWorkspace as input
      calculateIqtFunc calculateIqtFunction = [this, resolutionWorkspace, &rebinParams](MatrixWorkspace_sptr workspace) {
        return this->calculateIqt(workspace, resolutionWorkspace, rebinParams); };

      monteCarloErrorCalculation(sampleWorkspace, calculateIqtFunction);
    }

    std::string CalculateIqt::rebinParamsAsString() {
      const double e_min = getProperty("EnergyMin");
      const double e_max = getProperty("EnergyMax");
      const double e_width = getProperty("EnergyWidth");
      return createRebinString(e_min, e_max, e_width);
    }

    MatrixWorkspace_sptr CalculateIqt::calculateIqt(MatrixWorkspace_sptr workspace,
      MatrixWorkspace_sptr resolutionWorkspace, const std::string &rebinParams) {
      workspace = normalizedFourierTransform(workspace, rebinParams);
      return divide(workspace, resolutionWorkspace);
    }

    MatrixWorkspace_sptr CalculateIqt::normalizedFourierTransform(MatrixWorkspace_sptr workspace, const std::string &rebinParams) {
      workspace = rebin(workspace, rebinParams);
      auto workspace_int = integration(workspace);
      workspace = convertToPointData(workspace);
      workspace = extractFFTSpectrum(workspace);
      workspace = divide(workspace, workspace_int);
      return workspace;
    }

    void CalculateIqt::monteCarloErrorCalculation(MatrixWorkspace_sptr sample,
      const calculateIqtFunc &calculateIqtFunction) {
      auto outputWorkspace = calculateIqtFunction(sample);
      const unsigned int nIterations = getProperty("NumberOfIterations");
      const unsigned int seed = getProperty("SeedValue");
      const int energyMax = getProperty("EnergyMax");
      const int energyMin = getProperty("EnergyMin");
      std::vector<MatrixWorkspace_sptr> simulatedWorkspaces;
      simulatedWorkspaces.reserve(nIterations);
      simulatedWorkspaces.emplace_back(outputWorkspace);

      for (auto i = 0u; i < nIterations - 1; ++i) {
        auto simulatedWorkspace = randomizeYWithinError(sample->clone(), seed, energyMin, energyMax);
        simulatedWorkspace = calculateIqtFunction(simulatedWorkspace);
        simulatedWorkspaces.emplace_back(simulatedWorkspace);
      }
      std::vector<std::vector<double>> allSimY;
      allSimY.reserve(nIterations);
      for (auto i = 0u; i < outputWorkspace->getNumberHistograms(); ++i) {
        auto &outputY = outputWorkspace->mutableY(i);
        for (auto &simWorkspace : simulatedWorkspaces)
          allSimY.emplace_back(simWorkspace->readY(i));
        outputY = standardDeviationArray(allSimY);
      }
}


std::map<std::string, std::string> CalculateIqt::validateInputs() {
  std::map<std::string, std::string> emptyMap;
  return emptyMap;
}

} // namespace Algorithms
} // namespace Mantid
