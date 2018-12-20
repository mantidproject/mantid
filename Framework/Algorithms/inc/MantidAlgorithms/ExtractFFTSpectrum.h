#ifndef MANTID_ALGORITHMS_EXTRACT_FFT_SPECTRUM_H_
#define MANTID_ALGORITHMS_EXTRACT_FFT_SPECTRUM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
    This algorithm performs a Fast Fourier Transform on each spectra of the
   input workspace.
    It then takes a specified part of the FFT result (parameter "FFTPart") and
   places it in
    a new workspace, which will share the Y axis of the old one (ie,
   spectra-detector map) and
    have the unit label set to "Time / ns" (a non-functional unit).

    @author Michael Whitty, STFC ISIS
    @date 21/09/2010

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ExtractFFTSpectrum : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ExtractFFTSpectrum"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm performs a Fast Fourier Transform on each spectrum "
           "in a workspace, and from the result takes the indicated spectrum "
           "and places it into the OutputWorkspace, so that you end up with "
           "one result spectrum for each input spectrum in the same workspace.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"FFT",     "FFTDerivative", "MaxEnt",
            "RealFFT", "SassenaFFT",    "FFTSmooth"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EXTRACT_FFT_SPECTRUM_H_*/
