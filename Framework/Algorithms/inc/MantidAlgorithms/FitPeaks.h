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
}

namespace Algorithms {

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

private:
  /// Init
  void init() override;
  /// Main exec method
  void exec() override;

  /// process inputs (main and child algorithms)
  void processInputs();
  /// peak centers
  void ProcessInputPeakCenters();
  /// fitting tolerance
  void ProcessInputPeakTolerance();
  void processInputFunctions();
  void processInputFitRanges();

  /// Generate output workspaces
  void GenerateFittedParametersValueWorkspace();
  /// main method to create output workspaces
  void GenerateOutputPeakPositionWS();
  /// Generate workspace for calculated values
  void GenerateCalculatedPeaksWS();

  /// Convert peak function's parameter names to parameter
  /// index for fast access
  void ConvertParametersNameToIndex();

  /// methods to retrieve fit range and peak centers
  std::vector<double> GetExpectedPeakPositions(size_t wi);
  std::pair<double, double> GetPeakFitWindow(size_t wi, size_t ipeak);

  enum EstimatePeakWidth { NoEstimation, Observation, InstrumentResolution };

  /// suites of method to fit peaks
  void fitPeaks();

  /// fit peaks in a same spectrum
  void
  fitSpectrumPeaks(size_t wi, const std::vector<double> &expected_peak_centers,
                   std::vector<double> &fitted_peak_centers,
                   std::vector<std::vector<double>> &fitted_function_parameters,
                   std::vector<double> *peak_chi2_vec);

  /// fit background
  bool FitBackground(API::IAlgorithm_sptr md_fitter, const size_t &ws_index,
                     const std::pair<double, double> &fit_window,
                     const double &expected_peak_pos,
                     API::IBackgroundFunction_sptr bkgd_func);

  // Peak fitting suite
  double FitIndividualPeak(size_t wi, API::IAlgorithm_sptr fitter,
                           API::IAlgorithm_sptr bkgd_fitter,
                           const double &expected_peak_center,
                           const std::pair<double, double> &fitwindow,
                           const bool &high, const bool &observe_peak_width,
                           API::IPeakFunction_sptr peakfunction,
                           API::IBackgroundFunction_sptr bkgdfunc);

  /// Methods to fit functions (general)
  double FitFunctionSD(API::IAlgorithm_sptr fit,
                       API::IPeakFunction_sptr peak_function,
                       API::IBackgroundFunction_sptr bkgd_function,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax, bool observe_peak_width,
                       bool estimate_background);

  double FitFunctionMD(API::IAlgorithm_sptr fit,
                       API::IFunction_sptr fit_function,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       std::vector<double> &vec_xmin,
                       std::vector<double> &vec_xmax);

  /// fit a single peak with high background
  double FitFunctionHighBackground(
      API::IAlgorithm_sptr fit, API::IAlgorithm_sptr bkgd_fitter,
      const std::pair<double, double> &fit_window, const size_t &ws_index,
      const double &expected_peak_center, API::IPeakFunction_sptr peakfunction,
      API::IBackgroundFunction_sptr bkgdfunc, bool observe_peak_width);

  /// get vector X, Y and E in a given range
  void GetRangeData(size_t iws, const std::pair<double, double> &fit_window,
                    std::vector<double> *vec_x, std::vector<double> *vec_y,
                    std::vector<double> *vec_e);

  /// Reduce background
  void ReduceBackground(API::IBackgroundFunction_sptr bkgd_func,
                        const std::vector<double> &vec_x,
                        std::vector<double> *vec_y, std::vector<double> *vec_e);

  API::MatrixWorkspace_sptr
  CreateMatrixWorkspace(const std::vector<double> &vec_x,
                        const std::vector<double> &vec_y,
                        const std::vector<double> &vec_e);

  /// Esitmate background by 'observation'
  void EstimateBackground(API::MatrixWorkspace_sptr dataws, size_t wi,
                          const std::pair<double, double> &peak_window,
                          API::IBackgroundFunction_sptr bkgd_function);
  /// estimate linear background
  void estimateLinearBackground(API::MatrixWorkspace_sptr dataws, size_t wi,
                                double left_window_boundary,
                                double right_window_boundary, double &bkgd_a1,
                                double &bkgd_a0);

  /// Estimate peak parameters by 'observation'
  int EstimatePeakParameters(API::MatrixWorkspace_sptr dataws, size_t wi,
                             const std::pair<double, double> &peak_window,
                             API::IPeakFunction_sptr peakfunction,
                             API::IBackgroundFunction_sptr bkgdfunction,
                             bool observe_peak_width);

