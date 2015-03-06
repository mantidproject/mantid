#ifndef CONCRETE_PEAKS_PRESENTER_VSI_TEST_H_
#define CONCRETE_PEAKS_PRESENTER_VSI_TEST_H_

#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidKernel/V3D.h"
#include "MockObjects.h"
#include <boost/shared_ptr.hpp>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>


#include <string>

using namespace ::testing;
using namespace Mantid::VATES;

class MockPeak : public Mantid::DataObjects::Peak
{
public:
  MOCK_CONST_METHOD0(getHKL, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQLabFrame, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQSampleFrame, Mantid::Kernel::V3D (void));
};

class MockPeaksWorkspace : public Mantid::DataObjects::PeaksWorkspace
{
public:
  MOCK_CONST_METHOD0(getSpecialCoordinateSystem, Mantid::Kernel::SpecialCoordinateSystem());
  MOCK_CONST_METHOD0(getNumberPeaks, int());
  MOCK_METHOD1(getPeak, Mantid::DataObjects::Peak & (int peakNum));
  MOCK_CONST_METHOD2(createPeak, Mantid::API::IPeak* (Mantid::Kernel::V3D QLabFrame, double detectorDistance));
};

class ConcretePeaksPresenterVsiTest : public CxxTest::TestSuite {
public:
  void testSetupPresenterCorrectly() {
    // Arrange
    std::string frame = "testFrame";
    std::string name = "name";
    
    LeftPlane left(1.0, 0.0, 0.0, 1.0);

    RightPlane right(-1.0, 0.0, 0.0, 1.0);
    BottomPlane bottom(0.0, 1.0, 0.0, 1.0);
    TopPlane top(0.0, -1.0, 0.0, 1.0);
    FarPlane farPlane(0.0, 0.0, 1.0, 1.0);
    NearPlane nearPlane(0.0, 0.0, -1.0,1.0);
    ViewFrustum frustum(left, right, bottom, top, farPlane, nearPlane);

    boost::shared_ptr<MockPeaksWorkspace> pw_ptr(new MockPeaksWorkspace());
    MockPeaksWorkspace & pw = *pw_ptr;
    // Act
    ConcretePeaksPresenterVsi presenter(pw_ptr, frustum, frame);

    // Assert
    TSM_ASSERT_EQUALS("Should have recorded the frame", presenter.getFrame(), frame);
  }

  void testCorrectPeaksInfoIsExtractedForValidRow() {
    // Arrange
    std::string frame = "Q_LAB";
    
    LeftPlane left(1.0, 0.0, 0.0, 1.0);
    RightPlane right(-1.0, 0.0, 0.0, 1.0);
    BottomPlane bottom(0.0, 1.0, 0.0, 1.0);
    TopPlane top(0.0, -1.0, 0.0, 1.0);
    FarPlane farPlane(0.0, 0.0, 1.0, 1.0);
    NearPlane nearPlane(0.0, 0.0, -1.0,1.0);
    ViewFrustum frustum(left, right, bottom, top, farPlane, nearPlane);

    Mantid::Kernel::V3D coordinate(1,0,0);
    double peakRadius = 10;
    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::SpecialCoordinateSystem::QSample;
    Mantid::Geometry::PeakShape_sptr shape(new Mantid::DataObjects::PeakShapeSpherical(peakRadius, coordinateSystem, "test", 1));
    MockPeak peak;
    peak.setPeakShape(shape);
    EXPECT_CALL(peak, getQLabFrame()).WillOnce(Return(coordinate));
    EXPECT_CALL(peak, getHKL()).Times(0);
    EXPECT_CALL(peak, getQSampleFrame()).Times(0);


    boost::shared_ptr<MockPeaksWorkspace> pw_ptr(new MockPeaksWorkspace());
    MockPeaksWorkspace & pw = *pw_ptr;
    EXPECT_CALL(pw, getSpecialCoordinateSystem()).WillOnce(Return(coordinateSystem));
    EXPECT_CALL(pw, getPeak(_)).WillOnce(ReturnRef(peak));

    // Act
    ConcretePeaksPresenterVsi presenter(pw_ptr, frustum, frame);
    double radius = 0;;
    Mantid::Kernel::V3D coord(0,0,0);
    presenter.getPeaksInfo(pw_ptr,0,coord, radius);
  }
};

#endif