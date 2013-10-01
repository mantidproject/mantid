#ifndef PANELSSURFACE_H
#define PANELSSURFACE_H

#include "UnwrappedSurface.h"

struct FlatBankInfo
{
    /// Component id of the bank
    Mantid::Geometry::ComponentID id;
    /// Normal to bank's plane
    Mantid::Kernel::V3D normal;
};

/**
  * @class PanelsSurface
  * @brief Finds all flat banks of detectors and places them side by side.
  *
  * Who qualifies as a flat bank:
  *
  *  - Rectangular detector
  *  - CompAssembly containing an array of ObjCompAssemblies
  *     + all ObjCompAssemblies must be cylinders
  *     + two first ObjCompAssemblies (returned by CompAssembly::getChild(i))
  *       cannot lie on the same line (being parallel is alright)
  */
class PanelsSurface: public UnwrappedSurface
{
public:
  PanelsSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
  void init();

  void project(const Mantid::Kernel::V3D & pos, double & u, double & v, double & uscale, double & vscale) const;

protected:

  void rotate(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const;
  // Find all flat banks of detectors.
  void findFlatBanks();
  // Add a flat bank
  void addFlatBank(Mantid::Geometry::ComponentID id, const Mantid::Kernel::V3D &normal, QList<Mantid::Geometry::ComponentID> objCompAssemblies);

protected:

  const Mantid::Kernel::V3D m_pos;   ///< Origin (sample position)
  /// The z axis defines the plane of the projection. All flat banks
  /// are rotated to be parallel to this plane.
  const Mantid::Kernel::V3D m_zaxis;

  /// Keep info of the flat banks
  QList<FlatBankInfo> m_flatBanks;
  /// Maps detector ids to indices of FlatBankInfos in m_flatBanks
  QMap<Mantid::detid_t,int> m_detector2bankMap;

  friend class FlatBankFinder;

};

#endif // PANELSSURFACE_H
