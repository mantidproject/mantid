#ifndef MANTID_ALGORITHMS_Q1D2_H_
#define MANTID_ALGORITHMS_Q1D2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/GravitySANSHelper.h"

namespace Mantid {
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
  /// (Empty) Constructor
  Q1D2() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~Q1D2() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Q1D"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "SANS 1D reduction. Converts a workspace in wavelength into a "
           "workspace of momentum transfer, assuming elastic scattering";
  }

  /// Algorithm's version
  virtual int version() const { return (2); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// the experimental workspace with counts across the detector
  API::MatrixWorkspace_const_sptr m_dataWS;
  bool m_doSolidAngle;

  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  API::MatrixWorkspace_sptr
  setUpOutputWorkspace(const std::vector<double> &binParams) const;
  // these are the steps that are run on each individual spectrum
  void calculateNormalization(const size_t wavStart, const size_t specInd,
                              API::MatrixWorkspace_const_sptr pixelAdj,
                              API::MatrixWorkspace_const_sptr wavePixelAdj,
                              double const *const binNorms,
                              double const *const binNormEs,
                              const MantidVec::iterator norm,
                              const MantidVec::iterator normETo2) const;
  void pixelWeight(API::MatrixWorkspace_const_sptr pixelAdj,
                   const size_t specIndex, double &weight, double &error) const;
  void addWaveAdj(const double *c, const double *Dc, MantidVec::iterator bInOut,
                  MantidVec::iterator e2InOut) const;
  void addWaveAdj(const double *c, const double *Dc, MantidVec::iterator bInOut,
                  MantidVec::iterator e2InOut, MantidVec::const_iterator,
                  MantidVec::const_iterator) const;
  void normToMask(const size_t offSet, const size_t specIndex,
                  const MantidVec::iterator theNorms,
                  const MantidVec::iterator errorSquared) const;
  void convertWavetoQ(const size_t specInd, const bool doGravity,
                      const size_t offset, MantidVec::iterator Qs) const;
  void getQBinPlus1(const MantidVec &OutQs, const double QToFind,
                    MantidVec::const_iterator &loc) const;
  void normalize(const MantidVec &normSum, const MantidVec &normError2,
                 MantidVec &YOut, MantidVec &errors) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_Q1D2_H_*/