  bool DecideToEstimatePeakWidth(size_t peak_index,
                                 API::IPeakFunction_sptr peak_function);

  /// observe peak center
  int ObservePeakCenter(HistogramData::HistogramX &vector_x,
                        HistogramData::HistogramY &vector_y,
                        API::FunctionValues &bkgd_values, size_t start_index,
                        size_t stop_index, double *peak_center,
                        size_t *peak_center_index, double *peak_intensity);

  /// Observe peak width
  double ObservePeakWidth(HistogramData::HistogramX &vector_x,
                          HistogramData::HistogramY &vector_y,
                          API::FunctionValues &bkgd_values, double peak_height,
                          size_t ipeak, size_t istart, size_t istop);

  /// Process the result from fitting a single peak
  void ProcessSinglePeakFitResult(
      size_t wsindex, size_t peakindex,
      const std::vector<double> &expected_peak_positions,
      API::IPeakFunction_sptr peakfunction,
      API::IBackgroundFunction_sptr bkgdfunction, double cost,
      std::vector<double> &fitted_peak_positions,
      std::vector<std::vector<double>> &function_parameters_vector,
      std::vector<double> *peak_chi2_vec);

  /// calculate peak+background for fitted
  void CalculateFittedPeaks();

  /// Get index of value X in a spectrum's X histogram
  size_t GetXIndex(size_t wi, double x);

  /// Get the parameter name for peak height (I or height or etc)
  std::string
  GetPeakHeightParameterName(API::IPeakFunction_const_sptr peak_function);

  /// Set the workspaces and etc to output properties
  void ProcessOutputs();

  /// Write result of peak fit per spectrum to output analysis workspaces
  void writeFitResult(size_t wi, const std::vector<double> &expected_positions,
                      std::vector<double> &fitted_positions,
                      std::vector<std::vector<double>> &peak_parameters,
                      std::vector<double> &peak_chi2_vec, bool noevents);

  //  /// set peak positions tolerance
  //  void setPeakPosTolerance(const std::vector<double> &peak_pos_tolerances);

  //------- Workspaces-------------------------------------
  /// mandatory input and output workspaces
  API::MatrixWorkspace_sptr m_inputMatrixWS;
  bool is_d_space_;
  /// event workspace for input
  DataObjects::EventWorkspace_const_sptr m_inputEventWS; // cast from m_inputWS
  /// output workspace for peak positions
  API::MatrixWorkspace_sptr
      output_peak_position_workspace_; // output workspace for peak positions
  /// matrix workspace contains number of events of each spectrum
  API::MatrixWorkspace_const_sptr m_eventNumberWS;
  /// optional output analysis workspaces
  /// table workspace for fitted parameters
  API::ITableWorkspace_sptr m_fittedParamTable;
  /// matrix workspace contained calcalated peaks+background from fitted result
  /// it has same number of spectra of input workspace even if only part of
  /// spectra
  /// to have peaks to fit
  API::MatrixWorkspace_sptr m_fittedPeakWS;

  //-------- Functions ------------------------------------------------------
  /// Peak profile name
  API::IPeakFunction_sptr m_peakFunction;
  /// Background function
  API::IBackgroundFunction_sptr m_bkgdFunction;
  /// Linear background function for high backgroun fitting
  API::IBackgroundFunction_sptr linear_background_function_;

  /// Minimzer
  std::string m_minimizer;
  /// Cost function
  std::string m_costFunction;
  /// Fit from right or left
  bool fit_peaks_from_right_;

  //-------- Input param init values --------------------------------
  /// input starting parameters' indexes in peak function
  std::vector<size_t> m_initParamIndexes;

  /// Designed peak positions and tolerance
  std::vector<double> m_peakCenters;
  API::MatrixWorkspace_const_sptr m_peakCenterWorkspace;
  size_t m_numPeaksToFit;
  bool m_uniformPeakPositions;

  /// flag to estimate peak width from
  double m_peakDSpacePercentage;

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
  EstimatePeakWidth peak_width_estimate_approach_;
  bool constrain_peaks_position_;

  /// peak windows
  std::vector<std::vector<double>> m_peakWindowVector;
  API::MatrixWorkspace_const_sptr m_peakWindowWorkspace;
  bool m_uniformPeakWindows;
  bool m_partialWindowSpectra;
  /// flag to calcualte peak fit window from instrument resolution
  bool calculate_window_instrument_;

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

  std::stringstream m_sstream;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FITPEAKS_H_ */
