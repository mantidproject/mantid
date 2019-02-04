// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_FACTORY_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_FACTORY_H_

#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayView.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
// Forward dec.
class IPeak;
} // namespace Geometry
namespace API {
// Forward dec.
class IPeaksWorkspace;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace SliceViewer {
class PeaksPresenter;

/** Abstract view factory. For creating types of IPeakOverlay.

@date 2012-08-24
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlayViewFactory {
public:
  /// Create a peak view from the index of a peak in the peaks workspace
  virtual boost::shared_ptr<PeakOverlayView>
  createView(PeaksPresenter *const presenter,
             Mantid::Geometry::PeakTransform_const_sptr transform) const = 0;
  /// Destructor
  virtual ~PeakOverlayViewFactory() {}
  /// Get the plot x-axis label
  virtual std::string getPlotXLabel() const = 0;
  /// Get the plot y-axis label
  virtual std::string getPlotYLabel() const = 0;
  /// Same factory settings for a different peaks workspace
  virtual void swapPeaksWorkspace(
      boost::shared_ptr<Mantid::API::IPeaksWorkspace> &peaksWS) = 0;
  virtual void getNonOrthogonalInfo(NonOrthogonalAxis &info) = 0;
};

/// Factory Shared Pointer typedef.
using PeakOverlayViewFactory_sptr = boost::shared_ptr<PeakOverlayViewFactory>;
} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_VIEW_FACTORY_H_ */
