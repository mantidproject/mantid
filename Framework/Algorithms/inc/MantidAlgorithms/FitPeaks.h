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

  void fitPeaks();

  void fitSpectrumPeaks(size_t wi, std::vector<double> &peak_pos,
                       std::vector<std::vector<double>> &peak_params,
                       std::vector<double> &peak_chi2_vec,
                       std::vector<std::vector<double>> &fitted_functions,
                       std::vector<std::vector<double>> &fitted_peak_windows);

  double fitSinglePeak(size_t wsindex, size_t peakindex,
                       const std::vector<double> &init_peak_values,
                       const std::vector<double> &init_bkgd_values,
                       const std::vector<double> &fit_window,
                       const std::vector<double> &peak_range,
                       std::vector<double> &fitted_params_values,
                       std::vector<double> &fitted_params_errors,
                       std::vector<double> &fitted_window,
                       std::vector<double> &fitted_data);

  void estimateLinearBackground(size_t wi, double left_window_boundary,
                                double right_window_boundary, double &bkgd_a1,
                                double &bkgd_a0);

  double findMaxValue(size_t wi, double left_window_boundary,
                      double right_window_boundary, double b1, double b0,
                      double &peak_center, double &max_value);

  std::vector<size_t> getRange(size_t wi,
                               const std::vector<double> &peak_window);

  void generateOutputWorkspaces();

  double processFitResult(DataObjects::TableWorkspace_sptr param_table,
                          std::vector<double> &param_values,
                          std::vector<double> &param_errors);

  void setOutputProperties();

  // Peak fitting suite
  double FitIndividualPeak();

  /// Methods to fit functions (general)
  double fitFunctionSD(API::IAlgorithm_sptr fit, API::IFunction_sptr fitfunc,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax);

  double fitFunctionMD(boost::shared_ptr<API::MultiDomainFunction> mdfunction,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       std::vector<double> &vec_xmin,
                       std::vector<double> &vec_xmax);

  // ------------------------

  API::MatrixWorkspace_const_sptr m_inputWS;
  DataObjects::EventWorkspace_const_sptr m_eventWS;
  API::MatrixWorkspace_const_sptr m_eventNumberWS;

  /// Peak profile name
  API::IPeakFunction_sptr m_peakFunction;
  /// Background function
  API::IBackgroundFunction_sptr m_bkgdFunc;

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

  double m_minPeakMaxValue;

  API::MatrixWorkspace_sptr m_peakPosWS; // output workspace
  API::MatrixWorkspace_sptr
      m_peakParamsWS; // output workspace for all peak parameters
  API::MatrixWorkspace_sptr m_fittedPeakWS;

  size_t m_startWorkspaceIndex;
  size_t m_stopWorkspaceIndex;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FITPEAKS_H_ */
