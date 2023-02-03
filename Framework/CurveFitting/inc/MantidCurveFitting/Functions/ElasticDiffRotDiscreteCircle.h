// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DeltaFunction.h"
#include "MantidCurveFitting/Functions/FunctionQDepends.h"
// standard library (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date July 24 2016
*/

/* Class representing the elastic portion of DiffRotDiscreteCircle
 * Contains a Delta Dirac.
 */
class MANTID_CURVEFITTING_DLL ElasticDiffRotDiscreteCircle : public DeltaFunction, public FunctionQDepends {
public:
  /// Constructor
  ElasticDiffRotDiscreteCircle();

  /// overwrite IFunction base class methods
  std::string name() const override { return "ElasticDiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

  /// A rescaling of the peak intensity
  double HeightPrefactor() const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
