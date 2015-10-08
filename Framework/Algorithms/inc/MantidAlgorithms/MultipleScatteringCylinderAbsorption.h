#ifndef MANTID_ALGORITHM_MULTIPLE_SCATTERING_ABSORPTION_H_
#define MANTID_ALGORITHM_MULTIPLE_SCATTERING_ABSORPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <vector>

namespace Mantid {
namespace Algorithms {
/** Multiple scattering absorption correction, originally used to
    correct vanadium spectrum at IPNS.  Algorithm originally worked
    out by Jack Carpenter and Asfia Huq and implmented in Java by
    Alok Chatterjee.  Translated to C++ by Dennis Mikkelson.

    @author Dennis Mikkelson
    @date 17/08/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory &
    NScD Oak Ridge National Laboratory

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

    File change history is stored at:
                  <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport MultipleScatteringCylinderAbsorption : public API::Algorithm {
public:
  /// Default constructor
  MultipleScatteringCylinderAbsorption();

  /// Destructor
  virtual ~MultipleScatteringCylinderAbsorption();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const;

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const;

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const;

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Multiple scattering absorption correction, originally used to "
           "correct vanadium spectrum at IPNS.";
  }

private:
  // Overridden Algorithm methods
  void init();
  void exec();

  // Attenuation Factor
  double AttFac(const double sigir, const double sigsr,
                const std::vector<double> &Z);

  // Set up "Z" array for specified angle
  void ZSet(const double angle_deg, std::vector<double> &Z);

  // Wavelength function
  double wavelength(double path_length_m, double tof_us);

  /// MultipleScatteringCylinderAbsorption correction calculation.
  void apply_msa_correction(double total_path, double angle_deg, double radius,
                            double coeff1, double coeff2, double coeff3,
                            std::vector<double> &tof,
                            std::vector<double> &y_val,
                            std::vector<double> &errors);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MULTIPLE_SCATTERING_ABSORPTION_H_*/
