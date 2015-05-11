#ifndef MANTID_VATES_CONCRETE_PEAKS_PRESENTER_VSI_H
#define MANTID_VATES_CONCRETE_PEAKS_PRESENTER_VSI_H

#include "MantidKernel/System.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidAPI/PeakTransform.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <vector>

namespace Mantid {
namespace VATES {
class DLLExport ConcretePeaksPresenterVsi : public PeaksPresenterVsi {
public:
  ConcretePeaksPresenterVsi(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
                            ViewFrustum_const_sptr frustum, std::string wsFrame);
  ~ConcretePeaksPresenterVsi();
  virtual Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace() const;
  virtual std::vector<bool> getViewablePeaks() const;
  virtual void updateViewFrustum(ViewFrustum_const_sptr frustum);
  virtual std::string getFrame() const;
  virtual std::string getPeaksWorkspaceName() const;
  virtual void
  getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row,
               Mantid::Kernel::V3D &position, double &radius,
               Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem) const;
  virtual void sortPeaksWorkspace(const std::string &byColumnName,
                                  const bool ascending);

private:
  /// Get the max radius.
  double getMaxRadius(Mantid::Geometry::PeakShape_sptr shape) const;
  /// Viewable Peaks
  mutable std::vector<bool> m_viewablePeaks;
  /// The viewable region
  ViewFrustum_const_sptr m_viewableRegion;
  /// The peaks workspace
  Mantid::API::IPeaksWorkspace_sptr m_peaksWorkspace;
  /// The frame
  std::string m_frame;
};
}
}
#endif