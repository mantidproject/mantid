#ifndef FFTDERIVATIVE_H_
#define FFTDERIVATIVE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** Calculates derivatives of the spectra in a MatrixWorkspace using a Fast
   Fourier Transform.

    @author Roman Tolchenov
    @date 21/02/2011

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
class FFTDerivative : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "FFTDerivative"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculated derivatives of a spectra in the MatrixWorkspace using "
           "Fast Fourier Transform (FFT).";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execRealFFT();
  void execComplexFFT();
};

} // FFT
} // Mandid

#endif /*DERIVATIVE_H_*/
