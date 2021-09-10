// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidCurveFitting/DllConfig.h"

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** SplineBackground

    @author Roman Tolchenov
    @date 09/10/2009
 */
class MANTID_CURVEFITTING_DLL SplineBackground : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SplineBackground"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Fit", "SplineInterpolation", "SplineSmoothing"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Optimization;CorrectionFunctions\\BackgroundCorrections"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Fit spectra background using b-splines."; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Adds data from the workspace to the GSL vectors for later processing
  void addWsDataToSpline(const API::MatrixWorkspace *ws, const size_t specNum, int expectedNumBins);

  /// Allocates various pointers used within GSL
  void allocateBSplinePointers(int numBins, int ncoeffs);

  /// Calculates the bin weight using the error values in the WS
  double calculateBinWeight(double errValue);

  /// Deallocates various pointers within GSL
  void freeBSplinePointers();

  /// Calculates the number on unmasked bins to process
  size_t calculateNumBinsToProcess(const API::MatrixWorkspace *ws);

  /// Gets the values from the fitted GSL, and creates a clone of input
  /// workspace with new values
  API::MatrixWorkspace_sptr saveSplineOutput(const API::MatrixWorkspace_sptr &ws, const size_t spec);

  /// Sets up the splines for later fitting
  void setupSpline(double xMin, double xMax, int numBins, int ncoeff);

  /// Struct holding various pointers required by GSL
  struct bSplinePointers {
    gsl_bspline_workspace *splineToProcess{nullptr};
    gsl_vector *inputSplineWs{nullptr};
    gsl_vector *xData{nullptr}, *yData{nullptr};
    gsl_vector *coefficients{nullptr}, *binWeights{nullptr};
    gsl_matrix *fittedWs{nullptr}, *covariance{nullptr};
    gsl_multifit_linear_workspace *weightedLinearFitWs{nullptr};
  };

  bSplinePointers m_splinePointers{};
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
