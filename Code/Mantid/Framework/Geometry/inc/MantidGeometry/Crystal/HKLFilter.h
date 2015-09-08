#ifndef MANTID_GEOMETRY_HKLFILTER_H_
#define MANTID_GEOMETRY_HKLFILTER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/StructureFactorCalculatorSummation.h"

#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {

/** HKLFilter


      @author Michael Wedel, ESS
      @date 06/09/2015

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
class MANTID_GEOMETRY_DLL HKLFilter {
public:
  HKLFilter() {}
  virtual ~HKLFilter() {}

  virtual std::string getName() const = 0;

  virtual bool isAllowed(const Kernel::V3D &hkl) const = 0;
};

class MANTID_GEOMETRY_DLL HKLFilterBinaryLogicOperation : public HKLFilter {
public:
  HKLFilterBinaryLogicOperation(const HKLFilter &lhs, const HKLFilter &rhs)
      : HKLFilter(), m_lhs(lhs), m_rhs(rhs) {}
  virtual ~HKLFilterBinaryLogicOperation() {}

  const HKLFilter &getLHS() const { return m_lhs; }
  const HKLFilter &getRHS() const { return m_rhs; }

protected:
  const HKLFilter &m_lhs;
  const HKLFilter &m_rhs;
};

class MANTID_GEOMETRY_DLL HKLFilterAnd : public HKLFilterBinaryLogicOperation {
public:
  HKLFilterAnd(const HKLFilter &lhs, const HKLFilter &rhs)
      : HKLFilterBinaryLogicOperation(lhs, rhs) {}

  std::string getName() const { return "AND"; }

  bool isAllowed(const Kernel::V3D &hkl) const {
    return m_lhs.isAllowed(hkl) && m_rhs.isAllowed(hkl);
  }
};

HKLFilterAnd operator&(const HKLFilter &lhs, const HKLFilter &rhs) {
  return HKLFilterAnd(lhs, rhs);
}

class MANTID_GEOMETRY_DLL HKLFilterCrystalStructure : public HKLFilter {
public:
  HKLFilterCrystalStructure() : HKLFilter() {}

  virtual void setCrystalStructure(const CrystalStructure &crystalStructure) {
    UNUSED_ARG(crystalStructure);
  }
};

class MANTID_GEOMETRY_DLL HKLFilterDRange : public HKLFilterCrystalStructure {
public:
  HKLFilterDRange(double dMin, double dMax)
      : HKLFilterCrystalStructure(), m_cell(), m_dmin(dMin), m_dmax(dMax) {}

  std::string getName() const { return "dRange"; }

  void setCrystalStructure(const CrystalStructure &crystalStructure) {
    m_cell = crystalStructure.cell();
  }

  bool isAllowed(const Kernel::V3D &hkl) const {
    double d = m_cell.d(hkl);

    return d >= m_dmin && d <= m_dmax;
  }

protected:
  UnitCell m_cell;
  double m_dmin, m_dmax;
};

class MANTID_GEOMETRY_DLL HKLFilterSpaceGroup
    : public HKLFilterCrystalStructure {
public:
  HKLFilterSpaceGroup() : HKLFilterCrystalStructure(), m_spaceGroup() {}

  std::string getName() const { return "SpaceGroup"; }

  void setCrystalStructure(const CrystalStructure &crystalStructure) {
    m_spaceGroup = crystalStructure.spaceGroup();
  }

  bool isAllowed(const Kernel::V3D &hkl) const {
    return m_spaceGroup->isAllowedReflection(hkl);
  }

protected:
  SpaceGroup_const_sptr m_spaceGroup;
};

class MANTID_GEOMETRY_DLL HKLFilterStructureFactor
    : public HKLFilterCrystalStructure {
public:
  HKLFilterStructureFactor() : HKLFilterCrystalStructure(), m_calculator() {}

  std::string getName() const { return "SF"; }

  void setCrystalStructure(const CrystalStructure &crystalStructure) {
    m_calculator.setCrystalStructure(crystalStructure);
  }

  bool isAllowed(const Kernel::V3D &hkl) const {
    return m_calculator.getFSquared(hkl) > 1e-6;
  }

protected:
  StructureFactorCalculatorSummation m_calculator;
};

class MANTID_GEOMETRY_DLL HKLFilterCentering
    : public HKLFilterCrystalStructure {
public:
  HKLFilterCentering() : HKLFilterCrystalStructure(), m_centering() {}

  std::string getName() const { return "Centering"; }

  void setCrystalStructure(const CrystalStructure &crystalStructure) {
    m_centering = crystalStructure.centering();
  }

  bool isAllowed(const Kernel::V3D &hkl) const {
    return m_centering->isAllowed(static_cast<int>(hkl.X()),
                                  static_cast<int>(hkl.Y()),
                                  static_cast<int>(hkl.Z()));
  }

protected:
  ReflectionCondition_sptr m_centering;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKLFILTER_H_ */
