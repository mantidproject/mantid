// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/ElasticDiffSphere.h"
#include "MantidCurveFitting/Functions/InelasticDiffSphere.h"
// Mantid headers from other projects
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
// third party library headers (N/A)
// standard library headers (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date 11/14/2011
*/

class MANTID_CURVEFITTING_DLL DiffSphere : public API::ImmutableCompositeFunction {

public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "DiffSphere"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "QuasiElastic"; }

  /// overwrite IFunction base class methods
  virtual int version() const { return 1; }

  /// Propagate an attribute to member functions
  virtual void trickleDownAttribute(const std::string &name);

  /// Override parent definition
  virtual void declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue);

  /// Override parent definition
  void setAttribute(const std::string &name, const API::IFunction::Attribute &att) override;

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  std::shared_ptr<Mantid::CurveFitting::Functions::ElasticDiffSphere>
      m_elastic; // elastic intensity of the DiffSphere structure factor
  std::shared_ptr<Mantid::CurveFitting::Functions::InelasticDiffSphere>
      m_inelastic; // inelastic intensity of the DiffSphere structure factor
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
