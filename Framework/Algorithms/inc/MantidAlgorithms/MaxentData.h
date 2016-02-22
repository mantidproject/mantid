#ifndef MANTID_ALGORITHMS_MAXENTDATA_H_
#define MANTID_ALGORITHMS_MAXENTDATA_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/MaxentEntropy.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Algorithms {

/** MaxentData : TODO: DESCRIPTION

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

struct QuadraticCoefficients {
  Kernel::DblMatrix s1; // Quadratic coefficient S_mu
  Kernel::DblMatrix c1; // Quadratic coefficient C_mu
  Kernel::DblMatrix s2; // Quadratic coefficient g_mu_nu
  Kernel::DblMatrix c2; // Quadratic coefficient M_mu_nu
};

class MANTID_ALGORITHMS_DLL MaxentData final {
public:
  MaxentData(MaxentEntropy_sptr entropy);
  MaxentData() = delete;
  virtual ~MaxentData() = default;

	// Setters
  void load(const std::vector<double> &data,
                    const std::vector<double> &errors,
                    const std::vector<double> &image, double background,
                    bool isComplex);
	void correctImage();
	void setImage(const std::vector<double> &image);

	// Getters
  std::vector<double> getChiGrad() const;
	std::vector<double> getEntropy() const;
	std::vector<double> getEntropyGrad() const;
	std::vector<double> getMetric() const;
	std::vector<std::vector<double>> getSearchDirections();
	QuadraticCoefficients getQuadraticCoefficients();
	double getAngle();
	double getChisq();

	// Other
	void calculateSearchDirections();
	std::vector<double>
		transformImageToData(const std::vector<double> &input);
	std::vector<double>
		transformDataToImage(const std::vector<double> &input);

private:
	std::vector<double> m_data;
	std::vector<double> m_errors;
	std::vector<double> m_image;
	std::vector<double> m_dataCalc;
	double m_background;
	double m_angle;
	double m_chisq;
	MaxentEntropy_sptr m_entropy;
  std::vector<std::vector<double>> m_directionsIm;
  QuadraticCoefficients m_coeffs;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTDATA_H_ */