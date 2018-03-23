#ifndef MANTID_GEOMETRY_PEAKTRANSFORM_H_
#define MANTID_GEOMETRY_PEAKTRANSFORM_H_

#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include "MantidKernel/SpecialCoordinateSystem.h"

namespace Mantid {
namespace Geometry {
/**
@class PeakTransform
Used to remap coordinates into a form consistent with an axis reordering.
*/
class DLLExport PeakTransform {
public:
  PeakTransform(const std::string &xPlotLabel, const std::string &yPlotLabel,
                const boost::regex &regexOne, const boost::regex &regexTwo,
                const boost::regex &regexThree);
  virtual ~PeakTransform() = default;
  /// Perform Transform
  virtual Mantid::Kernel::V3D
  transform(const Mantid::Kernel::V3D &original) const;
  /// Perform Transform
  virtual Mantid::Kernel::V3D
  transformPeak(const Mantid::Geometry::IPeak &peak) const = 0;
  /// Perform reverse transform.
  Mantid::Kernel::V3D
  transformBack(const Mantid::Kernel::V3D &transformed) const;
  /// Get a regex to find the axis of the free peak.
  boost::regex getFreePeakAxisRegex() const;
  /// Virtual constructor.
  virtual boost::shared_ptr<PeakTransform> clone() const = 0;
  /// Getter for a friendly name to describe the transform type.
  virtual std::string getFriendlyName() const = 0;
  /// Getter for the special coordinate representation of this transform type.
  virtual Mantid::Kernel::SpecialCoordinateSystem
  getCoordinateSystem() const = 0;

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
using PeakTransform_sptr = boost::shared_ptr<PeakTransform>;
using PeakTransform_const_sptr = boost::shared_ptr<const PeakTransform>;

/**
@class PeakTransformException
Exceptions occuring when PeakTransformations cannot be formed.
*/
class PeakTransformException : public std::exception {
public:
  PeakTransformException() : std::exception() {}
};
}
}

#endif /* MANTID_GEOMETRY_PEAKTRANSFORM_H_ */
