#ifndef MANTID_GEOMETRY_POINTGROUP_H_
#define MANTID_GEOMETRY_POINTGROUP_H_
    
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

namespace Mantid
{
namespace Geometry
{

  using Kernel::V3D;
  /** A class containing the Point Groups for a crystal.
   * 
   * @author Vickie Lynch
   * @date 2012-02-02
   */
  class MANTID_GEOMETRY_DLL PointGroup 
  {
  public:
    PointGroup() {}
    virtual ~PointGroup() {}
    /// Name of the point group
    virtual std::string getName() = 0;
    /// Return true if the hkls are in same group
    virtual bool isEquivalent(V3D hkl, V3D hkl2) = 0;
  };

  //------------------------------------------------------------------------
  /** -1 (Triclinic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue1 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "-1 (Triclinic)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,-l)); }  
  };

  //------------------------------------------------------------------------
  /** 1 2/m 1 (Monoclinic, unique axis b) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue2 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "1 2/m 1 (Monoclinic, unique axis b)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,l)); }  
  };

  //------------------------------------------------------------------------
  /** 1 1 2/m (Monoclinic, unique axis c) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue3 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "1 1 2/m (Monoclinic, unique axis c)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)); }  
  };

  //------------------------------------------------------------------------
  /** mmm (Orthorombic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue4 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "mmm (Orthorombic)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l)); }
  };

  //------------------------------------------------------------------------
  /** 4/m (Tetragonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue5 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "4/m (Tetragonal)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-k,h,l)) || (hkl2 == V3D(k,-h,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-k,h,-l)); }
  };

  //------------------------------------------------------------------------
  /** 4/mmm (Tetragonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue6 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "4/mmm (Tetragonal)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-k,h,l)) || (hkl2 == V3D(k,-h,l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-k,h,-l)) || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(k,h,l)); }
  };

  //------------------------------------------------------------------------
  /** -3 (Trigonal - Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue7 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "-3 (Trigonal - Hexagonal)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l)); }
  };

  //------------------------------------------------------------------------
  /** -3m1 (Trigonal - Rhombohedral) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue8 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "-3m1 (Trigonal - Rhombohedral)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
      { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l)) || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l)); }
  };

  //------------------------------------------------------------------------
  /** -31m (Trigonal - Rhombohedral) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue9 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "-31m (Trigonal - Rhombohedral)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l)) || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(h-k,-k,-l)) || (hkl2 == V3D(-h,-h+k,-l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l)) || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(-h+k,k,l)) || (hkl2 == V3D(h,h-k,l)); }
  };

  //------------------------------------------------------------------------
  /**  6/m (Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue10 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "6/m (Hexagonal)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(k,-h+k,l)) || (hkl2 == V3D(h-k,h,l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(-k,h-k,-l)) || (hkl2 == V3D(-h+k,-h,-l)); }
  };

  //------------------------------------------------------------------------
  /** 6/mmm (Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue11 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "6/mmm (Hexagonal)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-k,h-k,l)) || (hkl2 == V3D(-h+k,-h,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(k,-h+k,l)) || (hkl2 == V3D(h-k,h,l)) || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(h-k,-k,-l)) || (hkl2 == V3D(-h,-h+k,-l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(-h+k,k,-l)) || (hkl2 == V3D(h,h-k,-l)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(k,-h+k,-l)) || (hkl2 == V3D(h-k,h,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(-k,h-k,-l)) || (hkl2 == V3D(-h+k,-h,-l)) || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(-h+k,k,l)) || (hkl2 == V3D(h,h-k,l)) || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(h-k,-k,l)) || (hkl2 == V3D(-h,-h+k,l)); }
  };

  //------------------------------------------------------------------------
  /** m-3 (Cubic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue12 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "m-3 (Cubic)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(l,h,k)) || (hkl2 == V3D(l,-h,-k)) || (hkl2 == V3D(-l,-h,k)) || (hkl2 == V3D(-l,h,-k)) || (hkl2 == V3D(k,l,h)) || (hkl2 == V3D(-k,l,-h)) || (hkl2 == V3D(k,-l,-h)) || (hkl2 == V3D(-k,-l,h)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-l,-h,-k)) || (hkl2 == V3D(-l,h,k)) || (hkl2 == V3D(l,h,-k)) || (hkl2 == V3D(l,-h,k)) || (hkl2 == V3D(-k,-l,-h)) || (hkl2 == V3D(k,-l,h)) || (hkl2 == V3D(-k,l,h)) || (hkl2 == V3D(k,l,-h)); }
  };

  //------------------------------------------------------------------------
  /** m-3m (Cubic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue13 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName() { return "m-3m (Cubic)"; }
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(V3D hkl, V3D hkl2) 
     { double h=hkl[0];double k=hkl[1];double l=hkl[2]; return (hkl2 == V3D(h,k,l)) || (hkl2 == V3D(-h,-k,l)) || (hkl2 == V3D(-h,k,-l)) || (hkl2 == V3D(h,-k,-l)) || (hkl2 == V3D(l,h,k)) || (hkl2 == V3D(l,-h,-k)) || (hkl2 == V3D(-l,-h,k)) || (hkl2 == V3D(-l,h,-k)) || (hkl2 == V3D(k,l,h)) || (hkl2 == V3D(-k,l,-h)) || (hkl2 == V3D(k,-l,-h)) || (hkl2 == V3D(-k,-l,h)) || (hkl2 == V3D(k,h,-l)) || (hkl2 == V3D(-k,-h,-l)) || (hkl2 == V3D(k,-h,l)) || (hkl2 == V3D(-k,h,l)) || (hkl2 == V3D(h,l,-k)) || (hkl2 == V3D(-h,l,k)) || (hkl2 == V3D(-h,-l,-k)) || (hkl2 == V3D(h,-l,k)) || (hkl2 == V3D(l,k,-h)) || (hkl2 == V3D(l,-k,h)) || (hkl2 == V3D(-l,k,h)) || (hkl2 == V3D(-l,-k,-h)) || (hkl2 == V3D(-h,-k,-l)) || (hkl2 == V3D(h,k,-l)) || (hkl2 == V3D(h,-k,l)) || (hkl2 == V3D(-h,k,l)) || (hkl2 == V3D(-l,-h,-k)) || (hkl2 == V3D(-l,h,k)) || (hkl2 == V3D(l,h,-k)) || (hkl2 == V3D(l,-h,k)) || (hkl2 == V3D(-k,-l,-h)) || (hkl2 == V3D(k,-l,h)) || (hkl2 == V3D(-k,l,h)) || (hkl2 == V3D(k,l,-h)) || (hkl2 == V3D(-k,-h,l)) || (hkl2 == V3D(k,h,l)) || (hkl2 == V3D(-k,h,-l)) || (hkl2 == V3D(k,-h,-l)) || (hkl2 == V3D(-h,-l,k)) || (hkl2 == V3D(h,-l,-k)) || (hkl2 == V3D(h,l,k)) || (hkl2 == V3D(-h,l,-k)) || (hkl2 == V3D(-l,-k,h)) || (hkl2 == V3D(-l,k,-h)) || (hkl2 == V3D(l,-k,-h)) || (hkl2 == V3D(l,k,h)); }
  };


  /// Shared pointer to a PointGroup
  typedef boost::shared_ptr<PointGroup> PointGroup_sptr;

  MANTID_GEOMETRY_DLL std::vector<PointGroup_sptr> getAllPointGroups();

} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_POINTGROUP_H_ */
