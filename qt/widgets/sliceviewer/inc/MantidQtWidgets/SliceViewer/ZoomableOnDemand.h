// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ZOOMABLEONDEMAND_H_
#define ZOOMABLEONDEMAND_H_

#include "MantidKernel/System.h"

namespace MantidQt {
namespace SliceViewer {
/// Forward dec
class PeaksPresenter;

/** Abstract behavioural type for zooming to a peak region on demand.

 @date 2013-07-11
 */

class DLLExport ZoomableOnDemand {
public:
  /// Zoom to a peak
  virtual void zoomToPeak(PeaksPresenter *const presenter,
                          const int peakIndex) = 0;
  /// Reset/forget zoom
  virtual void resetZoom() = 0;
  /// destructor
  virtual ~ZoomableOnDemand() {}
};
} // namespace SliceViewer
} // namespace MantidQt

#endif /* ZOOMABLEONDEMAND_H_ */
