#ifndef MANTID_ALGORITHM_SASSENAFFT_H_
#define MANTID_ALGORITHM_SASSENAFFT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {

/** Perform Fourier Transform of the Sassena Intermediate Scattering Function

  @author Jose Borreguero
  @date 2012-05-29

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  */

class DLLExport SassenaFFT : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SassenaFFT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs complex Fast Fourier Transform of intermediate scattering "
           "function";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT", "FFTDerivative", "MaxEnt", "RealFFT",
            "FFTSmooth"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

protected:
  // Overridden Algorithm methods
  bool processGroups() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  bool checkGroups() override;
  static constexpr double m_T2ueV =
      1000.0 / Mantid::PhysicalConstants::meVtoKelvin; // conversion factor from
                                                       // Kelvin to ueV
  const double m_ps2meV = 4.136; // conversion factor from picosecond to mili-eV

}; // class SassenaFFT

} // namespace Algorithms
} // namespace Mantid

#endif // MANTID_ALGORITHM_SASSENAFFT_H_
