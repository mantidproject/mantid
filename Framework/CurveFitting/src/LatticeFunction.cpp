// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/LatticeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Geometry;

DECLARE_FUNCTION(LatticeFunction)

LatticeFunction::LatticeFunction() : ILatticeFunction(), m_cellParameters() {}

/// Calculates d-values using the internally stored cell parameters
void LatticeFunction::functionLattice(const LatticeDomain &latticeDomain, FunctionValues &values) const {
  throwIfNoFunctionSet();

  UnitCell cell = m_cellParameters->getUnitCellFromParameters();
  double zeroShift = m_cellParameters->getParameter("ZeroShift");

  for (size_t i = 0; i < values.size(); ++i) {
    values.setCalculated(i, cell.d(latticeDomain[i]) + zeroShift);
  }
}

/// Assigns the crystal system to the internally stored function. Number of
/// parameters may change after this function call.
void LatticeFunction::setLatticeSystem(const std::string &crystalSystem) {
  throwIfNoFunctionSet();

  m_cellParameters->setAttributeValue("LatticeSystem", crystalSystem);
}

/// Sets the unit cell parameters from a string that can be parsed by
/// Geometry::strToUnitCell.
void LatticeFunction::setUnitCell(const std::string &unitCellString) {
  throwIfNoFunctionSet();

  m_cellParameters->setParametersFromUnitCell(strToUnitCell(unitCellString));
}

/// Sets the unit cell parameters from a UnitCell object.
void LatticeFunction::setUnitCell(const UnitCell &unitCell) {
  throwIfNoFunctionSet();

  m_cellParameters->setParametersFromUnitCell(unitCell);
}

/// Returns a UnitCell object constructed from the function parameters.
UnitCell LatticeFunction::getUnitCell() const {
  throwIfNoFunctionSet();

  return m_cellParameters->getUnitCellFromParameters();
}

/// Sets the decorated function to expose parameters
void LatticeFunction::init() { setDecoratedFunction("PawleyParameterFunction"); }

/// Checks that the decorated function is a PawleyParameterFunction.
void LatticeFunction::beforeDecoratedFunctionSet(const IFunction_sptr &fn) {
  Functions::PawleyParameterFunction_sptr paramFn = std::dynamic_pointer_cast<Functions::PawleyParameterFunction>(fn);

  if (!paramFn) {
    throw std::invalid_argument("LatticeFunction can only decorate a PawleyParameterFunction.");
  }

  m_cellParameters = paramFn;
}

} // namespace CurveFitting
} // namespace Mantid
