// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/BSpline.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** Takes a 2D workspace and produces an output workspace containing a smoothed
  version of the data by selecting
  a number of points to define a spline for each histogram in the workspace.

  @author Samuel Jackson, STFC
  @date 24/07/2013
*/
class MANTID_CURVEFITTING_DLL SplineSmoothing final : public API::Algorithm {
public:
  SplineSmoothing();

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Fit", "SplineInterpolation", "SplineBackground"}; }
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Smooths a set of spectra using a cubic spline. Optionally, this "
           "algorithm can also calculate derivatives up to order 2 as a side "
           "product";
  }

private:
  /// number of smoothing points to start with
  const int M_START_SMOOTH_POINTS;

  // Overridden methods
  void init() override;
  void exec() override;

  /// smooth a single spectrum of the workspace
  void smoothSpectrum(const int index);

  /// calculate derivatives for a single spectrum
  void calculateSpectrumDerivatives(const int index, const int order);

  /// setup an output workspace using meta data from inws and taking a number of
  /// spectra
  API::MatrixWorkspace_sptr setupOutputWorkspace(const API::MatrixWorkspace_sptr &inws, const int size) const;

  /// Handle converting point data back to histograms
  void convertToHistogram();

  /// choose points to define a spline and smooth the data
  void selectSmoothingPoints(const API::MatrixWorkspace &inputWorkspace, const size_t row);

  /// calculate the spline based on the smoothing points chosen
  void calculateSmoothing(const API::MatrixWorkspace &inputWorkspace, API::MatrixWorkspace &outputWorkspace,
                          const size_t row) const;

  /// calculate the derivatives for a set of points on the spline
  void calculateDerivatives(const API::MatrixWorkspace &inputWorkspace, API::MatrixWorkspace &outputWorkspace,
                            const int order, const size_t row) const;

  /// add a set of smoothing points to the spline
  void addSmoothingPoints(const std::set<int> &points, const double *xs, const double *ys) const;

  /// check if the difference between smoothing points and data points is within
  /// a certain error bound
  bool checkSmoothingAccuracy(const int start, const int end, const double *ys, const double *ysmooth) const;

  /// Use an existing fit function to tidy smoothing
  void performAdditionalFitting(const API::MatrixWorkspace_sptr &ws, const int row);

  /// Converts histogram data to point data later processing
  /// convert a binned workspace to point data. Uses mean of the bins as point
  API::MatrixWorkspace_sptr convertBinnedData(API::MatrixWorkspace_sptr workspace);

  /// CubicSpline member used to perform smoothing
  std::shared_ptr<Functions::BSpline> m_cspline;
  /// pointer to the input workspace
  API::MatrixWorkspace_sptr m_inputWorkspace;
  /// pointer to the input workspace converted to point data
  API::MatrixWorkspace_sptr m_inputWorkspacePointData;
  /// pointer to the output workspace group of derivatives
  API::WorkspaceGroup_sptr m_derivativeWorkspaceGroup;
  /// pointer to the smoothed output workspace
  API::MatrixWorkspace_sptr m_outputWorkspace;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
