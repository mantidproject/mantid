#ifndef VTKPEAKMARKERFACTORY_TEST_H_
#define VTKPEAKMARKERFACTORY_TEST_H_

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MantidDataObjects/PeaksWorkspace.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace ::testing;
using Mantid::DataObjects::PeaksWorkspace;
using namespace Mantid::VATES;
using Mantid::VATES::vtkPeakMarkerFactory;

class MockPeaksWorkspace : public PeaksWorkspace
{
public:
  MOCK_METHOD1(setInstrument, void (Mantid::Geometry::Instrument_const_sptr inst));
  MOCK_METHOD0(getInstrument, Mantid::Geometry::Instrument_const_sptr ());
  MOCK_CONST_METHOD0(getNumberPeaks, int());
  MOCK_METHOD1(removePeak, void (const int peakNum) );
  MOCK_METHOD1(addPeak, void (const IPeak& ipeak));
  MOCK_METHOD1(getPeak, Mantid::API::IPeak & (const int peakNum));
  MOCK_METHOD2(createPeak, Mantid::API::IPeak* (Mantid::Kernel::V3D QLabFrame, double detectorDistance));
};

//=====================================================================================
// Functional Tests
//=====================================================================================
class vtkPeakMarkerFactoryTest: public CxxTest::TestSuite
{

public:

  void test_full()
  {
    //TODO: Finish this test!

    boost::shared_ptr<MockPeaksWorkspace> pw(new MockPeaksWorkspace());

//    pw->setI
//
//    ON_CALL( *pw, getPeak(_))
//      .WillByDefault(Return(Mantid::DataObjects::Peak()));
//
//    EXPECT_CALL( *pw, getPeak(_))
//      .Times(5);
//
//    pw->getPeak(1);
//
//    vtkPeakMarkerFactory factory("signal");
//    factory.initialize(pw);
//    vtkDataSet * set = factory.create();
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

  void testCreateMeshOnlyThrows()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.createMeshOnly() , std::runtime_error);
  }

  void testCreateScalarArrayThrows()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.createScalarArray() , std::runtime_error);
  }

  void testCreateWithoutInitializeThrows()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory("signal");
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testTypeName()
  {
    using namespace Mantid::VATES;
    vtkPeakMarkerFactory factory ("signal");
    TS_ASSERT_EQUALS("vtkPeakMarkerFactory", factory.getFactoryTypeName());
  }

};


#endif
