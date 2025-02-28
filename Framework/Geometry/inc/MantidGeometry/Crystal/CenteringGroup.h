// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/DllConfig.h"
#include <map>

#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace Geometry {

/**
    @class CenteringGroup

    This class is mostly a convenience class. It takes a bravais lattice symbol
    (P, I, A, B, C, F, R) and forms a group that contains all translations
    connected to the centering. This is for example used in the space group
    generation process.

    In addition to the inherited interface of Group, CenteringGroup provides
    methods that provide some meta information, namely the "name" of the
    centering operation. While CenteringGroup::getSymbol() returns a string,
    CenteringGroup::getType() returns a value of the enum type
    CenteringGroup::CenteringType.

    Important differences occur in the handling of Rhombohedral centering.
    CenteringType distinguishes between obverse (Robv) and reverse (Rrev)
    setting. These can be given explicitly as strings for construction. When
    only "R" is provided, the obverse setting is assumed.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 07/10/2014
  */
class MANTID_GEOMETRY_DLL CenteringGroup : public Group {
public:
  enum CenteringType { P, I, A, B, C, F, Robv, Rrev };

  CenteringGroup(const std::string &centeringSymbol);
  CenteringType getType() const;
  const std::string &getSymbol() const;

protected:
  CenteringType m_type;
  std::string m_symbol;
};

using CenteringGroup_sptr = std::shared_ptr<CenteringGroup>;
using CenteringGroup_const_sptr = std::shared_ptr<const CenteringGroup>;

/// Helper class to keep this out of the interface of CenteringGroup.
class MANTID_GEOMETRY_DLL CenteringGroupCreatorImpl {
public:
  CenteringGroup::CenteringType getCenteringType(const std::string &centeringSymbol) const;

  std::vector<SymmetryOperation> getSymmetryOperations(CenteringGroup::CenteringType centeringType) const;

protected:
  std::vector<SymmetryOperation> getPrimitive() const;
  std::vector<SymmetryOperation> getBodyCentered() const;
  std::vector<SymmetryOperation> getACentered() const;
  std::vector<SymmetryOperation> getBCentered() const;
  std::vector<SymmetryOperation> getCCentered() const;
  std::vector<SymmetryOperation> getFCentered() const;
  std::vector<SymmetryOperation> getRobvCentered() const;
  std::vector<SymmetryOperation> getRrevCentered() const;
  CenteringGroupCreatorImpl();

  std::map<std::string, CenteringGroup::CenteringType> m_centeringSymbolMap;

private:
  friend struct Mantid::Kernel::CreateUsingNew<CenteringGroupCreatorImpl>;
};

using CenteringGroupCreator = Mantid::Kernel::SingletonHolder<CenteringGroupCreatorImpl>;

} // namespace Geometry
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_GEOMETRY template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<Mantid::Geometry::CenteringGroupCreatorImpl>;
}
} // namespace Mantid
