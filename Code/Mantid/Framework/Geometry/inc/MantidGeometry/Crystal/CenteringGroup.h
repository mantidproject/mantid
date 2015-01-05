#ifndef MANTID_GEOMETRY_CENTERINGGROUP_H_
#define MANTID_GEOMETRY_CENTERINGGROUP_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/Group.h"
#include <map>

#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace Geometry {

/** CenteringGroup

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

    Copyright © 2014 PSI-MSS

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
class MANTID_GEOMETRY_DLL CenteringGroup : public Group {
public:
  enum CenteringType { P, I, A, B, C, F, Robv, Rrev };

  CenteringGroup(const std::string &centeringSymbol);
  virtual ~CenteringGroup() {}

  CenteringType getType() const;
  std::string getSymbol() const;

protected:
  CenteringType m_type;
  std::string m_symbol;
};

typedef boost::shared_ptr<CenteringGroup> CenteringGroup_sptr;
typedef boost::shared_ptr<const CenteringGroup> CenteringGroup_const_sptr;

/// Helper class to keep this out of the interface of CenteringGroup.
class MANTID_GEOMETRY_DLL CenteringGroupCreatorImpl {
public:
  ~CenteringGroupCreatorImpl() {}

  CenteringGroup::CenteringType
  getCenteringType(const std::string &centeringSymbol) const;

  std::vector<SymmetryOperation>
  getSymmetryOperations(CenteringGroup::CenteringType centeringType) const;

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

#ifdef _WIN32
template class MANTID_GEOMETRY_DLL
    Mantid::Kernel::SingletonHolder<CenteringGroupCreatorImpl>;
#endif

typedef Mantid::Kernel::SingletonHolder<CenteringGroupCreatorImpl>
    CenteringGroupCreator;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_CENTERINGGROUP_H_ */
