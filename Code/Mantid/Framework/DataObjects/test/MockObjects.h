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
      Mantid::Geometry::PeakShape*(const std::string& source));
  MOCK_METHOD1(setSuccessor,
      void(boost::shared_ptr<const PeakShapeFactory> successorFactory));
  virtual ~MockPeakShapeFactory() {}
};


class MockPeakShape : public Mantid::Geometry::PeakShape
{
public:

    MOCK_CONST_METHOD0(frame, Mantid::Kernel::SpecialCoordinateSystem() );
    MOCK_CONST_METHOD0(toJSON, std::string() );
    MOCK_CONST_METHOD0(clone, Mantid::Geometry::PeakShape*() );
    MOCK_CONST_METHOD0(algorithmName, std::string() );
    MOCK_CONST_METHOD0(algorithmVersion, int() ) ;
    MOCK_CONST_METHOD0(shapeName, std::string() );

    virtual ~MockPeakShape() {}
};

}
}

#endif /* MOCKOBJECTS_H_ */
