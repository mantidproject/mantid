#ifndef MANTID_ALGORITHM_FFT_H_
#define MANTID_ALGORITHM_FFT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/cow_ptr.h"
#include <gsl/gsl_fft_complex.h>

namespace boost {
template <typename T> class shared_array;
}

namespace Mantid {

namespace HistogramData {
class HistogramX;
}

namespace Algorithms {

/** Performs a Fast Fourier Transform of data

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
class DLLExport FFT : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FFT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs complex Fast Fourier Transform";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFTDerivative", "MaxEnt", "RealFFT",
            "SassenaFFT", "FFTSmooth"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

protected:
  /// Perform validation of inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void createUnitsLabels(double &df);

  // Perform forward transformation
  void transformForward(boost::shared_array<double> &data, const int xSize,
                        const int ySize, const int dys,
                        const bool addPositiveOnly, const bool centerShift,
                        const bool isComplex, const int iReal, const int iImag,
                        const double df, const double dx);
  // Perform backward transformation
  void transformBackward(boost::shared_array<double> &data, const int xSize,
                         const int ySize, const int dys, const bool centerShift,
                         const bool isComplex, const int iReal, const int iImag,
                         const double df);

  void setupTAxis(const int nOut, const bool addPositiveOnly);
  /// Check whether supplied values are evenly spaced
  bool areBinWidthsUneven(const HistogramData::BinEdges &xBins) const;
  /// Get phase shift - user supplied or auto-calculated
  double getPhaseShift(const HistogramData::Points &xPoints);

private:
  Mantid::API::MatrixWorkspace_const_sptr m_inWS;
  Mantid::API::MatrixWorkspace_const_sptr m_inImagWS;
  Mantid::API::MatrixWorkspace_sptr m_outWS;
  gsl_fft_complex_wavetable *m_wavetable;
  gsl_fft_complex_workspace *m_workspace;
  int m_iIm;
  int m_iRe;
  int m_iAbs;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FFT_H_*/
