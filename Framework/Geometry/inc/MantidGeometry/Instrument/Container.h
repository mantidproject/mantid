// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_CONTAINER_H_
#define MANTID_GEOMETRY_CONTAINER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
#include <unordered_map>

namespace Mantid {
namespace Geometry {

/**
  Models a Container is used to hold a sample in the beam. It gets most
  of its functionality from wrapped Geometry::IObject but can also hold a
  definition of what the sample geometry itself would be. If the sample shape
  definition is set then we term this a constrained sample geometry.
*/
class MANTID_GEOMETRY_DLL Container final : public IObject {
public:
  using ShapeArgs = std::unordered_map<std::string, double>;

  Container();
  Container(IObject_sptr shape);
  Container(const Container &container);
  Container(std::string xml);

  bool hasSampleShape() const;
  IObject_sptr createSampleShape(const ShapeArgs &args) const;

  void setSampleShape(const std::string &sampleShapeXML);

  bool isValid(const Kernel::V3D &p) const override {
    return m_shape->isValid(p);
  }
  bool isOnSide(const Kernel::V3D &p) const override {
    return m_shape->isOnSide(p);
  }
  bool hasValidShape() const override { return m_shape->hasValidShape(); }

  IObject *clone() const override { return new Container(*this); }

  IObject *cloneWithMaterial(const Kernel::Material &material) const override {
    return m_shape->cloneWithMaterial(material);
  }

  int getName() const override { return m_shape->getName(); }

  size_t interceptSurface(Geometry::Track &t) const override {
    return m_shape->interceptSurface(t);
  }
  double solidAngle(const Kernel::V3D &observer) const override {
    return m_shape->solidAngle(observer);
  }
  double solidAngle(const Kernel::V3D &observer,
                    const Kernel::V3D &scaleFactor) const override {
    return m_shape->solidAngle(observer, scaleFactor);
  }
  double volume() const override { return m_shape->volume(); }
  const BoundingBox &getBoundingBox() const override {
    return m_shape->getBoundingBox();
  }
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) const override {
    m_shape->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
  }

  int getPointInObject(Kernel::V3D &point) const override {
    return m_shape->getPointInObject(point);
  }
  Kernel::V3D generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                    const size_t i) const override {
    return m_shape->generatePointInObject(rng, i);
  }
  Kernel::V3D generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                    const BoundingBox &activeRegion,
                                    const size_t i) const override {
    return m_shape->generatePointInObject(rng, activeRegion, i);
  }

  detail::ShapeInfo::GeometryShape shape() const override {
    return m_shape->shape();
  }

  const detail::ShapeInfo &shapeInfo() const override {
    return m_shape->shapeInfo();
  }

  void GetObjectGeom(detail::ShapeInfo::GeometryShape &type,
                     std::vector<Kernel::V3D> &vectors, double &innerRadius,
                     double &radius, double &height) const override {
    m_shape->GetObjectGeom(type, vectors, innerRadius, radius, height);
  }
  boost::shared_ptr<GeometryHandler> getGeometryHandler() const override {
    return m_shape->getGeometryHandler();
  }

  void draw() const override { m_shape->draw(); }
  void initDraw() const override { m_shape->initDraw(); }

  const Kernel::Material &material() const override {
    return m_shape->material();
  }
  void setID(const std::string &id);
  const std::string &id() const override { return m_shape->id(); }

private:
  IObject_sptr m_shape;
  std::string m_sampleShapeXML;
};

/// Typdef for a shared pointer
using Container_sptr = boost::shared_ptr<Container>;
/// Typdef for a shared pointer to a const object
using Container_const_sptr = boost::shared_ptr<const Container>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_CONTAINER_H_ */
