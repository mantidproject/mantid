// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/Functions/DeltaFunction.h"

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
class MANTID_CURVEFITTING_DLL ElasticIsoRotDiff : public DeltaFunction {
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
