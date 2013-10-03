#ifndef PANELSSURFACE_H
#define PANELSSURFACE_H

#include "UnwrappedSurface.h"

#include <QPolygonF>

class PanelsSurface;

struct FlatBankInfo
{
    FlatBankInfo(PanelsSurface *s):surface(s){}
    /// Component id of the bank
    Mantid::Geometry::ComponentID id;
    /// Bank's rotation
    Mantid::Kernel::Quat rotation;
    /// Bounding rect of the projection
    //RectF rect;
    /// Starting index of bank's detectors in m_unwrappedDetectors vector
    size_t startDetectorIndex;
    /// Ending index of bank's detectors in m_unwrappedDetectors vector (1 past the last one)
    size_t endDetectorIndex;
    // translate the bank by a vector
    void translate(const QPointF &shift);
    QPolygonF polygon;
private:
    PanelsSurface *surface;
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
  ~PanelsSurface();
  void init();
  void project(const Mantid::Kernel::V3D & pos, double & u, double & v, double & uscale, double & vscale) const;

protected:

  void rotate(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const;
  // Setup the projection axes
  void setupAxes();
  // Find all flat banks of detectors.
  void findFlatBanks();
  // Add a flat bank
  void addFlatBank(Mantid::Geometry::ComponentID id, const Mantid::Kernel::V3D &normal, QList<Mantid::Geometry::ComponentID> objCompAssemblies);
  // Calculate bank rotation
  Mantid::Kernel::Quat calcBankRotation( const Mantid::Kernel::V3D &detPos, Mantid::Kernel::V3D normal ) const;
  // Spread the banks over the projection plane
  void spreadBanks();
  // Find index of the largest bank
  int findLargestBank() const;
  bool isOverlapped( QPolygonF &rect, int iexclude ) const;
  void clearBanks();

protected:

  const Mantid::Kernel::V3D m_pos;   ///< Origin (sample position)
  /// The z axis defines the plane of the projection. All flat banks
  /// are rotated to be parallel to this plane.
  const Mantid::Kernel::V3D m_zaxis;
  Mantid::Kernel::V3D m_xaxis;
  Mantid::Kernel::V3D m_yaxis;
  QPointF m_origin; ///< Origin in uv coords

  /// Keep info of the flat banks
  QList<FlatBankInfo*> m_flatBanks;
  /// Maps detector ids to indices of FlatBankInfos in m_flatBanks
  //QMap<Mantid::detid_t,int> m_detector2bankMap;

  friend class FlatBankFinder;
  friend class FlatBankInfo;

};

#endif // PANELSSURFACE_H
