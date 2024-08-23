// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/PeakParameterHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/cow_ptr.h"

#include <utility>

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
} // namespace HistogramData

namespace Algorithms {

namespace FitPeaksAlgorithm {
struct FitFunction {
  API::IPeakFunction_sptr peakfunction;
  API::IBackgroundFunction_sptr bkgdfunction;
};

class PeakFitResult {
public:
  PeakFitResult(size_t num_peaks, size_t num_params);
  double getPeakPosition(size_t ipeak) const;
  double getCost(size_t ipeak) const;
  size_t getNumberParameters() const;
  size_t getNumberPeaks() const;
  double getParameterValue(size_t ipeak, size_t iparam) const;
  double getParameterError(size_t ipeak, size_t iparam) const;
  void setRecord(size_t ipeak, const double cost, const double peak_position, const FitFunction &fit_functions);
  void setBadRecord(size_t ipeak, const double peak_position);
  void setFunctionParameters(size_t ipeak, std::vector<double> &param_values);

private:
  /// number of function parameters
  size_t m_function_parameters_number;
  // goodness of fitting
  std::vector<double> m_costs;
  // fitted peak positions
  std::vector<double> m_fitted_peak_positions;
  // fitted peak and background parameters
  std::vector<std::vector<double>> m_function_parameters_vector;
  /// fitted peak and background parameters' fitting error
  std::vector<std::vector<double>> m_function_errors_vector;
};

class PeakFitPreCheckResult {
public:
  PeakFitPreCheckResult()
      : m_submitted_spectrum_peaks{0}, m_submitted_individual_peaks{0}, m_low_count_spectrum{0}, m_out_of_range{0},
        m_low_count_individual{0}, m_not_enough_datapoints{0}, m_low_snr{0} {}
  PeakFitPreCheckResult &operator+=(const PeakFitPreCheckResult &another);

public:
  void setNumberOfSubmittedSpectrumPeaks(const size_t n);
  void setNumberOfSubmittedIndividualPeaks(const size_t n);
  void setNumberOfSpectrumPeaksWithLowCount(const size_t n);
  void setNumberOfOutOfRangePeaks(const size_t n);
  void setNumberOfIndividualPeaksWithLowCount(const size_t n);
  void setNumberOfPeaksWithNotEnoughDataPoints(const size_t n);
  void setNumberOfPeaksWithLowSignalToNoise(const size_t n);
  bool isIndividualPeakRejected() const;
  std::string getReport() const;

private:
  // number of peaks submitted for spectrum fitting
  size_t m_submitted_spectrum_peaks;
  // number of peaks submitted for individual fitting. Since some spectra might fail a pre-check, not all peaks might
  // make it to the individual fitting
  size_t m_submitted_individual_peaks;
  // number of peaks rejected as a whole spectrum due to its low signal count
  size_t m_low_count_spectrum;
  // number of peaks rejected individually because their predicted position is out of range
  size_t m_out_of_range;
  // number of peaks rejected individually due to low signal count
  size_t m_low_count_individual;
  // number of peask rejected due to not enough data points
  size_t m_not_enough_datapoints;
  // number of peaks rejected due to low signal-to-noise ratio
  size_t m_low_snr;
};
} // namespace FitPeaksAlgorithm

class MANTID_ALGORITHMS_DLL FitPeaks final : public API::Algorithm {
public:
  FitPeaks();

  /// Algorithm's name
  const std::string name() const override { return "FitPeaks"; }

  /// Summary of algorithms purpose
  const std::string summary() const override { return "Fit one or multiple peaks in all spectra of a given workspace"; }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Optimization"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Init
  void init() override;
  /// Main exec method
  void exec() override;

  /// process inputs (main and child algorithms)
  void processInputs();
  /// peak centers
  void processInputPeakCenters();
  /// process inputs about fitted peak positions' tolerance
  void processInputPeakTolerance();
  /// process inputs for peak and background functions
  void processInputFunctions();
  /// process inputs for peak fitting range
  void processInputFitRanges();

  /// Generate output workspaces
  void generateFittedParametersValueWorkspaces();
  /// main method to create output workspaces
  void generateOutputPeakPositionWS();
  /// Generate workspace for calculated values
  void generateCalculatedPeaksWS();

  /// Convert peak function's parameter names to parameter
  /// index for fast access
  void convertParametersNameToIndex();

  /// suites of method to fit peaks
  std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fitPeaks();

