// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {

using VecV3D = std::vector<Mantid::Kernel::V3D>;
using VecVecV3D = std::vector<VecV3D>;

/** PeaksIntersection : Abstract base algorithm class for algorithms that
  identify peaks interacting with one or more surfaces
  i.e. a flat surface or a box made out of flat surfaces.
*/
class MANTID_CRYSTAL_DLL PeaksIntersection : public API::Algorithm {
public:
  static std::string detectorSpaceFrame();
  static std::string qLabFrame();
  static std::string qSampleFrame();
  static std::string hklFrame();
  /// Number of surface faces that make up this object.
  virtual int numberOfFaces() const = 0;

protected:
  /// Initalize the common properties.
  void initBaseProperties();

  /// Run the algorithm.
  void executePeaksIntersection(const bool checkPeakExtents = true);

  /// Get the peak radius.
  double getPeakRadius() const;

private:
  /// Validate the input extents.
  virtual void validateExtentsInput() const = 0;
  /// Create all faces.
  virtual VecVecV3D createFaces() const = 0;
  /// Check that a point is outside any of the extents
  virtual bool pointOutsideAnyExtents(const Mantid::Kernel::V3D &testPoint) const = 0;
  /// Check that a point is inside ALL of the extents
  virtual bool pointInsideAllExtents(const Mantid::Kernel::V3D &testPoints,
                                     const Mantid::Kernel::V3D &peakCenter) const = 0;

  /// Verfifies that the normals have been set up correctly such that the touch
  /// point falls onto the plane. Use for debugging.
  virtual void checkTouchPoint(const Mantid::Kernel::V3D &touchPoint, const Mantid::Kernel::V3D &normal,
                               const Mantid::Kernel::V3D &faceVertex) const = 0;

  // The peak radius.
  double m_peakRadius = 0.0;
};

} // namespace Crystal
} // namespace Mantid
