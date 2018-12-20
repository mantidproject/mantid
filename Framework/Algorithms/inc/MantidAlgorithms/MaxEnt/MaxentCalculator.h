#ifndef MANTID_ALGORITHMS_MAXENTCALCULATOR_H_
#define MANTID_ALGORITHMS_MAXENTCALCULATOR_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/MaxEnt/MaxentCoefficients.h"
#include "MantidAlgorithms/MaxEnt/MaxentEntropy.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransform.h"

namespace Mantid {
namespace Algorithms {

/** MaxentCalculator : This class performs one maxent iteration and calculates
  chi-sq, angle between gradient of S and gradient of chi-sq,  search directions
  and quadratic coefficients. Calculations are based on J. Skilling and R. K.
  Bryan: "Maximum entropy image reconstruction: general algorithm" (1984),
  section 3.6.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_ALGORITHMS_DLL MaxentCalculator final {
public:
  // Constructor
  MaxentCalculator(MaxentEntropy_sptr entropy, MaxentTransform_sptr transform);
  // Deleted default constructor
  MaxentCalculator() = delete;
  // Destructor
  virtual ~MaxentCalculator() = default;

  // Runs maxent iteration
  void iterate(const std::vector<double> &data,
               const std::vector<double> &errors,
               const std::vector<double> &image, double background,
               const std::vector<double> &linearAdjustments,
               const std::vector<double> &constAdjustments);

  // Getters
  // Returns the reconstructed (calculated) data
  std::vector<double> getReconstructedData() const;
  // Returns the image
  std::vector<double> getImage() const;
  // Returns the quadratic coefficients
  QuadraticCoefficients getQuadraticCoefficients() const;
  // Returns the search directions
  std::vector<std::vector<double>> getSearchDirections() const;
  // Returns the angle between Grad(S) and Grad(C)
  double getAngle() const;
  // Returns the chi-square
  double getChisq();

  std::vector<double> calculateData(const std::vector<double> &image) const;
  std::vector<double> calculateImage(const std::vector<double> &data) const;
  double calculateChiSquared(const std::vector<double> &data) const;

private:
  // Calculates the gradient of chi
  std::vector<double> calculateChiGrad() const;
  // Calculates chi-square
  void calculateChisq();

  // Member variables
  // The experimental (measured) data
  std::vector<double> m_data;
  // The experimental (measured) errors
  std::vector<double> m_errors;
  // The image
  std::vector<double> m_image;
  // The reconstructed (calculated) data
  std::vector<double> m_dataCalc;
  // The background
  double m_background;
  // The angle between Grad(C) and Grad(S)
  double m_angle;
  // Chi-square
  double m_chisq;
  // The search directions
  std::vector<std::vector<double>> m_directionsIm;
  // The quadratic coefficients
  QuadraticCoefficients m_coeffs;

  // The type of entropy
  MaxentEntropy_sptr m_entropy;
  // The type of transform
  MaxentTransform_sptr m_transform;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTCALCULATOR_H_ */
