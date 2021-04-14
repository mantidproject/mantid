// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/LatticeDomain.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace API {

/** ILatticeFunction

  This abstract class defines the interface for a function that calculates
  values based on HKL-values and a lattice. Currently the only implementation
  is CurveFitting::LatticeFunction.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/04/2015
*/
class MANTID_API_DLL ILatticeFunction : public FunctionParameterDecorator {
public:
  ILatticeFunction();

  void function(const FunctionDomain &domain, FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;

  /// Function that should calculate d-values for the HKLs provided in the
  /// domain.
  virtual void functionLattice(const LatticeDomain &latticeDomain, FunctionValues &values) const = 0;

  virtual void functionDerivLattice(const LatticeDomain &latticeDomain, Jacobian &jacobian);

  /// A string that names the crystal system.
  virtual void setLatticeSystem(const std::string &crystalSystem) = 0;

  /// Set the function parameters according to the supplied unit cell.
  virtual void setUnitCell(const std::string &unitCellString) = 0;
  /// Overload to set unit cell directly from UnitCell object.
  virtual void setUnitCell(const Geometry::UnitCell &unitCell) = 0;

  /// Returns a unit cell object created from the function parameters
  virtual Geometry::UnitCell getUnitCell() const = 0;
};

using ILatticeFunction_sptr = std::shared_ptr<ILatticeFunction>;

} // namespace API
} // namespace Mantid
