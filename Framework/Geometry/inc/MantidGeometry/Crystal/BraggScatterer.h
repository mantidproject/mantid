// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

#include <complex>
#include <memory>

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace Geometry {

using StructureFactor = std::complex<double>;

class BraggScatterer;

using BraggScatterer_sptr = std::shared_ptr<BraggScatterer>;

/**
    @class BraggScatterer

    BraggScatterer is a general interface for representing scatterers
    in the unit cell of a periodic structure. Since there are many possibilities
    of modelling scatterers, BraggScatterer is derived from PropertyManager.
    This way, new scatterers with very different parameters can be
    added easily.

    New implementations must override the declareProperties method and
    define any parameters there. For most applications it should be easier
    to inherit from BraggScattererInCrystalStructure, which provides some
    default properties that are useful in many cases. CompositeBraggScatterer
    is designed to combine several scatterers.

    CompositeBraggScatterer does not declare any properties by itself. For
    some properties it makes sense to be equal for all scatterers in the
    composite. This behavior can be achieved by calling the method
    makePropertyPropagating after it has been declared. Examples are
    the UnitCell and SpaceGroup properties in BraggScattererInCrystalStructure.

    Construction of concrete scatterers is done through ScattererFactory.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 20/10/2014
  */
class MANTID_GEOMETRY_DLL BraggScatterer : public Kernel::PropertyManager {
public:
  BraggScatterer();

  void initialize();
  bool isInitialized();

  virtual std::string name() const = 0;
  virtual BraggScatterer_sptr clone() const = 0;

  virtual StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const = 0;
  double calculateFSquared(const Kernel::V3D &hkl) const;

  bool isPropertyExposedToComposite(const std::string &propertyName) const;
  bool isPropertyExposedToComposite(Kernel::Property *property) const;

protected:
  /// Base implementation does nothing - for implementing classes only.
  virtual void declareProperties() {}

  void exposePropertyToComposite(const std::string &propertyName);
  void unexposePropertyFromComposite(const std::string &propertyName);

  const std::string &getPropagatingGroupName() const;

private:
  std::string m_propagatingGroupName;
  bool m_isInitialized;
};

} // namespace Geometry
} // namespace Mantid
