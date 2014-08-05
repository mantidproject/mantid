#ifndef MANTID_SINQ_POLDIPEAKCOLLECTION_H
#define MANTID_SINQ_POLDIPEAKCOLLECTION_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/V3D.h"
#include "boost/shared_ptr.hpp"

namespace Mantid {
namespace Poldi {

/** PoldiPeakCollection :
 *
  PoldiPeakCollection stores PoldiPeaks and acts as a bridge
  to TableWorkspace

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/03/2014

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

class MANTID_SINQ_DLL PoldiPeakCollection
{
public:
    PoldiPeakCollection();
    PoldiPeakCollection(DataObjects::TableWorkspace_sptr workspace);
    PoldiPeakCollection(const Geometry::UnitCell &unitCell, const Geometry::PointGroup_sptr &pointGroup, double dMin, double dMax);

    virtual ~PoldiPeakCollection() {}

    size_t peakCount() const;

    void addPeak(PoldiPeak_sptr newPeak);
    PoldiPeak_sptr peak(size_t index) const;
    const std::vector<PoldiPeak_sptr> &peaks() const;

    DataObjects::TableWorkspace_sptr asTableWorkspace();

protected:
    void prepareTable(DataObjects::TableWorkspace_sptr table);
    void peaksToTable(DataObjects::TableWorkspace_sptr table);

    void constructFromTableWorkspace(DataObjects::TableWorkspace_sptr tableWorkspace);
    bool checkColumns(DataObjects::TableWorkspace_sptr tableWorkspace);

    std::vector<Kernel::V3D> getUniqueHKLSet(const Geometry::UnitCell &unitCell, const Geometry::PointGroup_sptr &pointGroup, double dMin, double dMax);
    void setPeaks(const std::vector<Kernel::V3D> &hkls, const Geometry::UnitCell &unitCell);

    std::vector<PoldiPeak_sptr> m_peaks;
};

typedef boost::shared_ptr<PoldiPeakCollection> PoldiPeakCollection_sptr;

}
}

#endif // MANTID_SINQ_POLDIPEAKCOLLECTION_H
