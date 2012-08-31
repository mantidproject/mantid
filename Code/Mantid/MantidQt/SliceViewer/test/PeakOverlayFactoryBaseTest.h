#ifndef SLICE_VIEWER_PEAKOVERLAYFACTORY_TEST_H_
#define SLICE_VIEWER_PEAKOVERLAYFACTORY_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/FirstExperimentInfoQuery.h"
#include "MantidQtSliceViewer/PeakOverlayFactoryBase.h"


using namespace MantidQt::SliceViewer;
using namespace Mantid;
using namespace testing;

class PeakOverlayFactoryBaseTest : public CxxTest::TestSuite
{
private:

  /**
  Testint Type derived from the base type. All tests will use this.
  */
  class MockPeakOverlayFactory : public PeakOverlayFactoryBase
  {
  public:
    MockPeakOverlayFactory(const FirstExperimentInfoQuery& query) :  PeakOverlayFactoryBase(query){}
    MOCK_CONST_METHOD3(createViewAtPoint, boost::shared_ptr<PeakOverlayView>(const Mantid::Kernel::V3D&, const double&, const bool));
    ~MockPeakOverlayFactory(){}
  };

  /*------------------------------------------------------------
  Mock Query
  ------------------------------------------------------------*/
  class MockFirstExperimentInfoQuery : public FirstExperimentInfoQuery
  {
  public:
    MOCK_CONST_METHOD0(hasOrientedLattice, bool());
    MOCK_CONST_METHOD0(hasRotatedGoniometer, bool());
    ~MockFirstExperimentInfoQuery(){}
  };

  /*------------------------------------------------------------
  Mock Peak Overlay View
  ------------------------------------------------------------*/
  class MockPeakOverlayView : public PeakOverlayView
  {
  public:
    MOCK_METHOD1(setPlaneDistance, void(const double&));
    MOCK_METHOD0(updateView, void());
    MOCK_METHOD1(setSlicePoint, void(const double&));
    MOCK_METHOD0(hideView, void());
    MOCK_METHOD1(setNormalisation, void(const double&));
    ~MockPeakOverlayView(){}
  };

  /*------------------------------------------------------------
  Mock Peak
  ------------------------------------------------------------*/
  class MockIPeak : public Mantid::API::IPeak 
  {
 public:
  MOCK_METHOD1(setInstrument,
      void(Geometry::Instrument_const_sptr inst));
  MOCK_CONST_METHOD0(getDetectorID,
      int());
  MOCK_METHOD1(setDetectorID,
      void(int m_DetectorID));
  MOCK_CONST_METHOD0(getDetector,
      Geometry::IDetector_const_sptr());
  MOCK_CONST_METHOD0(getInstrument,
      Geometry::Instrument_const_sptr());
  MOCK_CONST_METHOD0(getRunNumber,
      int());
  MOCK_METHOD1(setRunNumber,
      void(int m_RunNumber));
  MOCK_CONST_METHOD0(getMonitorCount,
      double());
  MOCK_METHOD1(setMonitorCount,
      void(double m_MonitorCount));
  MOCK_CONST_METHOD0(getH,
      double());
  MOCK_CONST_METHOD0(getK,
      double());
  MOCK_CONST_METHOD0(getL,
      double());
  MOCK_CONST_METHOD0(getHKL,
      Mantid::Kernel::V3D());
  MOCK_METHOD1(setH,
      void(double m_H));
  MOCK_METHOD1(setK,
      void(double m_K));
  MOCK_METHOD1(setL,
      void(double m_L));
  MOCK_METHOD3(setHKL,
      void(double H, double K, double L));
  MOCK_METHOD1(setHKL,
      void(Mantid::Kernel::V3D HKL));
  MOCK_CONST_METHOD0(getQLabFrame,
      Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(getQSampleFrame,
      Mantid::Kernel::V3D());
  MOCK_METHOD0(findDetector,
      bool());
  MOCK_METHOD2(setQSampleFrame,
      void(Mantid::Kernel::V3D QSampleFrame, double detectorDistance));
  MOCK_METHOD2(setQLabFrame,
      void(Mantid::Kernel::V3D QLabFrame, double detectorDistance));
  MOCK_METHOD1(setWavelength,
      void(double wavelength));
  MOCK_CONST_METHOD0(getWavelength,
      double());
  MOCK_CONST_METHOD0(getScattering,
      double());
  MOCK_CONST_METHOD0(getDSpacing,
      double());
  MOCK_CONST_METHOD0(getTOF,
      double());
  MOCK_CONST_METHOD0(getInitialEnergy,
      double());
  MOCK_CONST_METHOD0(getFinalEnergy,
      double());
  MOCK_METHOD1(setInitialEnergy,
      void(double m_InitialEnergy));
  MOCK_METHOD1(setFinalEnergy,
      void(double m_FinalEnergy));
  MOCK_CONST_METHOD0(getIntensity,
      double());
  MOCK_CONST_METHOD0(getSigmaIntensity,
      double());
  MOCK_METHOD1(setIntensity,
      void(double m_Intensity));
  MOCK_METHOD1(setSigmaIntensity,
      void(double m_SigmaIntensity));
  MOCK_CONST_METHOD0(getBinCount,
      double());
  MOCK_METHOD1(setBinCount,
      void(double m_BinCount));
  MOCK_CONST_METHOD0(getGoniometerMatrix,
      Mantid::Kernel::Matrix<double>());
  MOCK_METHOD1(setGoniometerMatrix,
      void(Mantid::Kernel::Matrix<double> m_GoniometerMatrix));
  MOCK_CONST_METHOD0(getBankName,
      std::string());
  MOCK_CONST_METHOD0(getRow,
      int());
  MOCK_CONST_METHOD0(getCol,
      int());
  MOCK_CONST_METHOD0(getDetPos,
      Mantid::Kernel::V3D());
  MOCK_CONST_METHOD0(getL1,
      double());
  MOCK_CONST_METHOD0(getL2,
      double());
};

public:

