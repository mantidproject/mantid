// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MONTECARLOTESTING_H
#define MANTID_ALGORITHMS_MONTECARLOTESTING_H

#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>
#include <gmock/gmock.h>

/*
 * A set of testing classes commonly used by the classes involved in
 * the Monte Carlo absorption algorithm
 */
namespace MonteCarloTesting {

// -----------------------------------------------------------------------------
// Mock Random Number Generator
// -----------------------------------------------------------------------------
class MockRNG final : public Mantid::Kernel::PseudoRandomNumberGenerator {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD0(nextValue, double());
  MOCK_METHOD2(nextValue, double(double, double));
  MOCK_METHOD2(nextInt, int(int, int));
  MOCK_METHOD0(restart, void());
  MOCK_METHOD0(save, void());
  MOCK_METHOD0(restore, void());
  MOCK_METHOD1(setSeed, void(size_t));
  MOCK_METHOD2(setRange, void(const double, const double));
  MOCK_CONST_METHOD0(min, double());
  MOCK_CONST_METHOD0(max, double());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

// -----------------------------------------------------------------------------
// Create test samples
// -----------------------------------------------------------------------------
enum class TestSampleType {
  SolidSphere,
  Annulus,
  ThinAnnulus,
  SamplePlusContainer
};

inline std::string annulusXML(double innerRadius, double outerRadius,
                              double height,
                              const Mantid::Kernel::V3D &upAxis) {
  using Mantid::Kernel::V3D;

  // Cylinders oriented along up, with origin at centre of cylinder
  // Assume upAxis is a unit vector
  V3D centre(upAxis);
  centre *= -0.5 * height;
  const std::string inner = ComponentCreationHelper::cappedCylinderXML(
      innerRadius, height, centre, upAxis, "inner");
  const std::string outer = ComponentCreationHelper::cappedCylinderXML(
      outerRadius, height, centre, upAxis, "outer");

  // Combine shapes
  std::ostringstream os;
  os << inner << outer << "<algebra val=\"(outer (# inner))\" />";
  return os.str();
}

inline Mantid::Geometry::IObject_sptr
createAnnulus(double innerRadius, double outerRadius, double height,
              const Mantid::Kernel::V3D &upAxis) {
  using Mantid::Geometry::ShapeFactory;
  return ShapeFactory().createShape(
      annulusXML(innerRadius, outerRadius, height, upAxis));
}

inline Mantid::API::Sample createSamplePlusContainer() {
  using Mantid::API::Sample;
  using Mantid::Geometry::Container;
  using Mantid::Geometry::SampleEnvironment;
  using Mantid::Geometry::ShapeFactory;
  using Mantid::Kernel::Material;
  using Mantid::Kernel::V3D;
  using Mantid::PhysicalConstants::getNeutronAtom;

  // Create an annulus Vanadium can with silicon sample
  const double height(0.05), innerRadius(0.0046), outerRadius(0.005);
  const V3D centre(0, 0, -0.5 * height), upAxis(0, 0, 1);
  // Container
  auto canShape = ShapeFactory().createShape(
      annulusXML(innerRadius, outerRadius, height, upAxis));
  // CSG Object assumed
  if (auto csgObj =
          boost::dynamic_pointer_cast<Mantid::Geometry::CSGObject>(canShape)) {
    csgObj->setMaterial(Material("Vanadium", getNeutronAtom(23), 0.02));
  }
  auto can = boost::make_shared<Container>(canShape);
  auto environment =
      std::make_unique<SampleEnvironment>("Annulus Container", can);
  // Sample volume
  auto sampleCell = ComponentCreationHelper::createCappedCylinder(
      innerRadius, height, centre, upAxis, "sample");
  // CSG Object assumed
  if (auto csgObj = boost::dynamic_pointer_cast<Mantid::Geometry::CSGObject>(
          sampleCell)) {
    csgObj->setMaterial(Material("Si", getNeutronAtom(14), 0.15));
  }

  // Sample object
  Sample testSample;
  testSample.setShape(sampleCell);
  testSample.setEnvironment(std::move(environment));
  return testSample;
}

inline Mantid::API::Sample createTestSample(TestSampleType sampleType) {
  using Mantid::API::Sample;
  using Mantid::Geometry::IObject_sptr;
  using Mantid::Kernel::Material;
  using Mantid::Kernel::V3D;
  using Mantid::PhysicalConstants::getNeutronAtom;

  using namespace Mantid::Geometry;

  Sample testSample;
  if (sampleType == TestSampleType::SamplePlusContainer) {
    testSample = createSamplePlusContainer();
  } else {
    IObject_sptr shape;
    if (sampleType == TestSampleType::SolidSphere) {
      shape = ComponentCreationHelper::createSphere(0.1);
    } else if (sampleType == TestSampleType::Annulus) {
      shape = createAnnulus(0.1, 0.15, 0.15, V3D(0, 0, 1));
    } else if (sampleType == TestSampleType::ThinAnnulus) {
      shape = createAnnulus(0.01, 0.0101, 0.4, V3D(0, 1, 0));
    } else {
      throw std::invalid_argument("Unknown testing shape type requested");
    }
    // CSG Object assumed
    if (auto csgObj =
            boost::dynamic_pointer_cast<Mantid::Geometry::CSGObject>(shape)) {
      csgObj->setMaterial(Material("Vanadium", getNeutronAtom(23), 0.02));
    }
    testSample.setShape(shape);
  }
  return testSample;
}
} // namespace MonteCarloTesting

#endif // MANTID_ALGORITHMS_MONTECARLOTESTING_H
