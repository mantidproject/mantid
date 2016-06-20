#ifndef MOCKOBJECTS_H_
#define MOCKOBJECTS_H_

#include <gmock/gmock.h>
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidDataObjects/PeakShapeFactory.h"
#include "MantidGeometry/Crystal/PeakShape.h"

namespace Mantid {
namespace DataObjects {

class MockPeakShapeFactory : public PeakShapeFactory {
public:
  MOCK_CONST_METHOD1(create,
                     Mantid::Geometry::PeakShape *(const std::string &source));
  MOCK_METHOD1(
      setSuccessor,
      void(boost::shared_ptr<const PeakShapeFactory> successorFactory));
  ~MockPeakShapeFactory() override {}
};

#if defined(GCC_VERSION) && GCC_VERSION >= 50000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif

class MockPeakShape : public Mantid::Geometry::PeakShape {
public:
  MOCK_CONST_METHOD0(frame, Mantid::Kernel::SpecialCoordinateSystem());
  MOCK_CONST_METHOD0(toJSON, std::string());
  MOCK_CONST_METHOD0(clone, Mantid::Geometry::PeakShape *());
  MOCK_CONST_METHOD0(algorithmName, std::string());
  MOCK_CONST_METHOD0(algorithmVersion, int());
  MOCK_CONST_METHOD0(shapeName, std::string());
  MOCK_CONST_METHOD1(
      radius, boost::optional<double>(Mantid::Geometry::PeakShape::RadiusType));
  ~MockPeakShape() override {}
};
}
}

#if defined(GCC_VERSION) && GCC_VERSION >= 50000
#pragma GCC diagnostic pop
#endif

#endif /* MOCKOBJECTS_H_ */
