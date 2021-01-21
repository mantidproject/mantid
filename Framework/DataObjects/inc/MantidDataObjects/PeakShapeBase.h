// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"

#include <string>

namespace Json {
// Forward declaration
class Value;
} // namespace Json

namespace Mantid {
namespace DataObjects {

/** PeakShapeBase : Base class for concrete PeakShapes containing common code.
 */

class DLLExport PeakShapeBase : public Mantid::Geometry::PeakShape {

public:
  /// Constructor
  PeakShapeBase(Kernel::SpecialCoordinateSystem frame, std::string algorithmName = std::string(),
                int algorithmVersion = -1);
  /// Get the coordinate frame
  Kernel::SpecialCoordinateSystem frame() const override;
  /// Get the name of the algorithm used to make this shape
  std::string algorithmName() const override;
  /// Get the version of the algorithm used to make this shape
  int algorithmVersion() const override;

protected:
  /// Copy constructor
  PeakShapeBase(const PeakShapeBase &) = default;

  /// Assignment operator
  PeakShapeBase &operator=(const PeakShapeBase &) = default;

  /// Special coordinate system
  Mantid::Kernel::SpecialCoordinateSystem m_frame;

  /// Generating algorithm name
  std::string m_algorithmName;

  /// Generating algorithm version
  int m_algorithmVersion;

  /// Build common parts of outgoing JSON serialization
  void buildCommon(Json::Value &root) const;

  /// Equals operator
  bool operator==(const PeakShapeBase &other) const;
};

} // namespace DataObjects
} // namespace Mantid
