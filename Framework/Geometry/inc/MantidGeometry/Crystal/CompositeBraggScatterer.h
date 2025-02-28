// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/BraggScatterer.h"
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

/**
    @class CompositeBraggScatterer

    CompositeBraggScatterer accumulates scatterers, for easier calculation
    of structure factors. Scatterers can be added through the method
    addScatterer. The supplied scatterer is not stored directly,
    it is cloned instead, so there is a new instance. The original instance
    is not modified at all.

    For structure factor calculations, all contributions from
    contained scatterers are summed. Contained scatterers may be
    CompositeBraggScatterers themselves, so it's possible to build up elaborate
    structures.

    There are two ways of creating instances of CompositeBraggScatterer. The
    first possibility is to use BraggScattererFactory, just like for other
    implementations of BraggScatterer. Additionally there is a static method
    CompositeBraggScatterer::create, which creates a composite scatterer of
    the supplied vector of scatterers.

    CompositeBraggScatterer does not declare any methods by itself, instead it
    exposes some properties of the contained scatterers (those which were marked
    using exposePropertyToComposite). When these properties are set,
    their values are propagated to all members of the composite. The default
    behavior when new properties are declared in subclasses of BraggScatterer is
    not to expose them in this way.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 21/10/2014
  */
class CompositeBraggScatterer;

using CompositeBraggScatterer_sptr = std::shared_ptr<CompositeBraggScatterer>;

class MANTID_GEOMETRY_DLL CompositeBraggScatterer : public BraggScatterer {
public:
  CompositeBraggScatterer();

  static CompositeBraggScatterer_sptr create();
  static CompositeBraggScatterer_sptr create(const std::vector<BraggScatterer_sptr> &scatterers);

  std::string name() const override { return "CompositeBraggScatterer"; }
  BraggScatterer_sptr clone() const override;

  virtual void addScatterer(const BraggScatterer_sptr &scatterer);
  void addScatterers(const std::vector<BraggScatterer_sptr> &scatterer);
  void setScatterers(const std::vector<BraggScatterer_sptr> &scatterers);
  size_t nScatterers() const;
  BraggScatterer_sptr getScatterer(size_t i) const;
  const std::vector<BraggScatterer_sptr> &getScatterers() const;
  void removeScatterer(size_t i);
  void removeAllScatterers();

  StructureFactor calculateStructureFactor(const Kernel::V3D &hkl) const override;

protected:
  void afterPropertySet(const std::string &propertyName) override;
  void propagateProperty(const std::string &propertyName);
  void propagatePropertyToScatterer(BraggScatterer_sptr &scatterer, const std::string &propertyName,
                                    const std::string &propertyValue);

  void addScattererImplementation(const BraggScatterer_sptr &scatterer);
  void removeScattererImplementation(size_t i);

  void redeclareProperties();
  std::map<std::string, size_t> getPropertyCountMap() const;

  std::vector<BraggScatterer_sptr> m_scatterers;
};
} // namespace Geometry
} // namespace Mantid
