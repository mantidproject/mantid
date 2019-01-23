// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_TOBYFITBMATRIX_H_
#define MANTID_MDALGORITHMS_TOBYFITBMATRIX_H_

#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace MDAlgorithms {
//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class CachedExperimentInfo;
struct QOmegaPoint;

/**
 * Defines the linear transformation from vector of
 * independent integration functions to random
 * integration variables. Defined in Toby G Perring's thesis
 * pg 112, equation A.48. It is intimately linked to the
 * TobyFitYVector as their values need to be in sync
 */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4661)
#endif
class DLLExport TobyFitBMatrix : public Kernel::DblMatrix {
public:
  /// Default constructor sets the size of the matrix
  TobyFitBMatrix();

  /// Calculate the values for this observation & QDeltaE point
  void recalculate(const CachedExperimentInfo &observation,
                   const QOmegaPoint &qOmega);
};
#if defined(_MSC_VER)
#pragma warning(pop)
#pragma warning(disable : 4661)
#endif

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_RESOLUTIONCOEFFICIENTS_H_ */