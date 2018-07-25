#ifndef MANTID_ALGORITHMS_MAXENT_H_
#define MANTID_ALGORITHMS_MAXENT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/MaxEnt/MaxentCalculator.h"
#include "MantidAlgorithms/MaxEnt/MaxentCoefficients.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Algorithms {

/** MaxEnt : Entropy maximization algorithm following the approach described in
  the article by J. Skilling and R. K. Bryan: "Maximum entropy image
  reconstruction: general algorithm", Mon. Not. R. astr. Soc. (1984) 211,
  111-124

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport MaxEnt : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override;
  /// Algorithm's version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT", "FFTDerivative", "RealFFT",
            "SassenaFFT", "FFTSmooth"};
  }
  /// Algorithm's category
  const std::string category() const override;
  /// Algorithm's summary
  const std::string summary() const override;

protected:
  /// Validate the input properties
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the algorithm's properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Returns spectrum 'spec' as a complex vector
  std::vector<double> toComplex(API::MatrixWorkspace_const_sptr &inWS,
                                size_t spec, bool errors, bool concatenatedSpectra);
  // Calculates chi-square by solving the matrix equation A*x = b
  double calculateChi(const QuadraticCoefficients &coeffs, double a,
                      std::vector<double> &beta);
  // Calculates the SVD of the input matrix A
  std::vector<double> solveSVD(Kernel::DblMatrix &A,
                               const Kernel::DblMatrix &B);
  /// Moves the system one step closer towards the solution
  std::vector<double> move(const QuadraticCoefficients &coeffs,
                           double ChiTargetOverN, double chiEps,
                           size_t alphaIter);
  /// Applies a distance penalty
  std::vector<double> applyDistancePenalty(const std::vector<double> &beta,
                                           const QuadraticCoefficients &coeffs,
                                           const std::vector<double> &image,
                                           double background, double distEps);
  /// Updates the image
  std::vector<double> updateImage(const std::vector<double> &image,
                                  const std::vector<double> &delta,
                                  const std::vector<std::vector<double>> dirs);

  /// Populates the output workspace containing the reconstructed data
  void populateDataWS(API::MatrixWorkspace_const_sptr &inWS, size_t spec,
                      size_t nspec, const std::vector<double> &result,
                      bool concatenatedSpectra, bool complex,
                      API::MatrixWorkspace_sptr &outWS);
  /// Populates the output workspace containing the reconstructed image
  void populateImageWS(API::MatrixWorkspace_const_sptr &inWS, size_t spec,
                       size_t nspec, const std::vector<double> &result,
                       bool complex, API::MatrixWorkspace_sptr &outWS,
                       bool autoShift);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENT_H_ */
