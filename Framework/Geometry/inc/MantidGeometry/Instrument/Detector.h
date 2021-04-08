// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidKernel/V3D.h"
#include <string>

namespace Mantid {
namespace Geometry {
class Instrument;

/**
 * This class represents a detector - i.e. a single pixel in an instrument.
 * It is An extension of the ObjectComponent class to add a detector id.

  @class Detector
  @version A
  @author Laurent C Chapon, ISIS RAL
  @date 01/11/2007
 */
class MANTID_GEOMETRY_DLL Detector : public virtual IDetector, public ObjComponent {
public:
  /// A string representation of the component type
  std::string type() const override { return "DetectorComponent"; }

  Detector(const std::string &name, int id, IComponent *parent);
  Detector(const std::string &name, int id, const std::shared_ptr<IObject> &shape, IComponent *parent);
  // functions inherited from IObjectComponent
  Component *clone() const override { return new Detector(*this); }

  // IDetector methods
  Detector *cloneParameterized(const ParameterMap *map) const override { return new Detector(this, map); }
  detid_t getID() const override;
  std::size_t nDets() const override { return 1; } ///< A Detector object represents a single physical detector
  double getDistance(const IComponent &comp) const override;
  double getTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis) const override;
  double getSignedTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis,
                           const Kernel::V3D &instrumentUp) const override;
  double getPhi() const override;
  double getPhiOffset(const double &offset) const override;
  // end IDetector methods
  /** returns the detector's topology, namely, the meaning of the detector's
     angular measurements.
      It is different in cartesian and cylindrical (surrounding the beam)
     coordinate system */
  det_topology getTopology(Kernel::V3D &center) const override;

  const ParameterMap &parameterMap() const override;
  size_t index() const override;

  virtual size_t registerContents(class ComponentVisitor &componentVisitor) const override;

private:
  /// The detector id
  const detid_t m_id;

protected:
  /// Constructor for parametrized version
  Detector(const Detector *base, const ParameterMap *map);
  bool hasDetectorInfo() const;
};

} // namespace Geometry
} // namespace Mantid
