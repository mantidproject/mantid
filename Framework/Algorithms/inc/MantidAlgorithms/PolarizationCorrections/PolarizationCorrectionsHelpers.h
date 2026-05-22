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
#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unsupported/Eigen/AutoDiff>
#include <utility>

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

template <typename Provider, typename InputArray, typename CovarianceMatrix>
concept CovarianceMatrixProviderFor =
    std::invocable<Provider, const InputArray &, const InputArray &> &&
    std::convertible_to<std::invoke_result_t<Provider, const InputArray &, const InputArray &>, CovarianceMatrix>;

template <size_t N> class ErrorTypeHelper {
public:
  using DerType = Eigen::Matrix<double, N, 1>;
  using InputArray = DerType;
  using ADScalar = Eigen::AutoDiffScalar<DerType>;
  using CovarianceMatrix = Eigen::Matrix<double, N, N>;
};

template <size_t IndependentVars, size_t DependentVars, typename... DependentFuncs> class CovarianceMatrixProvider {
public:
  static_assert(sizeof...(DependentFuncs) == DependentVars, "Number of dependent functions must match DependentVars");

  static constexpr size_t TotalVars = IndependentVars + DependentVars;
  using IndependentTypes = ErrorTypeHelper<IndependentVars>;
  using Types = ErrorTypeHelper<TotalVars>;
  using InputArray = typename Types::InputArray;
  using CovarianceMatrix = typename Types::CovarianceMatrix;
  using ADScalar = typename IndependentTypes::ADScalar;
  using IndependentDerType = typename IndependentTypes::DerType;
  using FunctionInput = std::array<ADScalar, TotalVars>;

  explicit CovarianceMatrixProvider(DependentFuncs... dependentFuncs)
      : m_dependentFuncs(std::move(dependentFuncs)...) {}

  CovarianceMatrix operator()(const InputArray &inputValues, const InputArray &inputErrors) const {
    CovarianceMatrix covariance = inputErrors.array().square().matrix().asDiagonal();
    const auto functionInputs = makeFunctionInput(inputValues);
    const auto derivatives = dependentDerivatives(functionInputs, std::make_index_sequence<DependentVars>{});

    for (size_t dep = 0; dep < DependentVars; ++dep) {
      const size_t depIndex = IndependentVars + dep;
      for (size_t independent = 0; independent < IndependentVars; ++independent) {
        const double covarianceWithIndependent =
            derivatives[dep][independent] * inputErrors[independent] * inputErrors[independent];
        covariance(independent, depIndex) = covarianceWithIndependent;
        covariance(depIndex, independent) = covarianceWithIndependent;
      }
    }

    // Preserve supplied dependent variances on the diagonal. Only dependent-dependent off-diagonal terms are inferred.
    for (size_t depA = 0; depA < DependentVars; ++depA) {
      const size_t depAIndex = IndependentVars + depA;
      for (size_t depB = depA + 1; depB < DependentVars; ++depB) {
        const size_t depBIndex = IndependentVars + depB;
        double covarianceBetweenDependents = 0.0;
        for (size_t independent = 0; independent < IndependentVars; ++independent) {
          covarianceBetweenDependents += derivatives[depA][independent] * derivatives[depB][independent] *
                                         inputErrors[independent] * inputErrors[independent];
        }
        covariance(depAIndex, depBIndex) = covarianceBetweenDependents;
        covariance(depBIndex, depAIndex) = covarianceBetweenDependents;
      }
    }

    return covariance;
  }

private:
  FunctionInput makeFunctionInput(const InputArray &inputValues) const {
    FunctionInput functionInputs;
    for (size_t i = 0; i < IndependentVars; ++i) {
      functionInputs[i] = ADScalar(inputValues[i], IndependentDerType::Unit(IndependentVars, i));
    }
    for (size_t i = IndependentVars; i < TotalVars; ++i) {
      functionInputs[i] = ADScalar(inputValues[i], IndependentDerType::Zero());
    }
    return functionInputs;
  }

  template <typename Func>
  ADScalar evaluateDependentFunction(Func const &func, const FunctionInput &functionInputs) const {
    static_assert(std::invocable<Func, const FunctionInput &>,
                  "Dependent functions must accept the covariance provider input values");
    return func(functionInputs);
  }

  template <size_t... Indices>
  std::array<IndependentDerType, DependentVars> dependentDerivatives(const FunctionInput &functionInputs,
                                                                     std::index_sequence<Indices...>) const {
    return {evaluateDependentFunction(std::get<Indices>(m_dependentFuncs), functionInputs).derivatives()...};
  }

  std::tuple<DependentFuncs...> m_dependentFuncs;
};