  /// fit peaks in a same spectrum
  void fitSpectrumPeaks(size_t wi, const std::vector<double> &expected_peak_centers,
                        const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result,
                        std::vector<std::vector<double>> &lastGoodPeakParameters,
                        const std::shared_ptr<FitPeaksAlgorithm::PeakFitPreCheckResult> &pre_check_result);

  /// fit background
  bool fitBackground(const size_t &ws_index, const std::pair<double, double> &fit_window,
                     const double &expected_peak_pos, const API::IBackgroundFunction_sptr &bkgd_func);

  // Peak fitting suite
  double fitIndividualPeak(size_t wi, const API::IAlgorithm_sptr &fitter, const double expected_peak_center,
                           const std::pair<double, double> &fitwindow, const bool estimate_peak_width,
                           const API::IPeakFunction_sptr &peakfunction, const API::IBackgroundFunction_sptr &bkgdfunc,
                           const std::shared_ptr<FitPeaksAlgorithm::PeakFitPreCheckResult> &pre_check_result);

  /// Methods to fit functions (general)
  double fitFunctionSD(const API::IAlgorithm_sptr &fit, const API::IPeakFunction_sptr &peak_function,
                       const API::IBackgroundFunction_sptr &bkgd_function, const API::MatrixWorkspace_sptr &dataws,
                       size_t wsindex, const std::pair<double, double> &peak_range, const double &expected_peak_center,
                       bool estimate_peak_width, bool estimate_background);

  double fitFunctionMD(API::IFunction_sptr fit_function, const API::MatrixWorkspace_sptr &dataws, const size_t wsindex,
                       const std::pair<double, double> &vec_xmin, const std::pair<double, double> &vec_xmax);

  /// fit a single peak with high background
  double fitFunctionHighBackground(const API::IAlgorithm_sptr &fit, const std::pair<double, double> &fit_window,
                                   const size_t &ws_index, const double &expected_peak_center, bool observe_peak_shape,
                                   const API::IPeakFunction_sptr &peakfunction,
                                   const API::IBackgroundFunction_sptr &bkgdfunc);

  void setupParameterTableWorkspace(const API::ITableWorkspace_sptr &table_ws,
                                    const std::vector<std::string> &param_names, bool with_chi2);

  /// convert a histogram range to index boundaries
  void histRangeToIndexBounds(size_t iws, const std::pair<double, double> &range, size_t &left_index,
                              size_t &right_index); /// convert a histogram range to index boundaries

  /// calculate how many data points are in a histogram range
  size_t histRangeToDataPointCount(size_t iws, const std::pair<double, double> &range);

  /// get vector X, Y and E in a given range
  void getRangeData(size_t iws, const std::pair<double, double> &range, std::vector<double> &vec_x,
                    std::vector<double> &vec_y, std::vector<double> &vec_e);

  /// sum up all counts in histogram
  double numberCounts(size_t iws);

  /// sum up all counts in histogram range
  double numberCounts(size_t iws, const std::pair<double, double> &range);

  /// calculate signal-to-noise ratio in histogram range
  double calculateSignalToNoiseRatio(size_t iws, const std::pair<double, double> &range,
                                     const API::IBackgroundFunction_sptr &bkgd_function);

  API::MatrixWorkspace_sptr createMatrixWorkspace(const std::vector<double> &vec_x, const std::vector<double> &vec_y,
                                                  const std::vector<double> &vec_e);

  bool decideToEstimatePeakParams(const bool firstPeakInSpectrum, const API::IPeakFunction_sptr &peak_function);

  /// Process the result from fitting a single peak
  bool processSinglePeakFitResult(size_t wsindex, size_t peakindex, const double cost,
                                  const std::vector<double> &expected_peak_positions,
                                  const FitPeaksAlgorithm::FitFunction &fitfunction,
                                  const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result);

  /// calculate peak+background for fitted
  void calculateFittedPeaks(const std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> &fit_results);

  /// Get the parameter name for peak height (I or height or etc)
  std::string getPeakHeightParameterName(const API::IPeakFunction_const_sptr &peak_function);

  /// Set the workspaces and etc to output properties
  void processOutputs(std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fit_result_vec);

  /// Write result of peak fit per spectrum to output analysis workspaces
  void writeFitResult(size_t wi, const std::vector<double> &expected_positions,
                      const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result);

  /// check whether FitPeaks supports observation on a certain peak profile's
  /// parameters (width!)
  bool isObservablePeakProfile(const std::string &peakprofile);

