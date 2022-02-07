// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Helper class for the Q1D and Qxy algorithms

    @author Anders Markvardsen ISIS Rutherford Appleton Laboratory
    @date 30/09/2011
*/
class Qhelper {
public:
  void examineInput(const API::MatrixWorkspace_const_sptr &dataWS, const API::MatrixWorkspace_const_sptr &binAdj,
                    const API::MatrixWorkspace_const_sptr &detectAdj,
                    const API::MatrixWorkspace_const_sptr &qResolution);

  void examineInput(const API::MatrixWorkspace_const_sptr &dataWS, const API::MatrixWorkspace_const_sptr &binAdj,
                    const API::MatrixWorkspace_const_sptr &detectAdj);

  size_t waveLengthCutOff(const API::MatrixWorkspace_const_sptr &dataWS, const API::SpectrumInfo &spectrumInfo,
                          const double RCut, const double WCut, const size_t wsInd) const;

  void outputParts(API::Algorithm *alg, const API::MatrixWorkspace_sptr &sumOfCounts,
                   const API::MatrixWorkspace_sptr &sumOfNormFactors);

private:
  /// the experimental workspace with counts across the detector
};

} // namespace Algorithms
} // namespace Mantid
