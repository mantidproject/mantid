// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ILatticeFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/PawleyFunction.h"

namespace Mantid {
namespace CurveFitting {

/** LatticeFunction

  LatticeFunction implements API::ILatticeFunction. Internally it uses
  a PawleyParameterFunction to expose appropriate lattice parameters for each
  crystal system.

  For each HKL in the supplied LatticeDomain, the function method calculates
  the d-value using the UnitCell that is generated from the function parameters.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/04/2015
*/
class MANTID_CURVEFITTING_DLL LatticeFunction : public API::ILatticeFunction {
public:
  LatticeFunction();

  std::string name() const override { return "LatticeFunction"; }

  void functionLattice(const API::LatticeDomain &latticeDomain, API::FunctionValues &values) const override;

  void setLatticeSystem(const std::string &crystalSystem) override;
  void setUnitCell(const std::string &unitCellString) override;
  void setUnitCell(const Geometry::UnitCell &unitCell) override;
  Geometry::UnitCell getUnitCell() const override;

protected:
  void init() override;
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn) override;

private:
  Functions::PawleyParameterFunction_sptr m_cellParameters;
};

using LatticeFunction_sptr = std::shared_ptr<LatticeFunction>;

} // namespace CurveFitting
} // namespace Mantid
