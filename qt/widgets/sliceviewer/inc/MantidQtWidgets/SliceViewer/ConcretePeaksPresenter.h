// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidGeometry/Crystal/PeakTransformFactory.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayViewFactory.h"
#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace MantidQt {
namespace SliceViewer {
/// Alias for Vector of Peak Overlay Views
using VecPeakOverlayView = std::vector<boost::shared_ptr<PeakOverlayView>>;

/// Coordinate System Enum to String.
std::string DLLExport
coordinateToString(Mantid::Kernel::SpecialCoordinateSystem coordSystem);

/*---------------------------------------------------------
ConcretePeaksPresenter

Concrete implmentation of the Peaks presenter.
----------------------------------------------------------*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER ConcretePeaksPresenter
    : public PeaksPresenter {
public:
  ConcretePeaksPresenter(
      PeakOverlayViewFactory_sptr viewFactory,
      Mantid::API::IPeaksWorkspace_sptr peaksWS,
      boost::shared_ptr<Mantid::API::MDGeometry> mdWS,
      Mantid::Geometry::PeakTransformFactory_sptr transformFactory);
  void reInitialize(Mantid::API::IPeaksWorkspace_sptr peaksWS) override;
  ~ConcretePeaksPresenter() override;
  void setNonOrthogonal(bool nonOrthogonalEnabled) override;
  bool changeShownDim(size_t dimX, size_t dimY) override;
  void update() override;
  void updateWithSlicePoint(const PeakBoundingBox &slicePoint) override;
  bool isLabelOfFreeAxis(const std::string &label) const override;
  SetPeaksWorkspaces presentedWorkspaces() const override;
  void setForegroundColor(const PeakViewColor /*color*/) override;
  void setBackgroundColor(const PeakViewColor /*color*/) override;
  std::string getTransformName() const override;
  void setShown(const bool shown) override;
  PeakBoundingBox getBoundingBox(const int /*peakIndex*/) const override;
  void setPeakSizeOnProjection(const double fraction) override;
  void setPeakSizeIntoProjection(const double fraction) override;
  double getPeakSizeOnProjection() const override;
  double getPeakSizeIntoProjection() const override;
  void registerOwningPresenter(UpdateableOnDemand *owner) override;
  bool getShowBackground() const override;
  PeakViewColor getBackgroundPeakViewColor() const override;
  PeakViewColor getForegroundPeakViewColor() const override;
  void zoomToPeak(const int index) override;
  bool isHidden() const override;
  bool contentsDifferent(PeaksPresenter const *other) const override;
  void peakEditMode(EditMode mode) override;
  bool deletePeaksIn(PeakBoundingBox plotCoordsBox) override;
  bool addPeakAt(double plotCoordsPointX, double plotCoordsPointY) override;

private:
  /// Peak overlay view.
  PeakOverlayView_sptr m_viewPeaks;
  /// View factory
  boost::shared_ptr<PeakOverlayViewFactory> m_viewFactory;
  /// Peaks workspace.
  boost::shared_ptr<const Mantid::API::IPeaksWorkspace> m_peaksWS;
  /// Transform factory
  boost::shared_ptr<Mantid::Geometry::PeakTransformFactory> m_transformFactory;
  /// Peak transformer
  Mantid::Geometry::PeakTransform_sptr m_transform;
  /// current slicing point.
  PeakBoundingBox m_slicePoint;
  /// Viewable Peaks
  std::vector<bool> m_viewablePeaks;
  /// Owning presenter.
  UpdateableOnDemand *m_owningPresenter;
  /// Flag to indicate that this is hidden.
  bool m_isHidden;
  /// Flag to indicate the current edit mode.
  EditMode m_editMode;
  /// Flag to indicate slice viewer in in non-orthogonal mode
  bool m_nonOrthogonalMode;
  /// Non orthogonal axis information
  NonOrthogonalAxis m_axisData;
  /// Configure peak transformations
  bool configureMappingTransform();
  /// Hide all views
  void hideAll();
  /// Show all views
  void showAll();
  /// Determine wheter a dimension name corresponds to the free axis for the
  /// peaks workspace.
  bool isDimensionNameOfFreeAxis(const std::string &name) const;
  /// Switch between showing background radius or not
  void showBackgroundRadius(const bool show) override;
  /// Produce the views from the PeaksWorkspace
  void produceViews();
  /// Check workspace compatibilities.
  void checkWorkspaceCompatibilities(
      boost::shared_ptr<Mantid::API::MDGeometry> mdWS);
  /// Find peaks interacting with the slice and update the view.
  void doFindPeaksInRegion();
  /// make owner update.
  void informOwnerUpdate();
  /// initialize the setup
  void initialize();
  /// Find visible peak indexes.
  std::vector<size_t> findVisiblePeakIndexes(const PeakBoundingBox &box);
  /// Set the visible peak list.
  void setVisiblePeaks(const std::vector<size_t> &indexes);
};
} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_ */
