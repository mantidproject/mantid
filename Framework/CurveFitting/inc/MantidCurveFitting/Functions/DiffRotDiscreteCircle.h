// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/ElasticDiffRotDiscreteCircle.h"
#include "MantidCurveFitting/Functions/InelasticDiffRotDiscreteCircle.h"
// Mantid headers from other projects
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
// 3rd party library headers (N/A)
// standard library (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date December 02 2013
*/

/* Class representing the dynamics structure factor of a particle undergoing
 * discrete jumps on N-sites evenly distributed in a circle. The particle can
 * only
 * jump to neighboring sites. This is the most common type of discrete
 * rotational diffusion in a circle.
 */
class MANTID_CURVEFITTING_DLL DiffRotDiscreteCircle : public API::ImmutableCompositeFunction {
public:
  std::string name() const override { return "DiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  virtual int version() const { return 1; }

  void init() override;

  /// Propagate an attribute to member functions
  virtual void trickleDownAttribute(const std::string &name);

  /// Override parent definition
  virtual void declareAttribute(const std::string &name, const API::IFunction::Attribute &defaultValue);

  /// Override parent definition
  void setAttribute(const std::string &name, const API::IFunction::Attribute &att) override;

private:
  std::shared_ptr<ElasticDiffRotDiscreteCircle> m_elastic;

  std::shared_ptr<InelasticDiffRotDiscreteCircle> m_inelastic;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
