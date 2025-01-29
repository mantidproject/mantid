// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/

class MANTID_ALGORITHMS_DLL MaxEnt : public API::Algorithm {
public:
  virtual ~MaxEnt() override = default;
  /// Algorithm's name
  const std::string name() const override;
  /// Algorithm's version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT", "FFTDerivative", "RealFFT", "SassenaFFT", "FFTSmooth"};
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
  std::vector<double> toComplex(API::MatrixWorkspace_const_sptr &inWS, size_t spec, bool errors,
                                bool concatenatedSpectra);
  // Calculates chi-square by solving the matrix equation A*x = b
  double calculateChi(const QuadraticCoefficients &coeffs, double a, std::vector<double> &beta);
  // Calculates the SVD of the input matrix A
  std::vector<double> solveSVD(Kernel::DblMatrix &A, const Kernel::DblMatrix &B);
  /// Moves the system one step closer towards the solution
  std::vector<double> move(const QuadraticCoefficients &coeffs, double ChiTargetOverN, double chiEps, size_t alphaIter);
  /// Applies a distance penalty
  std::vector<double> applyDistancePenalty(const std::vector<double> &beta, const QuadraticCoefficients &coeffs,
                                           const std::vector<double> &image, double background, double distEps);
  /// Updates the image
  std::vector<double> updateImage(const std::vector<double> &image, const std::vector<double> &delta,
                                  const std::vector<std::vector<double>> &dirs);

  /// Populates the output workspace containing the reconstructed data
  void populateDataWS(API::MatrixWorkspace_const_sptr &inWS, size_t spec, size_t nspec,
                      const std::vector<double> &result, bool concatenatedSpectra, bool complex,
                      API::MatrixWorkspace_sptr &outWS);
  /// Populates the output workspace containing the reconstructed image
  void populateImageWS(API::MatrixWorkspace_const_sptr &inWS, size_t spec, size_t nspec,
                       const std::vector<double> &result, bool complex, API::MatrixWorkspace_sptr &outWS,
                       bool autoShift);
  /// Checks the bin spacing is equal
  void validateBinEdges(const std::string &wsName, std::map<std::string, std::string> &messages);
};

} // namespace Algorithms
} // namespace Mantid
