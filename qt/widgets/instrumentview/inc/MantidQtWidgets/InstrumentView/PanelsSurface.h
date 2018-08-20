#ifndef PANELSSURFACE_H
#define PANELSSURFACE_H

#include "UnwrappedSurface.h"

#include <QPolygonF>
#include <boost/optional.hpp>

namespace MantidQt {
namespace MantidWidgets {
class PanelsSurface;

struct FlatBankInfo {
  explicit FlatBankInfo(PanelsSurface *s);
  /// Bank's rotation
  Mantid::Kernel::Quat rotation;
  /// Starting index of bank's detectors in m_unwrappedDetectors vector
  size_t startDetectorIndex;
  /// Ending index of bank's detectors in m_unwrappedDetectors vector (1 past
  /// the last one)
  size_t endDetectorIndex;
  /// Bank's shape
  QPolygonF polygon;
  // translate the bank by a vector
  void translate(const QPointF &shift);

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
 *  - CompAssembly with detectors lying in the same plane
 */
class PanelsSurface : public UnwrappedSurface {
public:
  PanelsSurface(const InstrumentActor *rootActor,
                const Mantid::Kernel::V3D &origin,
                const Mantid::Kernel::V3D &axis);
  ~PanelsSurface() override;
  void init() override;
  void project(const Mantid::Kernel::V3D &, double &, double &, double &,
               double &) const override;

protected:
  boost::optional<std::pair<std::vector<size_t>, Mantid::Kernel::V3D>>
  findFlatPanels(size_t rootIndex, const std::vector<size_t> &children,
                 std::vector<bool> &visited);

  void processStructured(const std::vector<size_t> &children, size_t rootIndex);

  void processTubes(size_t rootIndex, std::vector<bool> &visited);

  std::pair<std::vector<size_t>, Mantid::Kernel::V3D>
  processUnstructured(const std::vector<size_t> &children, size_t rootIndex,
                      std::vector<bool> &visited);

  void rotate(const UnwrappedDetector &udet,
              Mantid::Kernel::Quat &R) const override;
  // Setup the projection axes
  void setupAxes();
  // Add a flat bank
  void addFlatBankOfDetectors(const Mantid::Kernel::V3D &normal,
                              const std::vector<size_t> &detectors,
                              size_t bankIndex);
  void constructFromComponentInfo();
  Mantid::Kernel::Quat calcBankRotation(const Mantid::Kernel::V3D &detPos,
                                        Mantid::Kernel::V3D normal) const;
  // Add a detector from an assembly
  void addDetector(size_t detIndex, const Mantid::Kernel::V3D &refPos,
                   int index, Mantid::Kernel::Quat &rotation);
  // Spread the banks over the projection plane
  void spreadBanks();
  // Find index of the largest bank
  int findLargestBank() const;
  // Is a polygon overlapped with any of the flat banks
  bool isOverlapped(QPolygonF &polygon, int iexclude) const;
  // Remove all found flat banks
  void clearBanks();

protected:
  const Mantid::Kernel::V3D m_pos; ///< Origin (sample position)
  /// The z axis defines the plane of the projection. All flat banks
  /// are rotated to be parallel to this plane.
  const Mantid::Kernel::V3D m_zaxis;
  Mantid::Kernel::V3D m_xaxis;
  Mantid::Kernel::V3D m_yaxis;
  QPointF m_origin; ///< Origin in uv coords

  /// Keep info of the flat banks
  QList<FlatBankInfo *> m_flatBanks;
  /// Maps detector ids to indices of FlatBankInfos in m_flatBanks
  QMap<size_t, int> m_detector2bankMap;

  friend struct FlatBankInfo;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // PANELSSURFACE_H
