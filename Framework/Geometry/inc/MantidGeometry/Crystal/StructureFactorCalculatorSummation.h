// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/StructureFactorCalculator.h"
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

/** StructureFactorCalculatorSummation

  This implementation of StructureFactorCalculator uses the summation method
  provided by BraggScatterer and its sub-classes. It obtains all scatterers in
  the unit cell by combining the space group and the scatterers located in the
  asymmetric unit (both taken from CrystalStructure) and stores them.

      @author Michael Wedel, ESS
      @date 05/09/2015
*/
class MANTID_GEOMETRY_DLL StructureFactorCalculatorSummation : public StructureFactorCalculator {
public:
  StructureFactorCalculatorSummation();
  StructureFactor getF(const Kernel::V3D &hkl) const override;

protected:
  void crystalStructureSetHook(const CrystalStructure &crystalStructure) override;

  void updateUnitCellScatterers(const CrystalStructure &crystalStructure);
  std::string getV3DasString(const Kernel::V3D &point) const;

  CompositeBraggScatterer_sptr m_unitCellScatterers;
};

using StructureFactorSummation_sptr = std::shared_ptr<StructureFactorCalculatorSummation>;

} // namespace Geometry
} // namespace Mantid
