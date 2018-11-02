// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ELASTICISOROTDIFF_H_
#define MANTID_ELASTICISOROTDIFF_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/DeltaFunction.h"
// Mantid headers from other projects (N/A)
// 3rd party library headers (N/A)
// standard library headers (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date 25/09/2016
*/

/**
 * @brief Elastic part of the DiffSphere function
 */
class DLLExport ElasticIsoRotDiff : public DeltaFunction {
public:
  /// Constructor
  ElasticIsoRotDiff();

  /// overwrite IFunction base class methods
  std::string name() const override { return "ElasticIsoRotDiff"; }

  const std::string category() const override { return "QuasiElastic"; }

  /// A rescaling of the peak intensity
  double HeightPrefactor() const override;

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_ELASTICISOROTDIFF_H_
