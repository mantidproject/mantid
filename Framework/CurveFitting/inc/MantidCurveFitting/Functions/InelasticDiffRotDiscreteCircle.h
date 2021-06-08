// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid Coding standards <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/FunctionQDepends.h"
// Mantid headers from other projects
#include "MantidAPI/MatrixWorkspace.h"
// 3rd party library headers (N/A)
// standard library (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date July 24 2016
*/

/* Class representing the inelastic portion of DiffRotDiscreteCircle
 * Contains a linear combination of Lorentzians.
 */
class MANTID_CURVEFITTING_DLL InelasticDiffRotDiscreteCircle : public FunctionQDepends {
public:
  /// Constructor
  InelasticDiffRotDiscreteCircle();

  std::string name() const override { return "InelasticDiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  void init() override;

protected:
  void function1D(double *out, const double *xValues, const size_t nData) const override;

private:
  /// Cache Q values from the workspace
  void setWorkspace(std::shared_ptr<const API::Workspace> ws) override;

  const double m_hbar; // Plank constant, in meV*ps (or ueV*ns)

  std::vector<double> m_qValueCache; // List of calculated Q values
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
