#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Geometry
{
using Kernel::V3D;

  std::string PointGroupLaue1::getName()
  {
      return "-1 (Triclinic)";
  }

  bool PointGroupLaue1::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,-l));
  }

  std::string PointGroupLaue2::getName()
  {
      return "1 2/m 1 (Monoclinic, unique axis b)";
  }

  bool PointGroupLaue2::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,l));
  }

  std::string PointGroupLaue3::getName()
  {
      return "1 1 2/m (Monoclinic, unique axis c)";
  }

  bool PointGroupLaue3::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l));
  }

  std::string PointGroupLaue4::getName()
  {
      return "mmm (Orthorombic)";
  }

  bool PointGroupLaue4::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l))
              || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l))
              || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l));
  }

  std::string PointGroupLaue5::getName()
  {
      return "4/m (Tetragonal)";
  }

  bool PointGroupLaue5::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-k,h,l))
              || (hkl2 == V3D(k,-h,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l))
              || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-k,h,-l));
  }

  std::string PointGroupLaue6::getName()
  {
      return "4/mmm (Tetragonal)";
  }

  bool PointGroupLaue6::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-k,h,l))
              || (hkl2 == V3D(k,-h,l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,-l))
              || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h,-k,-l))
              || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-k,h,-l))
              || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-k,-h,l))
              || (hkl2 == V3D(k,h,l));
  }

  std::string PointGroupLaue7::getName()
  {
      return "-3 (Trigonal - Hexagonal)";
  }

  bool PointGroupLaue7::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l));
  }

  std::string PointGroupLaue8::getName()
  {
      return "-3m1 (Trigonal - Rhombohedral)";
  }

  bool PointGroupLaue8::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l));
  }

  std::string PointGroupLaue9::getName()
  {
      return "-31m (Trigonal - Rhombohedral)";
  }

  bool PointGroupLaue9::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l));
  }

  std::string PointGroupLaue10::getName()
  {
      return "6/m (Hexagonal)";
  }

  bool PointGroupLaue10::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(k,-h+k,l)) || (hkl2 == V3D(h-k,h,l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(-k,h-k,-l)) || (hkl2 == V3D(-h+k,-h,-l));
  }

  std::string PointGroupLaue11::getName()
  {
      return "6/mmm (Hexagonal)";
  }

  bool PointGroupLaue11::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l))
              || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(k,-h+k,l)) || (hkl2 == V3D(h-k,h,l))
              || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(h-k,-k,-l)) || (hkl2 == V3D(-h,-h+k,-l))
              || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l))
              || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(-k,h-k,-l)) || (hkl2 == V3D(-h+k,-h,-l))
              || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(-h+k,k,l)) || (hkl2 == V3D(h,h-k,l))
              || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l));
  }

  std::string PointGroupLaue12::getName()
  {
      return "m-3 (Cubic)";
  }

  bool PointGroupLaue12::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l))
              || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(l,h,k)) || (hkl2 == V3D(l,-h,-k))
              || (hkl2 == V3D(-l,-h,k)) || (hkl2 == V3D(-l,h,-k)) || (hkl2 == V3D(k,l,h))
              || (hkl2 == V3D(-k,l,-h)) || (hkl2 == V3D(k,-l,-h)) || (hkl2 == V3D(-k,-l,h))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(h,-k,l))
              || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-l,-h,-k)) || (hkl2 == V3D(-l,h,k))
              || (hkl2 == V3D(l,h,-k)) || (hkl2 == V3D(l,-h,k)) || (hkl2 == V3D(-k,-l,-h))
              || (hkl2 == V3D(k,-l,h)) || (hkl2 == V3D(-k,l,h)) || (hkl2 == V3D(k,l,-h));
  }

  std::string PointGroupLaue13::getName()
  {
      return "m-3m (Cubic)";
  }

  bool PointGroupLaue13::isEquivalent(V3D hkl, V3D hkl2)
  {
      double h=hkl[0];
      double k=hkl[1];
      double l=hkl[2];

      return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l))
              || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(l,h,k)) || (hkl2 == V3D(l,-h,-k))
              || (hkl2 == V3D(-l,-h,k)) || (hkl2 == V3D(-l,h,-k)) || (hkl2 == V3D(k,l,h))
              || (hkl2 == V3D(-k,l,-h)) || (hkl2 == V3D(k,-l,-h)) || (hkl2 == V3D(-k,-l,h))
              || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(k,-h,l))
              || (hkl2 == V3D(-k,h,l)) || (hkl2 == V3D(h,l,-k)) || (hkl2 == V3D(-h,l,k))
              || (hkl2 == V3D(-h,-l,-k)) || (hkl2 == V3D(h,-l,k)) || (hkl2 == V3D(l,k,-h))
              || (hkl2 == V3D(l,-k,h)) || (hkl2 == V3D(-l,k,h)) || (hkl2 == V3D(-l,-k,-h))
              || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(h,-k,l))
              || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-l,-h,-k)) || (hkl2 == V3D(-l,h,k))
              || (hkl2 == V3D(l,h,-k)) || (hkl2 == V3D(l,-h,k)) || (hkl2 == V3D(-k,-l,-h))
              || (hkl2 == V3D(k,-l,h)) || (hkl2 == V3D(-k,l,h)) || (hkl2 == V3D(k,l,-h))
              || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(-k,h,-l))
              || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-h,-l,k)) || (hkl2 == V3D(h,-l,-k))
              || (hkl2 == V3D(h,l,k)) || (hkl2 == V3D(-h,l,-k)) || (hkl2 == V3D(-l,-k,h))
              || (hkl2 == V3D(-l,k,-h)) || (hkl2 == V3D(l,-k,-h)) || (hkl2 == V3D(l,k,h));
  }

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

