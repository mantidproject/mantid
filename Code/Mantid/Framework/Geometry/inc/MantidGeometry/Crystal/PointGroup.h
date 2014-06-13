#ifndef MANTID_GEOMETRY_POINTGROUP_H_
#define MANTID_GEOMETRY_POINTGROUP_H_
    
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif
#include <vector>
#include <string>

namespace Mantid
{
namespace Geometry
{

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
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2) = 0;
  };

  //------------------------------------------------------------------------
  /** -1 (Triclinic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue1 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** 1 2/m 1 (Monoclinic, unique axis b) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue2 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** 1 1 2/m (Monoclinic, unique axis c) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue3 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** mmm (Orthorombic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue4 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** 4/m (Tetragonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue5 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** 4/mmm (Tetragonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue6 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** -3 (Trigonal - Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue7 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** -3m1 (Trigonal - Rhombohedral) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue8 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** -31m (Trigonal - Rhombohedral) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue9 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /**  6/m (Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue10 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** 6/mmm (Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue11 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** m-3 (Cubic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue12 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };

  //------------------------------------------------------------------------
  /** m-3m (Cubic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue13 : public PointGroup
  {
  public:
    /// Name of the point group
    virtual std::string getName();
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(Kernel::V3D hkl, Kernel::V3D hkl2);
  };


  /// Shared pointer to a PointGroup
  typedef boost::shared_ptr<PointGroup> PointGroup_sptr;

  MANTID_GEOMETRY_DLL std::vector<PointGroup_sptr> getAllPointGroups();

} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_POINTGROUP_H_ */
