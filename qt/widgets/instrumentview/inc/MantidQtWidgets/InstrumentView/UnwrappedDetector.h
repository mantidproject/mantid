// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include <limits>
#include <memory>

class QRectF;

namespace Mantid {
namespace Geometry {
class IDetector;
class IObject;
} // namespace Geometry
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {
/**
\class UnwrappedDetector
\brief Class helper for drawing detectors on unwraped surfaces
\date 15 Nov 2010
\author Roman Tolchenov, Tessella plc

This class keeps information used to draw a detector on an unwrapped surface.

*/
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW UnwrappedDetector {
public:
  UnwrappedDetector();
  UnwrappedDetector(const GLColor &color, size_t detIndex);
  UnwrappedDetector(const UnwrappedDetector &other);
  UnwrappedDetector &operator=(const UnwrappedDetector &other);
  bool empty() const;
  QRectF toQRectF() const;
  GLColor color;                                        ///< red, green, blue colour components (0 - 255)
  double u;                                             ///< horizontal "unwrapped" coordinate
  double v;                                             ///< vertical "unwrapped" coordinate
  double width;                                         ///< detector width in units of u
  double height;                                        ///< detector height in units of v
  double uscale;                                        ///< scaling factor in u direction
  double vscale;                                        ///< scaling factor in v direction
  size_t detIndex = std::numeric_limits<size_t>::max(); ///< Detector Index in
  ///< ComponentInfo/DetectorInfo.
};
} // namespace MantidWidgets
} // namespace MantidQt
