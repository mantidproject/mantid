// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

using namespace Mantid;
using namespace Mantid::API;

namespace PolCorrTestUtils {
static const std::string HE_EFF_TIME_ALG = "HeliumAnalyserEfficiencyTime";
static const std::string HE_EFF_ALG = "HeliumAnalyserEfficiency";

static const std::string REFERENCE_NAME = "reference";
static const std::string INPUT_NAME = "input";
static const std::string GROUP_NAME = "group";
static const std::string OUTPUT_NAME = "groupOut";
static const std::string OUTPUT_TABLE_NAME = "tableOut";
static const std::string OUTPUT_CURVES_NAME = "curvesOut";
static const std::string ANALYSER_EFFICIENCY_WS_NAME = "effAnalyser";
static const std::string X_UNIT = "Wavelength";
static const std::string REF_TIMESTAMP = "2025-07-01T08:00:00";

constexpr double WAV_MIN = 1;
constexpr double WAV_MAX = 8;
constexpr double BIN_WIDTH = 1;
constexpr int N_SPECS = 1;
constexpr double DEFAULT_LIFETIME = 45;
constexpr double DEFAULT_INI_POL = 0.9;
constexpr double DEFAULT_PXD = 12;
constexpr double LAMBDA_CONVERSION_FACTOR = 0.0733;

static const std::string DEFAULT_FUNC_STR = "name=UserFunction, Formula=x*0 + #";
static const std::string EFFICIENCY_FUNC_STR = "name=UserFunction,Formula=0.5 * (1 + tanh(x * #))";
static const std::string UNPOL_FUNC_STR = "name=UserFunction,Formula=exp(- # * x) * cosh(x * #)";
static const std::string SPIN_TEST_FUNC_STR = "name=UserFunction,Formula=0.9 * exp(- x * # )";

static const std::string SPIN_STATE = "11,10,01,00";

constexpr double DELTA = 0.01;

std::string fillFuncStr(const std::vector<double> &number, const std::string &funcStr = DEFAULT_FUNC_STR);

struct TestWorkspaceParameters {
  TestWorkspaceParameters() = default;
  TestWorkspaceParameters(const std::string &name, const std::string &func, const std::string &xUnit = X_UNIT,
                          const int numBanks = 1, const double min = 1, const double max = 8, const double binWidth = 1,
                          const double delay = 0, const std::string &refTimeStamp = REF_TIMESTAMP)

      : testName(name), funcStr(func), xUnit(xUnit), refTimeStamp(refTimeStamp), xMin(min), xMax(max),
        binWidth(binWidth), delay(delay), numBanks(numBanks) {}

  void updateNameAndFunc(const std::string &name, const std::string &func) {
    testName = name;
    funcStr = func;
  }
  void updateSpectra(const int numSpec, const double min, const double max, const double width) {
    numBanks = numSpec;
    xMin = min;
    xMax = max;
    binWidth = width;
  }
  std::string testName{INPUT_NAME};
  std::string funcStr{fillFuncStr({1.0})};
  std::string xUnit{X_UNIT};
  std::string refTimeStamp{REF_TIMESTAMP};
  double xMin{WAV_MIN};
  double xMax{WAV_MAX};
  double binWidth{BIN_WIDTH};
  double delay{0.0};
  int numBanks{N_SPECS};
};

MatrixWorkspace_sptr generateFunctionDefinedWorkspace(const TestWorkspaceParameters &parameters,
                                                      const std::string &name = "", const std::string &func = "");

WorkspaceGroup_sptr groupWorkspaces(const std::string &name, const std::vector<std::string> &wsToGroup);
WorkspaceGroup_sptr groupWorkspaces(const std::string &name, const std::vector<MatrixWorkspace_sptr> &wsToGroup);
WorkspaceGroup_sptr createPolarizedTestGroup(std::string const &outName, const TestWorkspaceParameters &parameters,
                                             const std::vector<double> &amplitudes, const bool isFullPolarized = true);
WorkspaceGroup_sptr createPolarizedTestGroup(std::string const &outName, const TestWorkspaceParameters &parameters,
                                             const std::vector<std::string> &funcs, const bool isFullPolarized = true);
WorkspaceGroup_sptr createPolarizedTestGroup(std::string const &outName, const TestWorkspaceParameters &parameters,
                                             const bool isFullPolarized = true);
MatrixWorkspace_sptr getMatrixWorkspaceFromInput(const std::string &wsName);

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
} // namespace PolCorrTestUtils
