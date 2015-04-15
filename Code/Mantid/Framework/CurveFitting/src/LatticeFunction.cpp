#include "MantidCurveFitting/LatticeFunction.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Geometry;

LatticeFunction::LatticeFunction() : ILatticeFunction(), m_cellParameters() {}

/// Calculates d-values using the internally stored cell parameters
void LatticeFunction::functionLattice(const LatticeDomain &latticeDomain,
                                      FunctionValues &values) const {
  throwIfNoFunctionSet();

  UnitCell cell = m_cellParameters->getUnitCellFromParameters();
  double zeroShift = m_cellParameters->getParameter("ZeroShift");

  for (size_t i = 0; i < values.size(); ++i) {
    values.setCalculated(i, cell.d(latticeDomain[i]) + zeroShift);
  }
}

/// Assigns the crystal system to the internally stored function. Number of
/// parameters may change after this function call.
void LatticeFunction::setCrystalSystem(const std::string &crystalSystem) {
  throwIfNoFunctionSet();

  m_cellParameters->setAttributeValue("CrystalSystem", crystalSystem);
}

/// Sets the unit cell parameters from a string that can be parsed by
/// Geometry::strToUnitCell.
void LatticeFunction::setUnitCell(const std::string &unitCellString) {
  throwIfNoFunctionSet();

  m_cellParameters->setParametersFromUnitCell(strToUnitCell(unitCellString));
}

/// Sets the decorated function to expose parameters
void LatticeFunction::init() {
  setDecoratedFunction("PawleyParameterFunction");
}

/// Checks that the decorated function is a PawleyParameterFunction.
void LatticeFunction::beforeDecoratedFunctionSet(const IFunction_sptr &fn) {
  PawleyParameterFunction_sptr paramFn =
      boost::dynamic_pointer_cast<PawleyParameterFunction>(fn);

  if (!paramFn) {
    throw std::invalid_argument(
        "LatticeFunction can only decorate a PawleyParameterFunction.");
  }

  m_cellParameters = paramFn;
}

} // namespace CurveFitting
} // namespace Mantid
