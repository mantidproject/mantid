#ifndef MANTID_ALGORITHMS_FITPEAKS_H_
#define MANTID_ALGORITHMS_FITPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"
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
  void init() override;
  void exec() override;

  /// process inputs (main and child algorithms)
  void processInputs();
  void processInputPeakCenters();
  void processInputFunctions();
  void processInputFitRanges();

  /// TODO - Implment .. convert peak function's parameter names to parameter
  /// index for fast access
  void convert_parameter_name_to_index();

  /// methods to retrieve fit range and peak centers
  std::vector<double> getExpectedPeakPositions(size_t wi);
  std::pair<double, double> getPeakFitWindow(size_t wi, size_t ipeak);

  /// suites of method to fit peaks
  void fitPeaks();
  void fitSpectrumPeaks(size_t wi, std::vector<double> &peak_pos_vec,
                        std::vector<std::vector<double>> &peak_params,
                        std::vector<double> &peak_chi2_vec,
                        std::vector<std::vector<double>> &fitted_functions,
                        std::vector<std::vector<double>> &fitted_peak_windows);
  // Peak fitting suite
  double fitIndividualPeak(size_t wi, API::IAlgorithm_sptr fitter,
                           API::IFunction_sptr peakbkgdfunc,
                           API::IPeakFunction_sptr peakfunction,
                           API::IBackgroundFunction_sptr bkgdfunc,
                           const std::pair<double, double> &fitwindow,
                           const double &exppeakcenter, const double &postol,
                           const bool high);

  /// Methods to fit functions (general)
  double fitFunctionSD(API::IAlgorithm_sptr fit, API::IFunction_sptr fitfunc,
                       API::MatrixWorkspace_const_sptr dataws, size_t wsindex,
                       double xmin, double xmax);

  double fitFunctionMD(boost::shared_ptr<API::MultiDomainFunction> mdfunction,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       std::vector<double> &vec_xmin,
                       std::vector<double> &vec_xmax);

  void estimateBackground(size_t wi,
                          const std::pair<double, double> &peak_window,
                          API::IBackgroundFunction_sptr bkgd_function);
  int estimatePeakParameters(size_t wi,
                             const std::pair<double, double> &peak_window,
                             API::IPeakFunction_sptr peakfunction,
                             API::IBackgroundFunction_sptr bkgdfunction);

  void reduceBackground(const std::vector<double> &vec_x,
                        const std::vector<double> &vec_y, double &bkgd_a,
                        double &bkgd_b);

  double findMaxValue(size_t wi, const std::pair<double, double> &window,
                      API::IBackgroundFunction_sptr bkgdfunction,
                      size_t &center_index, double &peak_center,
                      double &max_value);

  void processSinglePeakFitResult(
      size_t wi, size_t peakindex, API::IFunction_sptr peakbkgdfunction,
      API::IPeakFunction_sptr peakfunction,
      API::IBackgroundFunction_sptr bkgdfunction, double chi2,
      std::vector<double> &fitted_peak_positions,
      std::vector<std::vector<double>> &peak_params_vector,
      std::vector<double> &peak_chi2_vec);

  /// calculate peak+background for fitted
  void calculateFittedPeaks(API::IFunction_sptr peakbkgdfunction);

  //  std::vector<size_t> getRange(size_t wi,
  //                               const std::vector<double> &peak_window);

  size_t getXIndex(size_t wi, double x);

  void generateOutputWorkspaces();

  //  double processFitResult(DataObjects::TableWorkspace_sptr param_table,
  //                          std::vector<double> &param_values,
  //                          std::vector<double> &param_errors);

  void setOutputProperties();


  /// Write result of peak fit per spectrum to output analysis workspaces
  void writeFitResult(size_t wi, const std::vector<double> &peak_positions,
                      std::vector<std::vector<double>> &peak_parameters,
                      std::vector<std::vector<double>> &fitted_peaks,
                      std::vector<double> &peak_chi2_vec);

  /// estimate linear background
  void estimateLinearBackground(size_t wi, double left_window_boundary,
                                double right_window_boundary, double &bkgd_a1,
                                double &bkgd_a0);

  API::MatrixWorkspace_const_sptr m_inputWS;
  DataObjects::EventWorkspace_const_sptr m_eventWS;
  API::MatrixWorkspace_const_sptr m_eventNumberWS;

  /// Peak profile name
  API::IPeakFunction_sptr m_peakFunction;
  /// Background function
  API::IBackgroundFunction_sptr m_bkgdFunction;

  /// Minimzer
  std::string m_minimizer;
  /// Cost function
  std::string m_costFunction;

  /// Designed peak positions and tolerance
  std::vector<double> m_peakCenters;
  API::MatrixWorkspace_const_sptr m_peakCenterWorkspace;
  bool m_uniformPeakPositions;
  bool m_partialSpectra; // flag whether the peak center workspace has only
                         // spectra to fit
  std::vector<double> m_peakPosTolerances;
  size_t m_numPeaksToFit;

  /// peak windows
  std::vector<std::vector<double>> m_peakWindowVector;
  API::MatrixWorkspace_const_sptr m_peakWindowWorkspace;
  bool m_uniformPeakWindows;
  bool m_partialWindowSpectra;

  /// input peak parameters' names
  std::vector<std::string> m_peakParamNames;
  /// input peak parameters' starting values corresponding to above peak parameter names
  std::vector<double> m_initParamValues;
  /// table workspace for profile parameters' starting value
  API::ITableWorkspace_const_sptr m_profileStartingValueTable;
  /// flag for profile startng value being uniform or not
  bool m_uniformProfileStartingValue;

  // Peak information
  double m_minHeight; // TODO - Implement in init() and processInput()
  double m_minPeakMaxValue;

  API::MatrixWorkspace_sptr m_peakPosWS; // output workspace
  API::MatrixWorkspace_sptr
      m_peakParamsWS; // output workspace for all peak parameters
  API::MatrixWorkspace_sptr m_fittedPeakWS;

  size_t m_startWorkspaceIndex;
  size_t m_stopWorkspaceIndex;

  /// flag for high background
  bool m_highBackground;
  double m_bkgdSimga; // TODO - add to properties

  /// global message
  std::stringstream m_sstream;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FITPEAKS_H_ */
