#ifndef VTKPEAKMARKERFACTORY_TEST_H_
#define VTKPEAKMARKERFACTORY_TEST_H_

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace ::testing;
using namespace Mantid::VATES;
using Mantid::VATES::vtkPeakMarkerFactory;

class MockPeak : public Peak
{
public:
  MOCK_CONST_METHOD0(getHKL, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQLabFrame, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQSampleFrame, Mantid::Kernel::V3D (void));
};

class MockPeaksWorkspace : public PeaksWorkspace
{
public:
  MOCK_METHOD1(setInstrument, void (Mantid::Geometry::Instrument_const_sptr inst));
  MOCK_METHOD0(getInstrument, Mantid::Geometry::Instrument_const_sptr ());
  MOCK_CONST_METHOD0(clone, Mantid::DataObjects::PeaksWorkspace*());
  MOCK_CONST_METHOD0(getNumberPeaks, int());
  MOCK_METHOD1(removePeak, void (int peakNum) );
  MOCK_METHOD1(addPeak, void (const IPeak& ipeak));
  MOCK_METHOD1(getPeak, Mantid::API::IPeak & (int peakNum));
  MOCK_CONST_METHOD1(getPeak, Mantid::API::IPeak & (int peakNum));
  MOCK_CONST_METHOD2(createPeak, Mantid::API::IPeak* (Mantid::Kernel::V3D QLabFrame, double detectorDistance));
};

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkPeakMarkerFactoryTest: public CxxTest::TestSuite
{

public:

  void do_test(MockPeak & peak1, vtkPeakMarkerFactory::ePeakDimensions dims)
  {
    FakeProgressAction updateProgress;
    boost::shared_ptr<MockPeaksWorkspace> pw_ptr(new MockPeaksWorkspace());
    MockPeaksWorkspace & pw = *pw_ptr;

    //Peaks workspace will return 5 identical peaks
    EXPECT_CALL( pw, getNumberPeaks()).WillOnce(Return(5));
    EXPECT_CALL( pw, getPeak(_)).WillRepeatedly( ReturnRef( peak1 ));

    vtkPeakMarkerFactory factory("signal", dims);
    factory.initialize(pw_ptr);
    vtkDataSet * set = factory.create(updateProgress);
    TS_ASSERT(set);
    TS_ASSERT_EQUALS( set->GetNumberOfPoints(), 5);
    TS_ASSERT_EQUALS(set->GetPoint(0)[0], 1.0);
    TS_ASSERT_EQUALS(set->GetPoint(0)[1], 2.0);
    TS_ASSERT_EQUALS(set->GetPoint(0)[2], 3.0);

    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&pw));
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&peak1));
    set->Delete();
  }

  void test_progress_updates()
  {
    MockPeak peak1;
    EXPECT_CALL( peak1, getQLabFrame()).WillRepeatedly( Return( V3D(1,2,3) ));
    EXPECT_CALL( peak1, getHKL()).Times(AnyNumber());
    EXPECT_CALL( peak1, getQSampleFrame()).Times(AnyNumber());

    MockProgressAction mockProgress;
    //Expectation checks that progress should be >= 0 and <= 100 and called at least once!
    EXPECT_CALL(mockProgress, eventRaised(AllOf(Le(100),Ge(0)))).Times(AtLeast(1));

    boost::shared_ptr<MockPeaksWorkspace> pw_ptr(new MockPeaksWorkspace());
    MockPeaksWorkspace & pw = *pw_ptr;

    //Peaks workspace will return 5 identical peaks
    EXPECT_CALL( pw, getNumberPeaks()).WillRepeatedly(Return(5));
    EXPECT_CALL( pw, getPeak(_)).WillRepeatedly( ReturnRef( peak1 ));

    vtkPeakMarkerFactory factory("signal", vtkPeakMarkerFactory::Peak_in_Q_lab);
    factory.initialize(pw_ptr);
    vtkDataSet * set = factory.create(mockProgress);
    set->Delete();

    TSM_ASSERT("Progress Updates not used as expected.", Mock::VerifyAndClearExpectations(&mockProgress));

  }

  void test_q_lab()
  {
    MockPeak peak1;
    EXPECT_CALL( peak1, getQLabFrame()).Times(5).WillRepeatedly( Return( V3D(1,2,3) ));
    EXPECT_CALL( peak1, getHKL()).Times(0);
    EXPECT_CALL( peak1, getQSampleFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_Q_lab);
  }

  void test_q_sample()
  {
    MockPeak peak1;
    EXPECT_CALL( peak1, getQSampleFrame()).Times(5).WillRepeatedly( Return( V3D(1,2,3) ));
    EXPECT_CALL( peak1, getHKL()).Times(0);
    EXPECT_CALL( peak1, getQLabFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_Q_sample);
  }

  void test_hkl()
  {
    MockPeak peak1;
    EXPECT_CALL( peak1, getHKL()).Times(5).WillRepeatedly( Return( V3D(1,2,3) ));
    EXPECT_CALL( peak1, getQLabFrame()).Times(0);
    EXPECT_CALL( peak1, getQSampleFrame()).Times(0);

    do_test(peak1, vtkPeakMarkerFactory::Peak_in_HKL);
  }

  void testIsValidThrowsWhenNoWorkspace()
  {
    using namespace Mantid::VATES;
    using namespace Mantid::API;

    IMDWorkspace* nullWorkspace = NULL;
    Mantid::API::IMDWorkspace_sptr ws_sptr(nullWorkspace);

    vtkPeakMarkerFactory factory("signal");

    TSM_ASSERT_THROWS("No workspace, so should not be possible to complete initialization.", factory.initialize(ws_sptr), std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    FakeProgressAction progressUpdate;
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.create(progressUpdate), std::runtime_error);
  }

  void testTypeName()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory ("signal");
    TS_ASSERT_EQUALS("vtkPeakMarkerFactory", factory.getFactoryTypeName());
  }

};


#endif
