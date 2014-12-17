#ifndef MANTID_API_PEAKTRANSFORM_H_
#define MANTID_API_PEAKTRANSFORM_H_

#include "MantidKernel/V3D.h"
#include "MantidAPI/IPeak.h"
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include "MantidAPI/SpecialCoordinateSystem.h"

namespace Mantid {
namespace API {
/**
@class PeakTransform
Used to remap coordinates into a form consistent with an axis reordering.
*/
class DLLExport PeakTransform {
public:
  PeakTransform(const std::string &xPlotLabel, const std::string &yPlotLabel,
                const boost::regex &regexOne, const boost::regex &regexTwo,
                const boost::regex &regexThree);
  virtual ~PeakTransform();
  /// Perform Transform
  virtual Mantid::Kernel::V3D
  transform(const Mantid::Kernel::V3D &original) const;
  /// Perform Transform
  virtual Mantid::Kernel::V3D
  transformPeak(const Mantid::API::IPeak &peak) const = 0;
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
  virtual Mantid::API::SpecialCoordinateSystem getCoordinateSystem() const = 0;

protected:
  PeakTransform(const PeakTransform &other);
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
typedef boost::shared_ptr<PeakTransform> PeakTransform_sptr;
typedef boost::shared_ptr<const PeakTransform> PeakTransform_const_sptr;

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

#endif /* MANTID_API_CONCRETEPEAKSPRESENTER_H_ */
