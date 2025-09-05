// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

using namespace Mantid;
using namespace Mantid::API;

namespace HeAnalyserTest {
static const std::string REFERENCE_NAME = "reference";
static const std::string INPUT_NAME = "input";
static const std::string GROUP_NAME = "group";
static const std::string OUTPUT_NAME = "groupOut";
static const std::string OUTPUT_TABLE_NAME = "tableOut";
static const std::string OUTPUT_CURVES_NAME = "curvesOut";
static const std::string X_UNIT = "Wavelength";
static const std::string REF_TIMESTAMP = "2025-07-01T08:00:00";

constexpr double WAV_MIN = 1;
constexpr double WAV_MAX = 9;
constexpr double WAV_STEP = 1;
constexpr double DEFAULT_LIFETIME = 45;
constexpr double DEFAULT_INI_POL = 0.9;
constexpr double DEFAULT_PXD = 12;
constexpr double LAMBDA_CONVERSION_FACTOR = 0.0733;

static const std::string HE_ANALYZER_FIT_ALG = "HeliumAnalyserEfficiency";
static const std::string HE_ANALYZER_TIME_ALG = "HeliumAnalyserEfficiencyTime";
static const std::string SPIN_STATE = "00,01,10,11";

constexpr double DELTA = 0.01;

struct PolarizationTestParameters {
  PolarizationTestParameters() = default;
  PolarizationTestParameters(const double tauIni, const double polIni, const double pxdIni)
      : Tau(tauIni), polInitial(polIni), pxd(pxdIni), pxdError(), outPolarizations(), outEfficiencies() {}
  double Tau{DEFAULT_LIFETIME};
  double polInitial{DEFAULT_INI_POL};
  double pxd{DEFAULT_PXD};
  double pxdError{0};
  std::vector<double> outPolarizations{};
  std::vector<std::vector<double>> outEfficiencies{};
};

struct InputTestParameters {
  InputTestParameters() = default;
  InputTestParameters(const int spec, const int bins, const std::string &group, const std::string &wsName,
                      const std::string &units)
      : nSpec(spec), nBins(bins), groupName(group), testName(wsName), xUnit(units) {}
  int nSpec{1};
  int nBins{5};
  std::string groupName{GROUP_NAME};
  std::string testName{OUTPUT_NAME};
  std::string xUnit{X_UNIT};
};

double createFunctionArgument(const double lifetime = DEFAULT_LIFETIME, const double time = 1,
                              const double iniPol = DEFAULT_INI_POL, const double pxd = DEFAULT_PXD);

std::pair<std::vector<double>, std::vector<double>> createXYFromParams(double Xmin = WAV_MIN, double Xmax = WAV_MAX,
                                                                       double step = WAV_STEP, double Y = 1.0);

std::vector<double> generateOutputFunc(const std::vector<double> &x, const double factor = 1, const double mu = 0,
                                       const bool efficiency = true);

void groupWorkspaces(const std::string &name, const std::vector<MatrixWorkspace_sptr> &wsToGroup);

MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                       const std::vector<double> &y, const std::string &xUnit = X_UNIT,
                                       const int nSpec = 1, const double delay = 0,
                                       const std::string &refTimeStamp = REF_TIMESTAMP);

MatrixWorkspace_sptr getMatrixWorkspaceFromInput(const std::string &wsName);

IAlgorithm_sptr prepareHeEffAlgorithm(const std::vector<std::string> &inputWorkspaces,
                                      const std::string &outputName = OUTPUT_NAME,
                                      const std::string &spinState = SPIN_STATE,
                                      const std::string &outputFitParameters = "",
                                      const std::string &outputFitCurves = "");

template <typename T = MatrixWorkspace>
IAlgorithm_sptr prepareHeTimeAlgorithm(const std::shared_ptr<T> &inputWorkspace, const std::string &refTimeStamp = "",
                                       const std::shared_ptr<T> &referenceWorkspace = nullptr) {
  const auto heAlgorithm = AlgorithmManager::Instance().create("HeliumAnalyserEfficiencyTime");
  heAlgorithm->initialize();
  heAlgorithm->setProperty("InputWorkspace", inputWorkspace);
  if (referenceWorkspace) {
    heAlgorithm->setProperty("ReferenceWorkspace", referenceWorkspace);
  }
  heAlgorithm->setProperty("ReferenceTimeStamp", refTimeStamp);
  heAlgorithm->setProperty("OutputWorkspace", OUTPUT_NAME);

  return heAlgorithm;
}

// TimeDifference is a python algorithm. This is a basic mock for running the tests.
class TimeDifference final : public Algorithm {
public:
  const std::string name() const override { return "TimeDifference"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "TimeDifference Mock Algorithm"; }

private:
  void init() override;
  void exec() override;
};
} // namespace HeAnalyserTest
