#ifndef MANTID_GEOMETRY_CENTERINGGROUP_H_
#define MANTID_GEOMETRY_CENTERINGGROUP_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/Group.h"
#include <map>

namespace Mantid
{
namespace Geometry
{

/** CenteringGroup

    A class that holds symmetry operations to describe a lattice
    centering.

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
class MANTID_GEOMETRY_DLL CenteringGroup : public Group
{
public:
    enum CenteringType {
        P, I, A, B, C, F, Robv, Rrev
    };

    CenteringGroup(const std::string &centeringSymbol);
    virtual ~CenteringGroup() { }

    CenteringType getType() const;
    std::string getSymbol() const;

protected:
    CenteringType m_type;
    std::string m_symbol;

private:
    /// Private helper class to keep this out of the interface of CenteringGroup.
    class CenteringGroupCreationHelper
    {
    public:
        static CenteringGroup::CenteringType getCenteringType(const std::string &centeringSymbol);

        static std::vector<SymmetryOperation> getSymmetryOperations(CenteringGroup::CenteringType centeringType);

    protected:
        CenteringGroupCreationHelper() { }
        ~CenteringGroupCreationHelper() { }

        static std::vector<SymmetryOperation> getPrimitive();
        static std::vector<SymmetryOperation> getBodyCentered();
        static std::vector<SymmetryOperation> getACentered();
        static std::vector<SymmetryOperation> getBCentered();
        static std::vector<SymmetryOperation> getCCentered();
        static std::vector<SymmetryOperation> getFCentered();
        static std::vector<SymmetryOperation> getRobvCentered();
        static std::vector<SymmetryOperation> getRrevCentered();

        static std::map<std::string, CenteringGroup::CenteringType> m_centeringSymbolMap;
    };
    
};

typedef boost::shared_ptr<CenteringGroup> CenteringGroup_sptr;
typedef boost::shared_ptr<const CenteringGroup> CenteringGroup_const_sptr;





} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_CENTERINGGROUP_H_ */
