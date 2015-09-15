#ifndef MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidGeometry/Crystal/PeakTransformFactory.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace SliceViewer {
/// Alias for Vector of Peak Overlay Views
typedef std::vector<boost::shared_ptr<PeakOverlayView>> VecPeakOverlayView;

/// Coordinate System Enum to String.
std::string DLLExport
    coordinateToString(Mantid::Kernel::SpecialCoordinateSystem coordSystem);

/*---------------------------------------------------------
ConcretePeaksPresenter

Concrete implmentation of the Peaks presenter.
----------------------------------------------------------*/
class DLLExport ConcretePeaksPresenter : public PeaksPresenter {
public:
  ConcretePeaksPresenter(
      PeakOverlayViewFactory_sptr viewFactory,
      Mantid::API::IPeaksWorkspace_sptr peaksWS,
      boost::shared_ptr<Mantid::API::MDGeometry> mdWS,
      Mantid::Geometry::PeakTransformFactory_sptr transformFactory);
  void reInitialize(Mantid::API::IPeaksWorkspace_sptr peaksWS);
  virtual ~ConcretePeaksPresenter();
  virtual void update();
  virtual void updateWithSlicePoint(const PeakBoundingBox &slicePoint);
  virtual bool changeShownDim();
  virtual bool isLabelOfFreeAxis(const std::string &label) const;
  SetPeaksWorkspaces presentedWorkspaces() const;
  void setForegroundColor(const QColor);
  void setBackgroundColor(const QColor);
  std::string getTransformName() const;
  void setShown(const bool shown);
  virtual PeakBoundingBox getBoundingBox(const int) const;
  virtual void sortPeaksWorkspace(const std::string &byColumnName,
                                  const bool ascending);
  virtual void setPeakSizeOnProjection(const double fraction);
  virtual void setPeakSizeIntoProjection(const double fraction);
  virtual double getPeakSizeOnProjection() const;
  virtual double getPeakSizeIntoProjection() const;
  virtual void registerOwningPresenter(UpdateableOnDemand *owner);
  virtual bool getShowBackground() const;
  virtual QColor getBackgroundColor() const;
  virtual QColor getForegroundColor() const;
  virtual void zoomToPeak(const int index);
  virtual bool isHidden() const;
  virtual bool contentsDifferent(PeaksPresenter const *  other) const;
  virtual void peakEditMode(EditMode mode);
  virtual bool deletePeaksIn(PeakBoundingBox plotCoordsBox);
  virtual bool addPeakAt(double plotCoordsPointX, double plotCoordsPointY);
  virtual bool hasPeakAddMode() const;

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
  /// Can we add to this peaks workspace
  bool m_hasAddPeaksMode;

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
  void showBackgroundRadius(const bool show);
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
  void setVisiblePeaks(const std::vector<size_t>& indexes);
};


}
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_ */
