// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"
#include <boost/regex.hpp>
#include <memory>

namespace Mantid {
namespace Geometry {
/**
@class PeakTransform
Used to remap coordinates into a form consistent with an axis reordering.
*/
class MANTID_GEOMETRY_DLL PeakTransform {
public:
  PeakTransform(std::string xPlotLabel, std::string yPlotLabel, const boost::regex &regexOne,
                const boost::regex &regexTwo, const boost::regex &regexThree);
  virtual ~PeakTransform() = default;
  /// Perform Transform
  virtual Mantid::Kernel::V3D transform(const Mantid::Kernel::V3D &original) const;
  /// Perform Transform
  virtual Mantid::Kernel::V3D transformPeak(const Mantid::Geometry::IPeak &peak) const = 0;
  /// Perform reverse transform.
  Mantid::Kernel::V3D transformBack(const Mantid::Kernel::V3D &transformed) const;
  /// Get a regex to find the axis of the free peak.
  boost::regex getFreePeakAxisRegex() const;
  /// Virtual constructor.
  virtual std::shared_ptr<PeakTransform> clone() const = 0;
  /// Getter for a friendly name to describe the transform type.
  virtual std::string getFriendlyName() const = 0;
  /// Getter for the special coordinate representation of this transform type.
  virtual Mantid::Kernel::SpecialCoordinateSystem getCoordinateSystem() const = 0;

protected:
  PeakTransform(const PeakTransform &) = default;
  std::string m_xPlotLabel;
  std::string m_yPlotLabel;
  // For mapping/orientation from peak coordinates to plot coordinate
  int m_indexOfPlotX;
  int m_indexOfPlotY;
  int m_indexOfPlotZ;
  // For mapping/orientation from plot coordinates to peak coordinate
  int m_indexOfPeakX;
  int m_indexOfPeakY;
  int m_indexOfPeakZ;
  boost::regex m_FirstRegex;
  boost::regex m_SecondRegex;
  boost::regex m_ThirdRegex;
};

/// Typedef for a PeakTransform wrapped in a shared_pointer.
using PeakTransform_sptr = std::shared_ptr<PeakTransform>;
using PeakTransform_const_sptr = std::shared_ptr<const PeakTransform>;

/**
@class PeakTransformException
Exceptions occuring when PeakTransformations cannot be formed.
*/
class MANTID_GEOMETRY_DLL PeakTransformException : public std::exception {
public:
  PeakTransformException() : std::exception() {}
};
} // namespace Geometry
} // namespace Mantid
