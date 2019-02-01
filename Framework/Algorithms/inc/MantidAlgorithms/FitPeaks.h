// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_FITPEAKS_H_
#define MANTID_ALGORITHMS_FITPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

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
  void setRecord(size_t ipeak, const double cost, const double peak_position,
                 const FitFunction fit_functions);
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
} // namespace FitPeaksAlgorithm

class DLLExport FitPeaks : public API::Algorithm {
public:
  FitPeaks();

  /// Algorithm's name
  const std::string name() const override { return "FitPeaks"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fit one or multiple peaks in all spectra of a given workspace";
  }

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

  /// methods to retrieve fit range and peak centers
  std::vector<double> getExpectedPeakPositions(size_t wi);
  std::pair<double, double> getPeakFitWindow(size_t wi, size_t ipeak);

  enum EstimatePeakWidth { NoEstimation, Observation, InstrumentResolution };
  enum PeakFitResult { NOSIGNAL, LOWPEAK, OUTOFBOUND, GOOD };

  /// suites of method to fit peaks
  std::vector<boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fitPeaks();

  /// fit peaks in a same spectrum
  void fitSpectrumPeaks(
      size_t wi, const std::vector<double> &expected_peak_centers,
      boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result);

  /// fit background
  bool fitBackground(const size_t &ws_index,
                     const std::pair<double, double> &fit_window,
                     const double &expected_peak_pos,
                     API::IBackgroundFunction_sptr bkgd_func);

  // Peak fitting suite
  double fitIndividualPeak(size_t wi, API::IAlgorithm_sptr fitter,
                           const double expected_peak_center,
                           const std::pair<double, double> &fitwindow,
                           const bool observe_peak_width,
                           API::IPeakFunction_sptr peakfunction,
                           API::IBackgroundFunction_sptr bkgdfunc);

  /// Methods to fit functions (general)
  double fitFunctionSD(API::IAlgorithm_sptr fit,
                       API::IPeakFunction_sptr peak_function,
                       API::IBackgroundFunction_sptr bkgd_function,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax,
                       const double &expected_peak_center,
                       bool observe_peak_width, bool estimate_background);

  double fitFunctionMD(API::IFunction_sptr fit_function,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       std::vector<double> &vec_xmin,
                       std::vector<double> &vec_xmax);

  /// fit a single peak with high background
  double fitFunctionHighBackground(API::IAlgorithm_sptr fit,
                                   const std::pair<double, double> &fit_window,
                                   const size_t &ws_index,
                                   const double &expected_peak_center,
                                   bool observe_peak_width,
                                   API::IPeakFunction_sptr peakfunction,
                                   API::IBackgroundFunction_sptr bkgdfunc);

  void setupParameterTableWorkspace(API::ITableWorkspace_sptr table_ws,
                                    const std::vector<std::string> &param_names,
                                    bool with_chi2);

  /// get vector X, Y and E in a given range
  void getRangeData(size_t iws, const std::pair<double, double> &fit_window,
                    std::vector<double> &vec_x, std::vector<double> &vec_y,
                    std::vector<double> &vec_e);

  /// Reduce background
  void reduceByBackground(API::IBackgroundFunction_sptr bkgd_func,
                          const std::vector<double> &vec_x,
                          std::vector<double> &vec_y);

  API::MatrixWorkspace_sptr
  createMatrixWorkspace(const std::vector<double> &vec_x,
                        const std::vector<double> &vec_y,
                        const std::vector<double> &vec_e);

  /// Esitmate background by 'observation'
  void estimateBackground(const HistogramData::Histogram &histogram,
                          const std::pair<double, double> &peak_window,
                          API::IBackgroundFunction_sptr bkgd_function);
  /// estimate linear background
  void estimateLinearBackground(const HistogramData::Histogram &histogram,
                                double left_window_boundary,
                                double right_window_boundary, double &bkgd_a0,
                                double &bkgd_a1);

  /// Estimate peak parameters by 'observation'
  int estimatePeakParameters(const HistogramData::Histogram &histogram,
                             const std::pair<double, double> &peak_window,
                             API::IPeakFunction_sptr peakfunction,
                             API::IBackgroundFunction_sptr bkgdfunction,
                             bool observe_peak_width);

  bool decideToEstimatePeakWidth(const bool firstPeakInSpectrum,
                                 API::IPeakFunction_sptr peak_function);

  /// observe peak center
  int observePeakCenter(const HistogramData::Histogram &histogram,
                        API::FunctionValues &bkgd_values, size_t start_index,
                        size_t stop_index, double &peak_center,
                        size_t &peak_center_index, double &peak_height);

  /// Observe peak width
  double observePeakWidth(const HistogramData::Histogram &histogram,
                          API::FunctionValues &bkgd_values, size_t ipeak,
                          size_t istart, size_t istop);

  /// Process the result from fitting a single peak
  void processSinglePeakFitResult(
      size_t wsindex, size_t peakindex, const double cost,
      const std::vector<double> &expected_peak_positions,
      FitPeaksAlgorithm::FitFunction fitfunction,
      boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result);

  /// calculate peak+background for fitted
  void calculateFittedPeaks(
      std::vector<boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult>>
          fit_results);

  /// Get the parameter name for peak height (I or height or etc)
  std::string
  getPeakHeightParameterName(API::IPeakFunction_const_sptr peak_function);

  /// Set the workspaces and etc to output properties
  void processOutputs(
      std::vector<boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult>>
          fit_result_vec);

  /// Write result of peak fit per spectrum to output analysis workspaces
  void writeFitResult(
      size_t wi, const std::vector<double> &expected_positions,
      boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result);

  /// check whether FitPeaks supports observation on a certain peak profile's
  /// parameters (width!)
  bool isObservablePeakProfile(const std::string &peakprofile);

  //------- Workspaces-------------------------------------
  /// mandatory input and output workspaces
  API::MatrixWorkspace_sptr m_inputMatrixWS;
  bool m_inputIsDSpace;
  /// event workspace for input
  DataObjects::EventWorkspace_const_sptr m_inputEventWS; // cast from m_inputWS
  /// output workspace for peak positions
  API::MatrixWorkspace_sptr
      m_outputPeakPositionWorkspace; // output workspace for peak positions
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
  size_t m_numPeaksToFit;
  bool m_uniformPeakPositions;

  /// flag to estimate peak width from
  double m_peakWidthPercentage;

  //--------- Fitting range -----------------------------------------
  /// start index
  size_t m_startWorkspaceIndex;
  /// stop index (workspace index of the last spectrum included)
  size_t m_stopWorkspaceIndex;
  /// flag whether the peak center workspace has only a subset of spectra to fit
  bool m_partialSpectra;
  std::vector<double> m_peakPosTolerances;

  /// Flag for observing peak width: there are 3 states (1) no estimation (2)
  /// from 'observation' (3) calculated from instrument resolution
  EstimatePeakWidth m_peakWidthEstimateApproach;
  bool m_constrainPeaksPosition;

  /// peak windows
  std::vector<std::vector<double>> m_peakWindowVector;
  API::MatrixWorkspace_const_sptr m_peakWindowWorkspace;
  bool m_uniformPeakWindows;
  bool m_partialWindowSpectra;
  /// flag to calcualte peak fit window from instrument resolution
  bool m_calculateWindowInstrument;

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

  /// flag for high background
  bool m_highBackground;
  double m_bkgdSimga; // TODO - add to properties

  //----- Result criterias ---------------
  /// peak positon tolerance case b, c and d
  bool m_peakPosTolCase234;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FITPEAKS_H_ */
