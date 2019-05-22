// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKPEAKMARKERFACTORY_TEST_H_
#define VTKPEAKMARKERFACTORY_TEST_H_

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MockObjects.h"

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace ::testing;
using namespace Mantid::VATES;
using Mantid::VATES::vtkPeakMarkerFactory;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockPeakShape : public Peak {
public:
  MOCK_CONST_METHOD0(getHKL, Mantid::Kernel::V3D(void));
  MOCK_CONST_METHOD0(getQLabFrame, Mantid::Kernel::V3D(void));
  MOCK_CONST_METHOD0(getQSampleFrame, Mantid::Kernel::V3D(void));
  MOCK_CONST_METHOD0(getPeakShape, const Mantid::Geometry::PeakShape &(void));
};

class MockPeak : public Peak {
public:
  MOCK_CONST_METHOD0(getHKL, Mantid::Kernel::V3D(void));
  MOCK_CONST_METHOD0(getQLabFrame, Mantid::Kernel::V3D(void));
  MOCK_CONST_METHOD0(getQSampleFrame, Mantid::Kernel::V3D(void));
};

class MockPeaksWorkspace : public PeaksWorkspace {
  using Mantid::DataObjects::PeaksWorkspace::addPeak;

public:
  MOCK_METHOD1(setInstrument,
               void(const Mantid::Geometry::Instrument_const_sptr &inst));
  MOCK_CONST_METHOD0(clone, Mantid::DataObjects::PeaksWorkspace *());
  MOCK_CONST_METHOD0(getNumberPeaks, int());
  MOCK_METHOD1(removePeak, void(int peakNum));
  MOCK_METHOD1(addPeak, void(const IPeak &ipeak));
  MOCK_METHOD1(getPeak, Mantid::DataObjects::Peak &(int peakNum));
  MOCK_CONST_METHOD1(getPeak, const Mantid::DataObjects::Peak &(int peakNum));
};
GNU_DIAG_ON_SUGGEST_OVERRIDE

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkPeakMarkerFactoryTest : public CxxTest::TestSuite {

public:
  void do_test(MockPeak &peak1, vtkPeakMarkerFactory::ePeakDimensions dims) {
    FakeProgressAction updateProgress;
    auto pw_ptr = boost::make_shared<MockPeaksWorkspace>();
    MockPeaksWorkspace &pw = *pw_ptr;

    // Peaks workspace will return 5 identical peaks
    EXPECT_CALL(pw, getNumberPeaks()).WillOnce(Return(5));
    EXPECT_CALL(pw, getPeak(_)).WillRepeatedly(ReturnRef(peak1));

    vtkPeakMarkerFactory factory("signal", dims);
    factory.initialize(pw_ptr);
    auto set = factory.create(updateProgress);

    // As the marker type are three axes(2 points), we expect 5*2*3 points
    // The angle is 0 degrees and the size is 0.3

    TS_ASSERT(set);
    TS_ASSERT_EQUALS(set->GetNumberOfPoints(), 30);

    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&pw));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&peak1));
  }

  void test_progress_updates() {
    MockPeak peak1;
    EXPECT_CALL(peak1, getQLabFrame()).WillRepeatedly(Return(V3D(1, 2, 3)));
    EXPECT_CALL(peak1, getHKL()).Times(AnyNumber());
    EXPECT_CALL(peak1, getQSampleFrame()).Times(AnyNumber());

    MockProgressAction mockProgress;
    // Expectation checks that progress should be >= 0 and <= 100 and called at
    // least once!
    EXPECT_CALL(mockProgress, eventRaised(AllOf(Le(100), Ge(0))))
        .Times(AtLeast(1));

    boost::shared_ptr<MockPeaksWorkspace> pw_ptr =
        boost::make_shared<MockPeaksWorkspace>();
    MockPeaksWorkspace &pw = *pw_ptr;

    // Peaks workspace will return 5 identical peaks
    EXPECT_CALL(pw, getNumberPeaks()).WillRepeatedly(Return(5));
    EXPECT_CALL(pw, getPeak(_)).WillRepeatedly(ReturnRef(peak1));

    vtkPeakMarkerFactory factory("signal", vtkPeakMarkerFactory::Peak_in_Q_lab);
    factory.initialize(pw_ptr);
    auto set = factory.create(mockProgress);

    TSM_ASSERT("Progress Updates not used as expected.",
               Mock::VerifyAndClearExpectations(&mockProgress));
  }

  void test_q_lab() {
    MockPeak peak1;
    EXPECT_CALL(peak1, getQLabFrame())
        .Times(5)
        .WillRepeatedly(Return(V3D(1, 2, 3)));
    EXPECT_CALL(peak1, getHKL()).Times(0);
    EXPECT_CALL(peak1, getQSampleFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_Q_lab);
  }

  void test_q_sample() {
    MockPeak peak1;
    EXPECT_CALL(peak1, getQSampleFrame())
        .Times(5)
        .WillRepeatedly(Return(V3D(1, 2, 3)));
    EXPECT_CALL(peak1, getHKL()).Times(0);
    EXPECT_CALL(peak1, getQLabFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_Q_sample);
  }

  void test_hkl() {
    MockPeak peak1;
    EXPECT_CALL(peak1, getHKL()).Times(5).WillRepeatedly(Return(V3D(1, 2, 3)));
    EXPECT_CALL(peak1, getQLabFrame()).Times(0);
    EXPECT_CALL(peak1, getQSampleFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_HKL);
  }

  void testIsValidThrowsWhenNoWorkspace() {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace *nullWorkspace = nullptr;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkPeakMarkerFactory factory("signal");

    TSM_ASSERT_THROWS(
        "No workspace, so should not be possible to complete initialization.",
        factory.initialize(ws_sptr), const std::runtime_error &);
  }

  void testCreateWithoutInitializeThrows() {
    FakeProgressAction progressUpdate;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.create(progressUpdate), const std::runtime_error &);
  }

  void testTypeName() {
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_EQUALS("vtkPeakMarkerFactory", factory.getFactoryTypeName());
  }

  void testGetPeakRadiusDefault() {
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_EQUALS(-1, factory.getIntegrationRadius());
  }

  void testIsPeaksWorkspaceIntegratedDefault() {
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_EQUALS(false, factory.isPeaksWorkspaceIntegrated());
  }

  void testGetPeakRadiusWhenNotIntegrated() {
    auto mockWorkspace = Mantid::Kernel::make_unique<MockPeaksWorkspace>();
    const double expectedRadius = -1; // The default
    // Note that no PeaksRadius property has been set.

    vtkPeakMarkerFactory factory("signal");
    factory.initialize(
        Mantid::API::IPeaksWorkspace_sptr(std::move(mockWorkspace)));
    TS_ASSERT_EQUALS(expectedRadius, factory.getIntegrationRadius());
  }

  void testIsPeaksWorkspaceIntegratedWhenNotIntegrated() {
    auto mockWorkspace = Mantid::Kernel::make_unique<MockPeaksWorkspace>();
    // Note that no PeaksRadius property has been set.

    vtkPeakMarkerFactory factory("signal");
    factory.initialize(
        Mantid::API::IPeaksWorkspace_sptr(std::move(mockWorkspace)));
    TS_ASSERT_EQUALS(
        false, factory.isPeaksWorkspaceIntegrated()); // false is the default
  }

  void testGetPeakRadiusWhenIntegrated() {
    auto mockWorkspace = Mantid::Kernel::make_unique<MockPeaksWorkspace>();
    const double expectedRadius = 4;
    mockWorkspace->mutableRun().addProperty("PeakRadius", expectedRadius,
                                            true); // Has a PeaksRadius so must
                                                   // have been processed via
                                                   // IntegratePeaksMD

    vtkPeakMarkerFactory factory("signal");
    factory.initialize(
        Mantid::API::IPeaksWorkspace_sptr(std::move(mockWorkspace)));
    TS_ASSERT_EQUALS(expectedRadius, factory.getIntegrationRadius());
  }

  void testIsPeaksWorkspaceIntegratedWhenIntegrated() {
    auto mockWorkspace = Mantid::Kernel::make_unique<MockPeaksWorkspace>();
    const double expectedRadius = 4;
    mockWorkspace->mutableRun().addProperty("PeakRadius", expectedRadius,
                                            true); // Has a PeaksRadius so must
                                                   // have been processed via
                                                   // IntegratePeaksMD

    vtkPeakMarkerFactory factory("signal");
    factory.initialize(
        Mantid::API::IPeaksWorkspace_sptr(std::move(mockWorkspace)));
    TS_ASSERT_EQUALS(true, factory.isPeaksWorkspaceIntegrated());
  }

  void testShapeOfSphere() {
    FakeProgressAction updateProgress;
    auto pw_ptr = boost::make_shared<MockPeaksWorkspace>();
    MockPeaksWorkspace &pw = *pw_ptr;

    double actualRadius = 2.0;
    PeakShapeSpherical sphere(actualRadius,
                              Kernel::SpecialCoordinateSystem::QLab);

    MockPeakShape peak1;
    EXPECT_CALL(pw, getNumberPeaks()).WillOnce(Return(1));
    EXPECT_CALL(pw, getPeak(_)).WillRepeatedly(ReturnRef(peak1));
    EXPECT_CALL(peak1, getQLabFrame()).WillRepeatedly(Return(V3D(0., 0., 0.)));
    EXPECT_CALL(peak1, getPeakShape()).WillRepeatedly(ReturnRef(sphere));

    vtkPeakMarkerFactory factory("signal");
    factory.initialize(pw_ptr);
    auto set = factory.create(updateProgress);

    TS_ASSERT(set);
    TS_ASSERT_EQUALS(set->GetNumberOfPoints(), 300);

    for (vtkIdType i = 0; i < set->GetNumberOfPoints(); ++i) {
      double pt[3];
      set->GetPoint(i, pt);
      double radius = std::sqrt(pt[0] * pt[0] + pt[1] * pt[1] + pt[2] * pt[2]);
      TS_ASSERT_DELTA(radius, actualRadius, 1.0e-5);
    }

    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&pw));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&peak1));
  }

  void testShapeOfEllipsoid() {
    FakeProgressAction updateProgress;
    auto pw_ptr = boost::make_shared<MockPeaksWorkspace>();
    MockPeaksWorkspace &pw = *pw_ptr;

    // rotate in 60 degree increments in the x-y plane.
    for (size_t dir = 0; dir < 6; ++dir) {
      double theta = 2. * M_PI * static_cast<double>(dir) / 6.;

      std::vector<Mantid::Kernel::V3D> directions{
          {cos(theta), -1. * sin(theta), 0.},
          {sin(theta), cos(theta), 0.},
          {0., 0., 1.}};
      std::vector<double> abcRadii{1., 2., 3.};
      // not using these but the constructor requires we set a value.
      std::vector<double> abcRadiiBackgroundInner{1., 2., 3.};
      std::vector<double> abcRadiiBackgroundOuter{1., 2., 3.};

      PeakShapeEllipsoid ellipsoid(
          directions, abcRadii, abcRadiiBackgroundInner,
          abcRadiiBackgroundOuter, Kernel::SpecialCoordinateSystem::QLab);

      MockPeakShape peak1;
      EXPECT_CALL(pw, getNumberPeaks()).WillOnce(Return(1));
      EXPECT_CALL(pw, getPeak(_)).WillRepeatedly(ReturnRef(peak1));
      EXPECT_CALL(peak1, getQLabFrame())
          .WillRepeatedly(Return(V3D(0., 0., 0.)));
      EXPECT_CALL(peak1, getPeakShape()).WillRepeatedly(ReturnRef(ellipsoid));

      vtkPeakMarkerFactory factory("signal");
      factory.initialize(pw_ptr);
      auto set = factory.create(updateProgress);

      TS_ASSERT(set);
      TS_ASSERT_EQUALS(set->GetNumberOfPoints(), 300);
      // Use the standard equation of an ellipsoid to test the resulting
      // workspace.
      // https://en.wikipedia.org/wiki/Ellipsoid
      for (vtkIdType i = 0; i < set->GetNumberOfPoints(); ++i) {
        double pt[3];
        set->GetPoint(i, pt);
        double rot_x = pt[0] * cos(theta) - pt[1] * sin(theta);
        double rot_y = pt[0] * sin(theta) + pt[1] * cos(theta);
        double test = rot_x * rot_x / (abcRadii[0] * abcRadii[0]) +
                      rot_y * rot_y / (abcRadii[1] * abcRadii[1]) +
                      pt[2] * pt[2] / (abcRadii[2] * abcRadii[2]);
        TS_ASSERT_DELTA(test, 1., 1.0e-5);
      }

      TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&pw));
      TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&peak1));
    }
  }
};

#endif
