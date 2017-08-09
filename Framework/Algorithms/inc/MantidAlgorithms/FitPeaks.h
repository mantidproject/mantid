#ifndef MANTID_ALGORITHMS_FITPEAKS_H_
#define MANTID_ALGORITHMS_FITPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
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

  void fitSpectraPeaks(size_t wi);

  double fitSinglePeak(API::IFunction_sptr fitfunc,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax);

  void estimateLinearBackground(size_t wi, double left_window_boundary,
                                double right_window_boundary, double &bkgd_a1,
                                double &bkgd_a0);

  double findMaxValue(size_t wi, double left_window_boundary,
                      double right_window_boundary, double b1, double b0);

  void generateOutputWorkspaces();

  void setOutputProperties();

  API::MatrixWorkspace_const_sptr m_inputWS;

  std::vector<double> m_peakCenters;
  std::vector<double> m_peakWindowLeft;
  std::vector<double> m_peakWindowRight;

  std::vector<double> m_initParamValues;

  size_t m_numPeaksToFit;
  double m_minPeakMaxValue;

  API::MatrixWorkspace_sptr m_peakPosWS;  // output workspace
  API::MatrixWorkspace_sptr m_peakParamsWS;  // output workspace for all peak parameters

  size_t m_startWorkspaceIndex;
  size_t m_stopWorkspaceIndex;

};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FITPEAKS_H_ */
