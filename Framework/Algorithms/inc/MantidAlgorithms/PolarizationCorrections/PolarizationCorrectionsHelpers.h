// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/MultiThreaded.h"
#include <Eigen/Dense>
#include <optional>
#include <unsupported/Eigen/AutoDiff>

namespace Mantid::Algorithms {
namespace PolarizationCorrectionsHelpers {
MANTID_ALGORITHMS_DLL API::MatrixWorkspace_sptr workspaceForSpinState(const API::WorkspaceGroup_sptr &group,
                                                                      const std::string &spinStateOrder,
                                                                      const std::string &targetSpinState);
} // namespace PolarizationCorrectionsHelpers

namespace FlipperConfigurations {
static const std::string OFF_ON = "01";
static const std::string ON_OFF = "10";
static const std::string OFF_OFF = "00";
static const std::string ON_ON = "11";
static const std::string OFF = "0";
static const std::string ON = "1";
} // namespace FlipperConfigurations

namespace SpinStateConfigurationsFredrikze {
static const std::string PARA_ANTI = "pa";
static const std::string ANTI_PARA = "ap";
static const std::string PARA_PARA = "pp";
static const std::string ANTI_ANTI = "aa";
static const std::string PARA = "p";
static const std::string ANTI = "a";
} // namespace SpinStateConfigurationsFredrikze

namespace SpinStateConfigurationsWildes {
static const std::string MINUS_PLUS = "-+";
static const std::string PLUS_MINUS = "+-";
static const std::string MINUS_MINUS = "--";
static const std::string PLUS_PLUS = "++";
static const std::string MINUS = "-";
static const std::string PLUS = "+";
} // namespace SpinStateConfigurationsWildes

namespace SpinStatesORSO {
/*
 * Polarization constants and helper methods to support the Reflectometry ORSO file format
 */
static const std::string PP = "pp";
static const std::string PM = "pm";
static const std::string MP = "mp";
static const std::string MM = "mm";
static const std::string PO = "po";
static const std::string MO = "mo";

static const std::string LOG_NAME = "spin_state_ORSO";

MANTID_ALGORITHMS_DLL const std::string &getORSONotationForSpinState(const std::string &spinState);
MANTID_ALGORITHMS_DLL void addORSOLogForSpinState(const Mantid::API::MatrixWorkspace_sptr &ws,
                                                  const std::string &spinState);
} // namespace SpinStatesORSO

namespace Arithmetic {

template <size_t N> class ErrorTypeHelper {
public:
  using DerType = Eigen::Matrix<double, N, 1>;
  using InputArray = DerType;
  using ADScalar = Eigen::AutoDiffScalar<DerType>;
};

template <size_t N, typename Func> class ErrorPropagation {
public:
  using Types = ErrorTypeHelper<N>;
  using DerType = Types::DerType;
  using ADScalar = Types::ADScalar;
  using InputArray = Types::InputArray;
  ErrorPropagation(Func func) : computeFunc(std::move(func)) {}

  struct AutoDevResult {
    double value;
    double error;
    Eigen::Array<double, N, 1> derivatives;
  };

  AutoDevResult evaluate(const InputArray &values, const InputArray &errors) const {
    std::array<ADScalar, N> x;
    for (size_t i = 0; i < N; ++i) {
      x[i] = ADScalar(values[i], DerType::Unit(N, i));
    }
    const ADScalar y = computeFunc(x);
    const auto &derivatives = y.derivatives();
    return {y.value(), std::sqrt((derivatives.array().square() * errors.array().square()).sum()), derivatives};
  }

  template <std::same_as<API::MatrixWorkspace_sptr>... Ts>
  API::MatrixWorkspace_sptr evaluateWorkspaces(const bool outputWorkspaceDistribution, Ts... args) const {
    return evaluateWorkspacesImpl(outputWorkspaceDistribution, std::forward<Ts>(args)...);
  }

  template <std::same_as<API::MatrixWorkspace_sptr>... Ts>
  API::MatrixWorkspace_sptr evaluateWorkspaces(Ts... args) const {
    return evaluateWorkspacesImpl(std::nullopt, std::forward<Ts>(args)...);
  }

private:
  Func computeFunc;

  template <std::same_as<API::MatrixWorkspace_sptr>... Ts>
  API::MatrixWorkspace_sptr evaluateWorkspacesImpl(std::optional<bool> outputWorkspaceDistribution, Ts... args) const {
    const auto firstWs = std::get<0>(std::forward_as_tuple(args...));
    API::MatrixWorkspace_sptr outWs = firstWs->clone();

    if (outWs->id() == "EventWorkspace") {
      outWs = convertToWorkspace2D(outWs);
    }

    const size_t numSpec = outWs->getNumberHistograms();
    const size_t specSize = outWs->blocksize();

    // cppcheck-suppress unreadVariable
    const bool isThreadSafe = Kernel::threadSafe((*args)..., *outWs);
    // cppcheck-suppress unreadVariable
    const bool specOverBins = numSpec > specSize;

    PARALLEL_FOR_IF(isThreadSafe && specOverBins)
    for (int64_t i = 0; i < static_cast<int64_t>(numSpec); i++) {
      auto &yOut = outWs->mutableY(i);
      auto &eOut = outWs->mutableE(i);

      PARALLEL_FOR_IF(isThreadSafe && !specOverBins)
      for (int64_t j = 0; j < static_cast<int64_t>(specSize); ++j) {
        const auto result = evaluate(InputArray{args->y(i)[j]...}, InputArray(args->e(i)[j]...));
        yOut[j] = result.value;
        eOut[j] = result.error;
      }
    }

    if (outputWorkspaceDistribution.has_value()) {
      outWs->setDistribution(outputWorkspaceDistribution.value());
    }
    return outWs;
  }

  API::MatrixWorkspace_sptr runWorkspaceConversionAlg(const API::MatrixWorkspace_sptr &workspace,
                                                      const std::string &algName) const {
    auto conversionAlg = API::AlgorithmManager::Instance().create(algName);
    conversionAlg->initialize();
    conversionAlg->setChild(true);
    conversionAlg->setProperty("InputWorkspace", workspace);
    conversionAlg->setProperty("OutputWorkspace", workspace->getName());
    conversionAlg->execute();
    return conversionAlg->getProperty("OutputWorkspace");
  }

  API::MatrixWorkspace_sptr convertToWorkspace2D(const API::MatrixWorkspace_sptr &workspace) const {
    runWorkspaceConversionAlg(workspace, "ConvertToHistogram");
    return runWorkspaceConversionAlg(workspace, "ConvertToMatrixWorkspace");
  }
};

template <size_t N, typename Func> auto makeErrorPropagation(Func &&func) {
  return ErrorPropagation<N, std::decay_t<Func>>(std::forward<Func>(func));
}

} // namespace Arithmetic
} // namespace Mantid::Algorithms
