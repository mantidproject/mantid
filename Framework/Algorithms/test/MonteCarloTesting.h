#ifndef MANTID_ALGORITHMS_MONTECARLOTESTING_H
#define MANTID_ALGORITHMS_MONTECARLOTESTING_H

#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

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
  GCC_DIAG_OFF_SUGGEST_OVERRIDE
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
  GCC_DIAG_ON_SUGGEST_OVERRIDE
};

// -----------------------------------------------------------------------------
// Create test samples
// -----------------------------------------------------------------------------
enum class TestSampleType { SolidSphere, Annulus, SamplePlusContainer };

inline std::string annulusXML(double innerRadius, double outerRadius,
                              double height,
                              const Mantid::Kernel::V3D &upAxis) {
  using Mantid::Kernel::V3D;

  // Cylinders oriented along up, with origin at centre of cylinder
  const V3D centre(0, 0, -0.5 * height);
  const std::string inner = ComponentCreationHelper::cappedCylinderXML(
      innerRadius, height, centre, upAxis, "inner");
  const std::string outer = ComponentCreationHelper::cappedCylinderXML(
      outerRadius, height, centre, upAxis, "outer");

  // Combine shapes
  std::ostringstream os;
  os << inner << outer << "<algebra val=\"(outer (# inner))\" />";
  return os.str();
}

inline Mantid::Geometry::Object_sptr
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
  auto can = ShapeFactory().createShape<Container>(
      annulusXML(innerRadius, outerRadius, height, upAxis));
  can->setMaterial(Material("Vanadium", getNeutronAtom(23), 0.02));
  auto environment =
      Mantid::Kernel::make_unique<SampleEnvironment>("Annulus Container", can);
  // Sample volume
  auto sampleCell = ComponentCreationHelper::createCappedCylinder(
      innerRadius, height, centre, upAxis, "sample");
  sampleCell->setMaterial(Material("Si", getNeutronAtom(14), 0.15));

  // Sample object
  Sample testSample;
  testSample.setShape(*sampleCell);
  testSample.setEnvironment(environment.release());
  return testSample;
}

inline Mantid::API::Sample createTestSample(TestSampleType sampleType) {
  using Mantid::API::Sample;
  using Mantid::Kernel::Material;
  using Mantid::Kernel::V3D;
  using Mantid::Geometry::Object_sptr;
  using Mantid::PhysicalConstants::getNeutronAtom;

  using namespace Mantid::Geometry;

  Sample testSample;
  if (sampleType == TestSampleType::SamplePlusContainer) {
    testSample = createSamplePlusContainer();
  } else {
    Object_sptr shape;
    if (sampleType == TestSampleType::SolidSphere) {
      shape = ComponentCreationHelper::createSphere(0.1);
    } else if (sampleType == TestSampleType::Annulus) {
      shape = createAnnulus(0.1, 0.15, 0.15, V3D(0, 0, 1));
    } else {
      throw std::invalid_argument("Unknown testing shape type requested");
    }
    shape->setMaterial(Material("Vanadium", getNeutronAtom(23), 0.02));
    testSample.setShape(*shape);
  }
  return testSample;
}
}

#endif // MANTID_ALGORITHMS_MONTECARLOTESTING_H