  // log a message disregarding the current logging offset
  void logNoOffset(const size_t &priority, const std::string &msg);

  //------- Workspaces-------------------------------------
  /// mandatory input and output workspaces
  API::MatrixWorkspace_sptr m_inputMatrixWS;
  bool m_inputIsDSpace;
  /// event workspace for input
  DataObjects::EventWorkspace_const_sptr m_inputEventWS; // cast from m_inputWS
  /// output workspace for peak positions
  API::MatrixWorkspace_sptr m_outputPeakPositionWorkspace; // output workspace for peak positions
  /// output analysis workspaces
  /// table workspace for fitted parameters
  API::ITableWorkspace_sptr m_fittedParamTable;
  /// table workspace for fitted parameters' fitting error. This is optional
  API::ITableWorkspace_sptr m_fitErrorTable;
  /// flag to show that the pamarameters in table are raw parameters or
  /// effective parameters
  bool m_rawPeaksTable;
  /// matrix workspace contained calcalated peaks+background from fitted result
  /// it has same number of spectra of input workspace even if only part of
  /// spectra to have peaks to fit
  API::MatrixWorkspace_sptr m_fittedPeakWS;

  //-------- Functions ------------------------------------------------------
  /// Peak profile name
  API::IPeakFunction_sptr m_peakFunction;
  /// Background function
  API::IBackgroundFunction_sptr m_bkgdFunction;
  /// Linear background function for high background fitting
  API::IBackgroundFunction_sptr m_linearBackgroundFunction;

  /// Minimzer
  std::string m_minimizer;
  /// Cost function
  std::string m_costFunction;
  /// Fit from right or left
  bool m_fitPeaksFromRight;
  /// Fit iterations
  int m_fitIterations;

  //-------- Input param init values --------------------------------
  /// input starting parameters' indexes in peak function
  std::vector<size_t> m_initParamIndexes;

  /// Designed peak positions and tolerance
  std::vector<double> m_peakCenters;
  API::MatrixWorkspace_const_sptr m_peakCenterWorkspace;
  /// the number of peaks to fit in all spectra
  std::size_t m_numPeaksToFit;
  bool m_uniformPeakPositions;

  std::function<std::vector<double>(std::size_t const &)> m_getExpectedPeakPositions;
  std::function<std::pair<double, double>(std::size_t const &, std::size_t const &)> m_getPeakFitWindow;
  void checkWorkspaceIndices(std::size_t const &);
  void checkPeakIndices(std::size_t const &, std::size_t const &);
  void checkPeakWindowEdgeOrder(double const &, double const &);

  /// flag to estimate peak width from
  double m_peakWidthPercentage;

  //--------- Fitting range -----------------------------------------
  /// start index
  std::size_t m_startWorkspaceIndex;
  /// stop index (workspace index of the last spectrum included)
  std::size_t m_stopWorkspaceIndex;
  /// total number of spectra to be fit
  std::size_t m_numSpectraToFit;
  /// tolerances for fitting peak positions
  std::vector<double> m_peakPosTolerances;

  /// Flag for observing peak width: there are 3 states (1) no estimation (2)
  /// from 'observation' (3) calculated from instrument resolution
  Algorithms::PeakParameterHelper::EstimatePeakWidth m_peakWidthEstimateApproach;
  bool m_constrainPeaksPosition;

  /// peak windows
  std::vector<std::vector<double>> m_peakWindowVector;
  API::MatrixWorkspace_const_sptr m_peakWindowWorkspace;

  /// input peak parameters' names
  std::vector<std::string> m_peakParamNames;
  /// input peak parameters' starting values corresponding to above peak
  /// parameter names
  std::vector<double> m_initParamValues;
  /// table workspace for profile parameters' starting value
  API::ITableWorkspace_const_sptr m_profileStartingValueTable;
  /// flag for profile startng value being uniform or not
  bool m_uniformProfileStartingValue;

  // Criteria for fitting peaks
  /// minimum peak height without background and it also serves as the criteria
  /// for observed peak parameter
  double m_minPeakHeight;

  // Criteria for rejecting non-peaks or weak peaks from fitting
  double m_minSignalToNoiseRatio;
  double m_minPeakTotalCount;

  /// flag for high background
  bool m_highBackground;

  //----- Result criterias ---------------
  /// peak positon tolerance case b, c and d
  bool m_peakPosTolCase234;
};

} // namespace Algorithms
} // namespace Mantid
