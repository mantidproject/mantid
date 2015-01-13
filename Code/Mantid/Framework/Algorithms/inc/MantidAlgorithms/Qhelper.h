#ifndef MANTID_ALGORITHMS_QHELPER_H_
#define MANTID_ALGORITHMS_QHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/GravitySANSHelper.h"

namespace Mantid {
namespace Algorithms {
/** Helper class for the Q1D and Qxy algorithms

    @author Anders Markvardsen ISIS Rutherford Appleton Laboratory
    @date 30/09/2011

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
class Qhelper {
public:
  void examineInput(API::MatrixWorkspace_const_sptr dataWS,
                    API::MatrixWorkspace_const_sptr binAdj,
                    API::MatrixWorkspace_const_sptr detectAdj);

  size_t waveLengthCutOff(API::MatrixWorkspace_const_sptr dataWS,
                          const double RCut, const double WCut,
                          const size_t specInd) const;

  void outputParts(API::Algorithm *alg, API::MatrixWorkspace_sptr sumOfCounts,
                   API::MatrixWorkspace_sptr sumOfNormFactors);

private:
  /// the experimental workspace with counts across the detector
  /*  API::MatrixWorkspace_const_sptr m_dataWS;
    ///The radius cut off, should be value of the property RadiusCut. A value of
    zero here will disable the cut off and all wavelengths are used
    double m_RCut;
    ///The wavelength cut off divided by the radius cut is used in the
    calculation of the first wavelength to include, it's value is only used if
    RadiusCut > 0
    double m_WCutOver;



    void initizeCutOffs(const double RCut, const double WCut);

    API::MatrixWorkspace_sptr setUpOutputWorkspace(const std::vector<double> &
    binParams) const;
    //these are the steps that are run on each individual spectrum
    size_t waveLengthCutOff(const size_t specInd) const;
    void calculateNormalization(const size_t wavStart, const size_t specInd,
    API::MatrixWorkspace_const_sptr pixelAdj, double const * const binNorms,
    double const * const binNormEs, const MantidVec::iterator norm, const
    MantidVec::iterator normETo2) const;
    void pixelWeight(API::MatrixWorkspace_const_sptr pixelAdj, const size_t
    specIndex, double & weight, double & error) const;
    void addWaveAdj(const double * c, const double * Dc, MantidVec::iterator
    bInOut, MantidVec::iterator e2InOut) const;
    void normToMask(const size_t offSet, const size_t specIndex, const
    MantidVec::iterator theNorms, const MantidVec::iterator errorSquared) const;
    void convertWavetoQ(const size_t specInd, const bool doGravity, const size_t
    offset, MantidVec::iterator Qs) const;
    void getQBinPlus1(const MantidVec & OutQs, const double QToFind,
    MantidVec::const_iterator & loc) const;
    void normalize(const MantidVec & normSum, const MantidVec & normError2,
    MantidVec & YOut, MantidVec & errors) const;*/
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_Q1D2_H_*/
