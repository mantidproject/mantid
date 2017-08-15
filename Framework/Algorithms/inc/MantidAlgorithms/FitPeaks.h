#ifndef MANTID_ALGORITHMS_FITPEAKS_H_
#define MANTID_ALGORITHMS_FITPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
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

  void processInputs();

  void fitPeaks();

  void fitSpectraPeaks(size_t wi, std::vector<double> &peak_pos,
                       std::vector<std::vector<double>> &peak_params,
                       std::vector<std::vector<double>> &fitted_functions,
                       std::vector<std::vector<double>> &fitted_peak_windows);

  bool fitSinglePeak(size_t wsindex, size_t peakindex,
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

  API::MatrixWorkspace_const_sptr m_inputWS;
  API::MatrixWorkspace_const_sptr m_eventNumberWS;

  std::vector<double> m_peakCenters;
  std::vector<double> m_peakWindowLeft;
  std::vector<double> m_peakWindowRight;

  std::vector<std::vector<double>> m_peakWindows;
  std::vector<std::vector<double>> m_peakRangeVec;

  std::vector<double> m_initParamValues;

  size_t m_numPeaksToFit;
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