  void test_construction_selects_lab_view_required()
  {
    MockFirstExperimentInfoQuery mockQuery;
    EXPECT_CALL(mockQuery, hasOrientedLattice()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(mockQuery, hasRotatedGoniometer()).Times(1).WillOnce(Return(false));

    MockPeakOverlayFactory factory(mockQuery);
    TS_ASSERT_EQUALS( LabView ,factory.getPeakDimensionality());
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockQuery));
  }

  void test_construction_selects_sample_view_required()
  {
    MockFirstExperimentInfoQuery mockQuery;
    EXPECT_CALL(mockQuery, hasOrientedLattice()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(mockQuery, hasRotatedGoniometer()).Times(1).WillOnce(Return(true));

    MockPeakOverlayFactory factory(mockQuery);
    TS_ASSERT_EQUALS( SampleView ,factory.getPeakDimensionality());
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockQuery));
  }

  void test_construction_selects_HKL_view_required()
  {
    MockFirstExperimentInfoQuery mockQuery;
    EXPECT_CALL(mockQuery, hasOrientedLattice()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(mockQuery, hasRotatedGoniometer()).Times(1).WillOnce(Return(false));

    MockPeakOverlayFactory factory(mockQuery);
    TS_ASSERT_EQUALS( HKLView ,factory.getPeakDimensionality());
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockQuery));
  }

  void test_construction_selects_HKL_view_over_sample_view_when_both_available()
  {
    MockFirstExperimentInfoQuery mockQuery;
    EXPECT_CALL(mockQuery, hasOrientedLattice()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(mockQuery, hasRotatedGoniometer()).Times(1).WillOnce(Return(true));

    MockPeakOverlayFactory factory(mockQuery);
    TS_ASSERT_EQUALS( HKLView ,factory.getPeakDimensionality());
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockQuery));
  }

  void do_test_fixed_radius_calculated_from_peak_intensity(const double& peakIntensity)
  {
    NiceMock<MockFirstExperimentInfoQuery> mockQuery;
    MockPeakOverlayFactory factory(mockQuery);

    NiceMock<MockIPeak> mockPeak;
    const bool hasVariableRadius = (peakIntensity != 0); // Should replicate internal logic.

    EXPECT_CALL(mockPeak, getIntensity()).Times(1).WillOnce(Return(peakIntensity)); 
    EXPECT_CALL(mockPeak, getQLabFrame()).Times(1).WillOnce(Return(Mantid::Kernel::V3D(0,0,0)));

    MockPeakOverlayFactory mockFactory(mockQuery);

    MockPeakOverlayView* pMockView = new MockPeakOverlayView;
    PeakOverlayView_sptr mockView(pMockView);
    EXPECT_CALL(mockFactory, createViewAtPoint(_,_,hasVariableRadius)).Times(1).WillOnce(Return(mockView));

    mockFactory.createView(mockPeak);
 
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockPeak));
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockFactory));
  }

  void test_createview_specifies_fixed_radius_when_intensity_zero()
  {
    do_test_fixed_radius_calculated_from_peak_intensity(0);
  }

  void test_createview_specifies_non_fixed_radius_when_intensity_great_than_zero()
  {
    do_test_fixed_radius_calculated_from_peak_intensity(0.001);
  }

  void test_createview_specifies_non_fixed_radius_when_intensity_less_than_zero()
  {
    do_test_fixed_radius_calculated_from_peak_intensity(-0.001);
  }

  void test_createview_gets_peak_coordinates_as_lab_view_according_to_query_results()
  {
    NiceMock<MockFirstExperimentInfoQuery> mockQuery;
    EXPECT_CALL(mockQuery, hasOrientedLattice()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(mockQuery, hasRotatedGoniometer()).Times(1).WillOnce(Return(false));
    
    NiceMock<MockIPeak> mockPeak;
    EXPECT_CALL(mockPeak, getQLabFrame()).Times(1).WillOnce(Return(Mantid::Kernel::V3D(0,0,0)));

    MockPeakOverlayFactory mockFactory(mockQuery);

    MockPeakOverlayView* pMockView = new MockPeakOverlayView;
    PeakOverlayView_sptr mockView(pMockView);
    EXPECT_CALL(mockFactory, createViewAtPoint(_,_,_)).Times(1).WillOnce(Return(mockView));

    mockFactory.createView(mockPeak);
 
    TS_ASSERT_EQUALS( LabView ,mockFactory.getPeakDimensionality());
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockQuery));
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockPeak));
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockFactory));
  }

  void test_createview_gets_peak_coordinates_as_sample_view_according_to_query_results()
  {
    NiceMock<MockFirstExperimentInfoQuery> mockQuery;
    EXPECT_CALL(mockQuery, hasOrientedLattice()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(mockQuery, hasRotatedGoniometer()).Times(1).WillOnce(Return(true));
    
    NiceMock<MockIPeak> mockPeak;
    EXPECT_CALL(mockPeak, getQSampleFrame()).Times(1).WillOnce(Return(Mantid::Kernel::V3D(0,0,0)));

    MockPeakOverlayFactory mockFactory(mockQuery);
    
    MockPeakOverlayView* pMockView = new MockPeakOverlayView;
    PeakOverlayView_sptr mockView(pMockView);
    EXPECT_CALL(mockFactory, createViewAtPoint(_,_,_)).Times(1).WillOnce(Return(mockView));

    mockFactory.createView(mockPeak);
 
    TS_ASSERT_EQUALS( SampleView ,mockFactory.getPeakDimensionality());
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockQuery));
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockPeak));
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockFactory));
  }

  void test_createview_gets_peak_coordinates_as_HKL_view_according_to_query_results()
  {
    NiceMock<MockFirstExperimentInfoQuery> mockQuery;
    EXPECT_CALL(mockQuery, hasOrientedLattice()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(mockQuery, hasRotatedGoniometer()).Times(1).WillOnce(Return(false));
    
    NiceMock<MockIPeak> mockPeak;
    EXPECT_CALL(mockPeak, getHKL()).Times(1).WillOnce(Return(Mantid::Kernel::V3D(0,0,0)));

    MockPeakOverlayFactory mockFactory(mockQuery);

    MockPeakOverlayView* pMockView = new MockPeakOverlayView;
    PeakOverlayView_sptr mockView(pMockView);
    EXPECT_CALL(mockFactory, createViewAtPoint(_,_,_)).Times(1).WillOnce(Return(mockView));

    mockFactory.createView(mockPeak);
 
    TS_ASSERT_EQUALS( HKLView ,mockFactory.getPeakDimensionality());
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockQuery));
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockPeak));
    TS_ASSERT( Mock::VerifyAndClearExpectations(&mockFactory));
  }



};


#endif /*SLICE_VIEWER_PEAKOVERLAYFACTORY_TEST_H_*/
