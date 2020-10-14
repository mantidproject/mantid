// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include "MantidKernel/V2D.h"
#include <memory>

namespace MantidQt {
namespace SliceViewer {
/// Forward dec
class PeakBoundingBox;

/** Abstract view in Representing a view that can be zoomed in upon.

@date 2013-01-08
*/
class DLLExport ZoomablePeaksView {
public:
  /// Zoom to a peak position provided by a boundary rectangle in the windows
  /// coordinate system.
  virtual void zoomToRectangle(const PeakBoundingBox &) = 0;
  /// Zoom out
  virtual void resetView() = 0;
  /// Detach
  virtual void detach() = 0;
  /// Destructor
  virtual ~ZoomablePeaksView() {}
};
} // namespace SliceViewer
} // namespace MantidQt
