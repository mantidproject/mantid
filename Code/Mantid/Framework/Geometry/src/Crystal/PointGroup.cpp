#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Geometry
{

  /** @return a vector with all possible PointGroup objects */
  std::vector<PointGroup_sptr> getAllPointGroups()
  {
    std::vector<PointGroup_sptr> out;
    out.push_back(PointGroup_sptr(new PointGroupLaue1() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue2() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue3() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue4() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue5() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue6() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue7() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue8() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue9() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue10() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue11() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue12() ));
    out.push_back(PointGroup_sptr(new PointGroupLaue13() ));
    return out;
  }




} // namespace Mantid
} // namespace Geometry