// The first IndependentVars inputs are treated as independent. The following DependentVars inputs are treated as
// quantities derived from the independent variables. If dependent variable d_a has derivatives J_ai = dd_a/dx_i, then
// Cov(x_i, d_a) = J_ai Var(x_i) and Cov(d_a, d_b) = sum_i J_ai J_bi Var(x_i). Dependent functions must express each
// dependency directly in terms of the independent variables; if one dependent quantity depends on another, inline that
// dependency into the function. Use the input values' .value() when a fixed bin value is needed.
template <size_t IndependentVars, size_t DependentVars, typename... DependentFuncs>
auto makeCovarianceMatrixProvider(DependentFuncs &&...dependentFuncs) {
  return CovarianceMatrixProvider<IndependentVars, DependentVars, std::decay_t<DependentFuncs>...>(
      std::forward<DependentFuncs>(dependentFuncs)...);
}

template <size_t N, typename Func> class ErrorPropagation {
public:
  using Types = ErrorTypeHelper<N>;
  using DerType = Types::DerType;
  using ADScalar = Types::ADScalar;
  using InputArray = Types::InputArray;
  using CovarianceMatrix = Types::CovarianceMatrix;
  ErrorPropagation(Func func) : computeFunc(std::move(func)) {}

  struct AutoDevResult {
    double value;
    double error;
    Eigen::Array<double, N, 1> derivatives;
  };

  AutoDevResult evaluate(const InputArray &values, const InputArray &errors) const {
    return evaluateWithCovariance(values, covarianceMatrixFromErrors(errors));
  }

  AutoDevResult evaluateWithCovariance(const InputArray &values, const CovarianceMatrix &covariance) const {
    std::array<ADScalar, N> x;
    for (size_t i = 0; i < N; ++i) {
      x[i] = ADScalar(values[i], DerType::Unit(N, i));
    }
    const ADScalar y = computeFunc(x);
    const auto &derivatives = y.derivatives();
    // First-order Taylor propagation with correlated inputs:
    // Var(f) = J C J^T, where J is the row vector of derivatives df/dx_i and C is the covariance matrix.
    const double variance = derivatives.dot(covariance * derivatives);
    return {y.value(), std::sqrt(std::max(variance, 0.0)), derivatives};
  }

  template <std::same_as<API::MatrixWorkspace_sptr>... Ts>
  API::MatrixWorkspace_sptr evaluateWorkspaces(const bool outputWorkspaceDistribution, Ts... args) const {
    return evaluateWorkspacesImpl(outputWorkspaceDistribution, independentCovarianceMatrixProvider,
                                  std::forward<Ts>(args)...);
  }

  template <std::same_as<API::MatrixWorkspace_sptr>... Ts>
  API::MatrixWorkspace_sptr evaluateWorkspaces(Ts... args) const {
    return evaluateWorkspacesImpl(std::nullopt, independentCovarianceMatrixProvider, std::forward<Ts>(args)...);
  }

  template <typename Provider, std::same_as<API::MatrixWorkspace_sptr>... Ts>
    requires CovarianceMatrixProviderFor<Provider, InputArray, CovarianceMatrix>
  API::MatrixWorkspace_sptr evaluateWorkspacesWithCovariance(const bool outputWorkspaceDistribution,
                                                             Provider covarianceMatrixProvider, Ts... args) const {
    return evaluateWorkspacesImpl(outputWorkspaceDistribution, covarianceMatrixProvider, std::forward<Ts>(args)...);
  }

  template <typename Provider, std::same_as<API::MatrixWorkspace_sptr>... Ts>
    requires CovarianceMatrixProviderFor<Provider, InputArray, CovarianceMatrix>
  API::MatrixWorkspace_sptr evaluateWorkspacesWithCovariance(Provider covarianceMatrixProvider, Ts... args) const {
    return evaluateWorkspacesImpl(std::nullopt, covarianceMatrixProvider, std::forward<Ts>(args)...);
  }

  static CovarianceMatrix covarianceMatrixFromErrors(const InputArray &errors) {
    // Independent inputs have no off-diagonal covariance terms, so their covariance matrix is diagonal with
    // C_ii = Var(x_i) = sigma_i^2.
    return errors.array().square().matrix().asDiagonal();
  }

private:
  Func computeFunc;

  static CovarianceMatrix independentCovarianceMatrixProvider(const InputArray &, const InputArray &errors) {
    return covarianceMatrixFromErrors(errors);
  }

  template <typename Provider, std::same_as<API::MatrixWorkspace_sptr>... Ts>
    requires CovarianceMatrixProviderFor<Provider, InputArray, CovarianceMatrix>
  API::MatrixWorkspace_sptr evaluateWorkspacesImpl(std::optional<bool> outputWorkspaceDistribution,
                                                   Provider covarianceMatrixProvider, Ts... args) const {
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
        const InputArray values{args->y(i)[j]...};
        const InputArray errors(args->e(i)[j]...);
        const CovarianceMatrix covariance = covarianceMatrixProvider(values, errors);
        const auto result = evaluateWithCovariance(values, covariance);
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
