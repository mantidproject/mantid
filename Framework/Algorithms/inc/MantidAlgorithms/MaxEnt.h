#ifndef MANTID_ALGORITHMS_MAXENT_H_
#define MANTID_ALGORITHMS_MAXENT_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"
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

/// Auxiliary class to store the search directions (xi), quadratic coefficients
/// (c1, s1, c2, s2), angle and chi-sq
class SearchDirections {
  // SB eq. 21
  // SB eq. 24
public:
  SearchDirections(size_t dim, size_t points) {
    xIm = Kernel::DblMatrix(dim, points);
    xDat = Kernel::DblMatrix(dim, points);
    s1 = Kernel::DblMatrix(dim, 1);
    c1 = Kernel::DblMatrix(dim, 1);
    s2 = Kernel::DblMatrix(dim, dim);
    c2 = Kernel::DblMatrix(dim, dim);
  };
  Kernel::DblMatrix xIm;  // Search directions in image space
  Kernel::DblMatrix xDat; // Search directions in data space
  Kernel::DblMatrix s1;   // Quadratic coefficient S_mu
  Kernel::DblMatrix c1;   // Quadratic coefficient C_mu
  Kernel::DblMatrix s2;   // Quadratic coefficient g_mu_nu
  Kernel::DblMatrix c2;   // Quadratic coefficient M_mu_nu
  // Chi-sq
  double chisq;
  // Non-parallelism between S and C (angle)
  double angle;
};

class DLLExport MaxEnt : public API::Algorithm {
public:
  /// Constructor
  MaxEnt();
  /// Destructor
  virtual ~MaxEnt();

  /// Algorithm's name
  virtual const std::string name() const;
  /// Algorithm's version
  virtual int version() const;
  /// Algorithm's category
  virtual const std::string category() const;
  /// Algorithm's summary
  virtual const std::string summary() const;

private:
  /// Initialise the algorithm's properties
  void init();
  /// Run the algorithm
  void exec();
  /// Validate the input properties
  std::map<std::string, std::string> validateInputs();
  /// Transforms from image space to data space
  std::vector<double> transformImageToData(const std::vector<double> &input);
  /// Transforms from data space to image space
  std::vector<double> transformDataToImage(const std::vector<double> &input);
  /// Calculates chi-square
  double getChiSq(const std::vector<double> &data,
                  const std::vector<double> &errors,
                  const std::vector<double> &dataCalc);
  /// Calculates the gradient of Chi
  std::vector<double> getCGrad(const std::vector<double> &data,
                               const std::vector<double> &errors,
                               const std::vector<double> &dataCalc);
  /// Calculates the gradient of S (entropy)
  std::vector<double> getSGrad(const std::vector<double> &image,
                               double background);
  /// Calculates the search directions and the quadratic coefficients
  SearchDirections calculateSearchDirections(const std::vector<double> &data,
                                             const std::vector<double> &error,
                                             const std::vector<double> &image,
                                             double background);
  // Calculates chi-square by solving the matrix equation A*x = b
  double calculateChi(const SearchDirections &coeffs, double a,
                      std::vector<double> &beta);
  // Calculates the SVD of the input matrix A
  std::vector<double> solveSVD(const Kernel::DblMatrix &A,
                               const Kernel::DblMatrix &B);
  /// Moves the system one step closer towards the solution
  std::vector<double> move(const SearchDirections &dirs, double chiTarget,
                           double chiEps, size_t alphaIter);
  /// Calculates the distance of the current solution
  double distance(const Kernel::DblMatrix &s2, const std::vector<double> &beta);
  /// Populates the output workspaces
  void populateOutputWS(const API::MatrixWorkspace_sptr &inWS, size_t spec,
                        size_t nspec, const std::vector<double> &data,
                        const std::vector<double> &image,
                        API::MatrixWorkspace_sptr &outData,
                        API::MatrixWorkspace_sptr &outImage);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENT_H_ */