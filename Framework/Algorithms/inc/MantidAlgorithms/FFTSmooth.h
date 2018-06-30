#ifndef MANTID_ALGORITHM_FFTSMOOTH_H_
#define MANTID_ALGORITHM_FFTSMOOTH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/** Data smoothing using the FFT algorithm and various filters.

    @author Roman Tolchenov
    @date 07/07/2009

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
class DLLExport FFTSmooth : public API::Algorithm,
                            public API::DeprecatedAlgorithm {
public:
  /// Constructor
  FFTSmooth() { this->useAlgorithm("FFTSmooth", 2); }
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FFTSmooth"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs smoothing of a spectrum using various filters.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Arithmetic\\FFT;Transforms\\Smoothing";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  // Smoothing by truncation.
  void truncate(int n);
  // Smoothing by zeroing.
  void zero(int n);

  /// The input workspace
  API::MatrixWorkspace_sptr m_inWS;
  /// Temporary workspace for keeping the unfiltered Fourier transform of the
  /// imput spectrum
  API::MatrixWorkspace_sptr m_unfilteredWS;
  /// Temporary workspace for keeping the filtered spectrum
  API::MatrixWorkspace_sptr m_filteredWS;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FFTSMOOTH_H_*/
