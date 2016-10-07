#ifndef MANTID_ALGORITHMS_Q1D2_H_
#define MANTID_ALGORITHMS_Q1D2_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace API {
class SpectrumInfo;
}
namespace Algorithms {
/** Takes account of the effects of gravity for instruments where the y-axis
   points upwards, for
    example SANS instruments

    @author Steve Williams ISIS Rutherford Appleton Laboratory
    @date 10/12/2010

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
class DLLExport Q1D2 : public API::Algorithm {
public:
  /// Default constructor
  Q1D2();
  /// Algorithm's name
  const std::string name() const override { return "Q1D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "SANS 1D reduction. Converts a workspace in wavelength into a "
           "workspace of momentum transfer, assuming elastic scattering";
  }

  /// Algorithm's version
  int version() const override { return (2); }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  // typedefs for histogram data iterator types
  typedef std::vector<double>::iterator HistogramData_iter;
  typedef std::vector<double>::const_iterator HistogramData_const_iter;

  /// the experimental workspace with counts across the detector
  API::MatrixWorkspace_const_sptr m_dataWS;
  bool m_doSolidAngle;

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  API::MatrixWorkspace_sptr
  setUpOutputWorkspace(const std::vector<double> &binParams) const;
  // these are the steps that are run on each individual spectrum
  void calculateNormalization(const size_t wavStart, const size_t wsIndex,
                              API::MatrixWorkspace_const_sptr pixelAdj,
                              API::MatrixWorkspace_const_sptr wavePixelAdj,
                              double const *const binNorms,
                              double const *const binNormEs,
                              const HistogramData_iter norm,
                              const HistogramData_iter normETo2) const;
  void pixelWeight(API::MatrixWorkspace_const_sptr pixelAdj,
                   const size_t wsIndex, double &weight, double &error) const;
  void addWaveAdj(const double *c, const double *Dc, HistogramData_iter bInOut,
                  HistogramData_iter e2InOut) const;
  void addWaveAdj(const double *c, const double *Dc, HistogramData_iter bInOut,
                  HistogramData_iter e2InOut, HistogramData_const_iter,
                  HistogramData_const_iter) const;
  void normToMask(const size_t offSet, const size_t wsIndex,
                  const HistogramData_iter theNorms,
                  const HistogramData_iter errorSquared) const;
  void convertWavetoQ(const API::SpectrumInfo &spectrumInfo, const size_t wsInd,
                      const bool doGravity, const size_t offset,
                      HistogramData_iter Qs, const double extraLength) const;
  void getQBinPlus1(const HistogramData::HistogramX &OutQs, const double QToFind,
                    HistogramData_const_iter &loc) const;
  void normalize(const HistogramData::HistogramY &normSum,
                 const HistogramData::HistogramE &normError2,
                 HistogramData::HistogramY &counts,
                 HistogramData::HistogramE &errors) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_Q1D2_H_*/
