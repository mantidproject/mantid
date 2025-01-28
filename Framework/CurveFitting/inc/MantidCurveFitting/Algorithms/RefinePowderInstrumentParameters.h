// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/BackToBackExponential.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include "MantidCurveFitting/Functions/Polynomial.h"
#include "MantidCurveFitting/Functions/ThermalNeutronDtoTOFFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** RefinePowderInstrumentParameters : Algorithm to refine instrument geometry
  parameters only.
  This algorithm is the second part of the algorithm suite.
  It must use the output from FitPowderDiffPeaks() as the inputs.



  [ASSUMPTIONS]
  1.   CYRSTAL LATTICE IS CORRECT!  AS FOR FITTING INSTRUMENT PARAMETER, IT IS A
  GIVEN VALUE.
*/
class MANTID_CURVEFITTING_DLL RefinePowderInstrumentParameters final : public API::Algorithm,
                                                                       public API::DeprecatedAlgorithm {
public:
  RefinePowderInstrumentParameters();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RefinePowderInstrumentParameters"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Parameters include Dtt1, Dtt1t, Dtt2t, Zero, Zerot. "; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\Fitting"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  // Implement abstract Algorithm methods
  void exec() override;

  //----------------  Processing Input ---------------------
  /// Import instrument parameter from table (workspace)
  void importParametersFromTable(const DataObjects::TableWorkspace_sptr &parameterWS,
                                 std::map<std::string, double> &parameters);

  /// Import the Monte Carlo related parameters from table
  void importMonteCarloParametersFromTable(const DataObjects::TableWorkspace_sptr &tablews,
                                           const std::vector<std::string> &parameternames,
                                           std::vector<double> &stepsizes, std::vector<double> &lowerbounds,
                                           std::vector<double> &upperbounds);

  /// Generate (output) workspace of peak centers
  void genPeakCentersWorkspace(bool montecarlo, size_t numbestfit);

  /// Generate peaks from table (workspace)
  void genPeaksFromTable(const DataObjects::TableWorkspace_sptr &peakparamws);

  //---------------  Processing Output ------------------
  /// Generate (output) table worksspace for instrument parameters
  DataObjects::TableWorkspace_sptr genOutputInstrumentParameterTable();

  /// Generate an output table workspace for N best fitting result
  DataObjects::TableWorkspace_sptr genMCResultTable();

  //--------------- Fit and MC methods -------------------
  /// Fit instrument geometry parameters by ThermalNeutronDtoTOFFunction
  void fitInstrumentParameters();

  /// Calculate function's statistic
  double calculateFunctionStatistic(const API::IFunction_sptr &func, const API::MatrixWorkspace_sptr &dataws,
                                    size_t workspaceindex);

  /// Fit function to data
  bool fitFunction(const API::IFunction_sptr &func, double &gslchi2);

  /// Parse Fit() output parameter workspace
  std::string parseFitParameterWorkspace(API::ITableWorkspace_sptr paramws);

  /// Parse the fitting result
  std::string parseFitResult(API::IAlgorithm_sptr fitalg, double &chi2);

  /// Set up and run a monte carlo simulation to refine the peak parameters
  void refineInstrumentParametersMC(const DataObjects::TableWorkspace_sptr &parameterWS, bool fit2 = false);

  /// Core Monte Carlo random walk on parameter-space
  void doParameterSpaceRandomWalk(std::vector<std::string> &parnames, std::vector<double> &lowerbounds,
                                  std::vector<double> &upperbounds, std::vector<double> &stepsizes, size_t maxsteps,
                                  double stepsizescalefactor, bool fit2);

  /// Get the names of the parameters of D-TOF conversion function
  void getD2TOFFuncParamNames(std::vector<std::string> &parnames);

  /// Calculate the value and chi2
  double calculateD2TOFFunction(const API::IFunction_sptr &func, const API::FunctionDomain1DVector &domain,
                                API::FunctionValues &values, const Mantid::HistogramData::HistogramY &rawY,
                                const Mantid::HistogramData::HistogramE &rawE);

  /// Calculate d-space value from peak's miller index for thermal neutron
  // double calculateDspaceValue(std::vector<int> hkl, double lattice);

  /// Calculate value n for thermal neutron peak profile
  void calculateThermalNeutronSpecial(const API::IFunction_sptr &m_Function,
                                      const Mantid::HistogramData::HistogramX &xVals, std::vector<double> &vec_n);

  //--------------- Class Variables -------------------
  /// Output Workspace containing the dspacing ~ TOF peak positions
  DataObjects::Workspace2D_sptr m_dataWS;

  /// Map for all peaks to fit individually
  std::map<std::vector<int>, Functions::BackToBackExponential_sptr> m_Peaks;

  /// Map for all peaks' error (fitted vs. experimental): [HKL]: Chi^2
  std::map<std::vector<int>, double> m_PeakErrors;

  /// Map for function (instrument parameter)
  std::map<std::string, double> m_FuncParameters;
  /// Map to store the original (input) parameters
  std::map<std::string, double> m_OrigParameters;

  /// Peak function parameter names
  std::vector<std::string> m_PeakFunctionParameterNames;
  /// N sets of the peak parameter values for the best N chi2 for MC.  It is
  /// paired with mPeakFunctionParameterNames
  std::vector<std::pair<double, std::vector<double>>> m_BestMCParameters;
  /// N sets of the peak parameter values for the best N chi2 for MC.  It is
  /// paired with mPeakFunctionParameterNames
  std::vector<std::pair<double, std::vector<double>>> m_BestFitParameters;
  /// N sets of the homemade chi2 and gsl chi2
  std::vector<std::pair<double, double>> m_BestFitChi2s;
  /// Best Chi2 ever
  double m_BestGSLChi2;

  /// Minimum allowed sigma of a peak
  double m_MinSigma;

  /// Minimum number of fitted peaks for refinement
  size_t m_MinNumFittedPeaks;

  /// Maximum number of data stored
  size_t m_MaxNumberStoredParameters;

  /// Modelling function
  Functions::ThermalNeutronDtoTOFFunction_sptr m_Function;
};

/** Formular for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
 */
inline double linearInterpolateX(double x0, double xf, double y0, double yf, double y) {
  double x = ((xf - x0) * y - (xf * y0 - x0 * yf)) / (yf - y0);
  return x;
}

/** Formula for linear interpolation: Y = ( (xf*y0-x0*yf) + x*(yf-y0) )/(xf-x0)
 */
inline double linearInterpolateY(double x0, double xf, double y0, double yf, double x) {
  double y = ((xf * y0 - x0 * yf) + x * (yf - y0)) / (xf - x0);
  return y;
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
