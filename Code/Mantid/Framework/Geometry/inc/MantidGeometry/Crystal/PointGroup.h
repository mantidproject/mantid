#ifndef MANTID_GEOMETRY_POINTGROUP_H_
#define MANTID_GEOMETRY_POINTGROUP_H_
    
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"
#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif
#include <vector>
#include <string>
#include <set>

#include "MantidGeometry/Crystal/SymmetryOperation.h"

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
    enum CrystalSystem {
        Triclinic,
        Monoclinic,
        Orthorhombic,
        Tetragonal,
        Hexagonal,
        Trigonal,
        Cubic
    };

    virtual ~PointGroup() {}
    /// Name of the point group
    virtual std::string getName() const = 0;
    /// Hermann-Mauguin symbol
    std::string getSymbol() const;

    virtual CrystalSystem crystalSystem() const = 0;

    /// Return true if the hkls are in same group
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const = 0;

    /// Returns a vector with all equivalent hkls
    std::vector<Kernel::V3D> getEquivalents(const Kernel::V3D &hkl) const;
    /// Returns the same hkl for all equivalent hkls
    Kernel::V3D getReflectionFamily(const Kernel::V3D &hkl) const;

    /// In this method symmetry operations should be defined. It's called by the factory after construction of the object.
    virtual void init() = 0;

  protected:
    PointGroup(const std::string &symbolHM);

    void setSymmetryOperations(const std::vector<SymmetryOperation> &generators);
    void addSymmetryOperation(const SymmetryOperation &symmetryOperation);
    std::vector<SymmetryOperation> getSymmetryOperations() const;

    std::vector<SymmetryOperation> generateSymmetryOperations(const std::vector<SymmetryOperation> &symmetryOperations);

    std::set<Kernel::V3D> getEquivalentSet(const Kernel::V3D &hkl) const;

    std::vector<SymmetryOperation> m_symmetryOperations;
    std::string m_symbolHM;
  };

  //------------------------------------------------------------------------
  /** -1 (Triclinic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue1 : public PointGroup
  {
  public:
    PointGroupLaue1();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** 1 2/m 1 (Monoclinic, unique axis b) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue2 : public PointGroup
  {
  public:
    PointGroupLaue2();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** 1 1 2/m (Monoclinic, unique axis c) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue3 : public PointGroup
  {
  public:
    PointGroupLaue3();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** mmm (Orthorombic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue4 : public PointGroup
  {
  public:
    PointGroupLaue4();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** 4/m (Tetragonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue5 : public PointGroup
  {
  public:
    PointGroupLaue5();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** 4/mmm (Tetragonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue6 : public PointGroup
  {
  public:
    PointGroupLaue6();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** -3 (Trigonal - Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue7 : public PointGroup
  {
  public:
    PointGroupLaue7();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** -3m1 (Trigonal - Rhombohedral) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue8 : public PointGroup
  {
  public:
    PointGroupLaue8();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** -31m (Trigonal - Rhombohedral) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue9 : public PointGroup
  {
  public:
    PointGroupLaue9();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /**  6/m (Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue10 : public PointGroup
  {
  public:
    PointGroupLaue10();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** 6/mmm (Hexagonal) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue11 : public PointGroup
  {
  public:
    PointGroupLaue11();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** m-3 (Cubic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue12 : public PointGroup
  {
  public:
    PointGroupLaue12();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };

  //------------------------------------------------------------------------
  /** m-3m (Cubic) PointGroup */
  class MANTID_GEOMETRY_DLL PointGroupLaue13 : public PointGroup
  {
  public:
    PointGroupLaue13();
    /// Name of the point group
    virtual std::string getName() const;
    /// Return true if the hkls are equivalent.
    virtual bool isEquivalent(const Kernel::V3D &hkl, const Kernel::V3D &hkl2) const;
    virtual PointGroup::CrystalSystem crystalSystem() const;

    virtual void init();
  };


  /// Shared pointer to a PointGroup
  typedef boost::shared_ptr<PointGroup> PointGroup_sptr;

  MANTID_GEOMETRY_DLL std::vector<PointGroup_sptr> getAllPointGroups();

  typedef std::multimap<PointGroup::CrystalSystem, PointGroup_sptr> PointGroupCrystalSystemMap;
  MANTID_GEOMETRY_DLL PointGroupCrystalSystemMap getPointGroupsByCrystalSystem();



} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_POINTGROUP_H_ */
