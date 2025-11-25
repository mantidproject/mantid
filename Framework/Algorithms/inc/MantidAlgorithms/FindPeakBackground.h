// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {

namespace HistogramData {
class HistogramX;
class HistogramY;
} // namespace HistogramData

namespace Algorithms {

/** FindPeakBackground : Calculate Zscore for a Matrix Workspace
 */
class MANTID_ALGORITHMS_DLL FindPeakBackground : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FindPeakBackground"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Separates background from signal for spectra of a workspace."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Fit"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Utility\\Calculation"; }

  /// set sigma constant
  void setSigma(const double &sigma);

  /// set background order
  void setBackgroundOrder(size_t order);

  /// set fit window
  void setFitWindow(const std::vector<double> &window);

  /// find fit window's data point index
  void findWindowIndex(const HistogramData::Histogram &histogram, size_t &l0, size_t &n);

  /// main method to calculate background
  int findBackground(const HistogramData::Histogram &histogram, const size_t &l0, const size_t &n,
                     std::vector<size_t> &peak_min_max_indexes, std::vector<double> &bkgd3);

private:
  std::string m_backgroundType; //< The type of background to fit

  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;
  double moment4(const MantidVec &X, const size_t n, const double mean) const;
  void estimateBackground(const HistogramData::Histogram &histogram, const size_t i_min, const size_t i_max,
                          const size_t p_min, const size_t p_max, const bool hasPeak, double &out_bg0, double &out_bg1,
                          double &out_bg2);

  /// process inputs
  void processInputProperties();

  /// create output workspace
  void createOutputWorkspaces();

  // Histogram cannot be defined due to lack of default constructor. shared_ptr
  // will do the copy

  /// fit window
  std::vector<double> m_vecFitWindows;
  /// background order: 0 for flat, 1 for linear, 2 for quadratic
  size_t m_backgroundOrder;
  /// constant sigma
  double m_sigmaConstant;
  /// output workspace (table of result)
  API::ITableWorkspace_sptr m_outPeakTableWS;
  /// Input workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// workspace index
  size_t m_inputWSIndex;

  struct cont_peak {
    size_t start;
    size_t stop;
    double maxY;
  };
  struct by_len {
    bool operator()(cont_peak const &a, cont_peak const &b) { return a.maxY > b.maxY; }
  };
};

} // namespace Algorithms
} // namespace Mantid
