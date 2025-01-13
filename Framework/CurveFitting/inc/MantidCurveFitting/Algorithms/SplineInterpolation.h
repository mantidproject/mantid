// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/CubicSpline.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** Takes two workspaces as input. One contain a set of points which define a
  spline,
  and one which contains a number of spectra to be interpolated against spline.

  Produces an output workspace containing the interpolated points

  Optionally the algorithm will also produce a grouped workspace of derivatives
  of up to order 2
  for each of the interpolated points.

  @author Samuel Jackson, STFC
  @date 25/07/2013
*/
class MANTID_CURVEFITTING_DLL SplineInterpolation final : public API::Algorithm {
public:
  SplineInterpolation();

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Fit", "SplineBackground", "SplineSmoothing"}; }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  /// CubicSpline member used to perform interpolation
  std::shared_ptr<Functions::CubicSpline> m_cspline;

  /// setup an output workspace using meta data from inws and taking a number of
  /// spectra
  API::MatrixWorkspace_sptr setupOutputWorkspace(const API::MatrixWorkspace_sptr &mws,
                                                 const API::MatrixWorkspace_sptr &iws) const;

  /// convert a binned workspace to point data using ConvertToPointData
  API::MatrixWorkspace_sptr convertBinnedData(API::MatrixWorkspace_sptr workspace);

  /// set the points that define the spline used for interpolation of a
  /// workspace
  void setInterpolationPoints(const API::MatrixWorkspace_const_sptr &inputWorkspace, const size_t row) const;

  /// Calculate the interpolation of the input workspace against the spline and
  /// store it in outputWorkspace
  void calculateSpline(const API::MatrixWorkspace_const_sptr &inputWorkspace,
                       const API::MatrixWorkspace_sptr &outputWorkspace, const size_t row) const;

  /// Calculate the derivatives of the input workspace from the spline.
  void calculateDerivatives(const API::MatrixWorkspace_const_sptr &inputWorkspace,
                            const API::MatrixWorkspace_sptr &outputWorkspace, const size_t order) const;

  /// Find the interpolation range
  std::pair<size_t, size_t> findInterpolationRange(const API::MatrixWorkspace_const_sptr &iwspt,
                                                   const API::MatrixWorkspace_sptr &mwspt, const size_t row);

  /// Extrapolates flat for the points outside the x-range
  void extrapolateFlat(const API::MatrixWorkspace_sptr &ows, const API::MatrixWorkspace_const_sptr &iwspt,
                       const size_t row, const std::pair<size_t, size_t> &indices, const bool doDerivs,
                       std::vector<API::MatrixWorkspace_sptr> &derivs) const;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
