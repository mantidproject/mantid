#ifndef MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORSUMMATION_H_
#define MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORSUMMATION_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"

namespace Mantid {
namespace Geometry {

/** StructureFactorCalculatorSummation

  This implementation of StructureFactorCalculator uses the summation method
  provided by BraggScatterer and its sub-classes. It obtains all scatterers in
  the unit cell by combining the space group and the scatterers located in the
  asymmetric unit (both taken from CrystalStructure) and stores them.

      @author Michael Wedel, ESS
      @date 05/09/2015

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL StructureFactorCalculatorSummation
    : public StructureFactorCalculator {
public:
  StructureFactorCalculatorSummation();
  StructureFactor getF(const Kernel::V3D &hkl) const override;

protected:
  void
  crystalStructureSetHook(const CrystalStructure &crystalStructure) override;

  void updateUnitCellScatterers(const CrystalStructure &crystalStructure);
  std::string getV3DasString(const Kernel::V3D &point) const;

  CompositeBraggScatterer_sptr m_unitCellScatterers;
};

using StructureFactorSummation_sptr =
    boost::shared_ptr<StructureFactorCalculatorSummation>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_STRUCTUREFACTORCALCULATORSUMMATION_H_ */
