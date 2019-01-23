// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_QHELPER_H_
#define MANTID_ALGORITHMS_QHELPER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumInfo.h"

namespace Mantid {
namespace Algorithms {
/** Helper class for the Q1D and Qxy algorithms

    @author Anders Markvardsen ISIS Rutherford Appleton Laboratory
    @date 30/09/2011
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
