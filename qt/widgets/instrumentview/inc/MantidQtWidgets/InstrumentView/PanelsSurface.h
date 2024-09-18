// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "UnwrappedSurface.h"

#include <QPolygonF>
#include <optional>

namespace MantidQt {
namespace MantidWidgets {
class PanelsSurface;

struct EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW FlatBankInfo {
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
  /// optional override u, v for the bank
  std::optional<Mantid::Kernel::V2D> bankCentreOverride;
  /// further offset that is applied to bank following projection calculation as
  /// a result of bank arrangement logic
  std::optional<Mantid::Kernel::V2D> bankCentreOffset;
  /// the point on the bank about which rotation occurs during projection
  Mantid::Kernel::V3D refPos;
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
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PanelsSurface : public UnwrappedSurface {
public:
  PanelsSurface(const IInstrumentActor *rootActor, const Mantid::Kernel::V3D &origin, const Mantid::Kernel::V3D &axis,
                const QSize &widgetSize, const bool maintainAspectRatio);
  PanelsSurface() : m_zaxis({0., 0., 1.0}){};
  ~PanelsSurface() override;
  void init() override;
  void project(const size_t detIndex, double &u, double &v, double &uscale, double &vscale) const override;
  void project(const Mantid::Kernel::V3D &position, double &u, double &v, double &uscale,
               double &vscale) const override;
  void resetInstrumentActor(const IInstrumentActor *rootActor) override;

protected:
  void findFlatPanels(size_t rootIndex, std::vector<bool> &visited);

  void processStructured(size_t rootIndex);

  std::optional<size_t> processTubes(size_t rootIndex);

  void processGrid(size_t rootIndex);

  void processUnstructured(size_t rootIndex, std::vector<bool> &visited);

  void rotate(const UnwrappedDetector &udet, Mantid::Kernel::Quat &R) const override;
  // Setup the projection axes
  void setupAxes();
  // Add a flat bank
  void constructFromComponentInfo();
  Mantid::Kernel::Quat calcBankRotation(const Mantid::Kernel::V3D &detPos, Mantid::Kernel::V3D normal) const;
  // Add a detector from an assembly
  void addDetector(size_t detIndex, int bankIndex);
  // Arrange the banks on the projection plane
  void arrangeBanks();
  // Spread the banks over the projection plane
  void spreadBanks();
  // Move the bank centres to the specified position on the projection plane
  void ApplyBankCentreOverrides();
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
  /// Maps detector indices to indices of FlatBankInfos in m_flatBanks
  std::vector<int> m_detector2bankMap;

  friend struct FlatBankInfo;
};

} // namespace MantidWidgets
} // namespace MantidQt
