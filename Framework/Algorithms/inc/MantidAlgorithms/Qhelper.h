#ifndef MANTID_ALGORITHMS_QHELPER_H_
#define MANTID_ALGORITHMS_QHELPER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumInfo.h"

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
                    API::MatrixWorkspace_const_sptr detectAdj,
                    API::MatrixWorkspace_const_sptr qResolution);

  void examineInput(API::MatrixWorkspace_const_sptr dataWS,
                    API::MatrixWorkspace_const_sptr binAdj,
                    API::MatrixWorkspace_const_sptr detectAdj);

  size_t waveLengthCutOff(API::MatrixWorkspace_const_sptr dataWS,
                          const API::SpectrumInfo &spectrumInfo,
                          const double RCut, const double WCut,
                          const size_t wsInd) const;

  void outputParts(API::Algorithm *alg, API::MatrixWorkspace_sptr sumOfCounts,
                   API::MatrixWorkspace_sptr sumOfNormFactors);

private:
  /// the experimental workspace with counts across the detector
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_Q1D2_H_*/
