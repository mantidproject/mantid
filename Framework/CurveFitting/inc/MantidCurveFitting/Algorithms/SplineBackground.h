#ifndef MANTID_CURVEFITTING_SPLINEBACKGROUND_H_
#define MANTID_CURVEFITTING_SPLINEBACKGROUND_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"

#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** SplineBackground

    @author Roman Tolchenov
    @date 09/10/2009

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport SplineBackground : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SplineBackground"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Fit", "SplineInterpolation", "SplineSmoothing"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Optimization;CorrectionFunctions\\BackgroundCorrections";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fit spectra background using b-splines.";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Adds data from the workspace to the GSL vectors for later processing
  void addWsDataToSpline(const API::MatrixWorkspace *ws, const size_t specNum,
                         int expectedNumBins);

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
  API::MatrixWorkspace_sptr saveSplineOutput(const API::MatrixWorkspace_sptr ws,
                                             const size_t spec);

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

#endif /*MANTID_CURVEFITTING_SPLINEBACKGROUND_H_*/
