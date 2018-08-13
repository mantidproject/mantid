#ifndef MANTID_ALGORITHM_REALFFT_H_
#define MANTID_ALGORITHM_REALFFT_H_

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/FFT.h"

namespace Mantid {
namespace Algorithms {
/** Performs a Fast Fourier Transform of real data

    @author Roman Tolchenov
    @date 01/10/2009

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
class DLLExport RealFFT : public API::ParallelAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RealFFT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs real Fast Fourier Transform";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT",      "FFTDerivative", "MaxEnt",
            "SassenaFFT",         "FFTSmooth"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REALFFT_H_*/
