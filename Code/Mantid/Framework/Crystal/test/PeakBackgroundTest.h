#ifndef MANTID_CRYSTAL_PEAKBACKGROUNDTEST_H_
#define MANTID_CRYSTAL_PEAKBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include "MantidCrystal/PeakBackground.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Crystal::PeakBackground;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid;
using namespace testing;

namespace
{
  // Make a peaks workspace with a single HKL peak.
  IPeaksWorkspace_sptr make_peaks_workspace(const V3D& hklPeak)
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);

    // --- Make a fake PeaksWorkspace ---
    IPeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->setInstrument(inst);

    Peak peak(inst, 15050, 1.0);
    peak.setHKL(hklPeak[0], hklPeak[1], hklPeak[2]);
    peakWS->addPeak(peak);
    return peakWS;
  }

  // Mock Background strategy
  class MockIMDIterator : public IMDIterator {
  public:
    MOCK_CONST_METHOD0(getDataSize,
      size_t());
    MOCK_METHOD0(next,
      bool());
    MOCK_CONST_METHOD0(valid,
      bool());
    MOCK_METHOD1(jumpTo,
      void(size_t index));
    MOCK_METHOD1(next,
      bool(size_t skip));
    MOCK_CONST_METHOD0(getNormalizedSignal,
      signal_t());
    MOCK_CONST_METHOD0(getNormalizedError,
      signal_t());
    MOCK_CONST_METHOD0(getSignal,
      signal_t());
    MOCK_CONST_METHOD0(getError,
      signal_t());
    MOCK_CONST_METHOD1(getVertexesArray,
      coord_t*(size_t & numVertices));
    MOCK_CONST_METHOD3(getVertexesArray,
      coord_t*(size_t & numVertices, const size_t outDimensions, const bool * maskDim));
    MOCK_CONST_METHOD0(getCenter,
      Mantid::Kernel::VMD());
    MOCK_CONST_METHOD0(getNumEvents,
      size_t());
    MOCK_CONST_METHOD1(getInnerRunIndex,
      uint16_t(size_t index));
    MOCK_CONST_METHOD1(getInnerDetectorID,
      int32_t(size_t index));
    MOCK_CONST_METHOD2(getInnerPosition,
      coord_t(size_t index, size_t dimension));
    MOCK_CONST_METHOD1(getInnerSignal,
      signal_t(size_t index));
    MOCK_CONST_METHOD1(getInnerError,
      signal_t(size_t index));
    MOCK_CONST_METHOD0(getIsMasked,
      bool());
    MOCK_CONST_METHOD0(findNeighbourIndexes, std::vector<size_t>());
    MOCK_CONST_METHOD0(findNeighbourIndexesFaceTouching, std::vector<size_t>());
    MOCK_CONST_METHOD0(getLinearIndex, size_t());
    MOCK_CONST_METHOD1(isWithinBounds, bool(size_t));

  };


}

class PeakBackgroundTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakBackgroundTest *createSuite() { return new PeakBackgroundTest(); }
  static void destroySuite( PeakBackgroundTest *suite ) { delete suite; }

  void test_within_range()
  {
    const V3D hklPeak(1,1,1);
    IPeaksWorkspace_const_sptr peaksWS = make_peaks_workspace(hklPeak);
    const double radius = 1;
    const double threshold = 100;
    PeakBackground strategy(peaksWS, radius, threshold, NoNormalization, Mantid::Kernel::HKL);

    MockIMDIterator mockIterator;
    EXPECT_CALL(mockIterator, getNormalizedSignal()).WillOnce(Return(threshold+1)); // Returns above the threshold.
    EXPECT_CALL(mockIterator, getCenter()).WillOnce(Return(hklPeak)); // Returns center as being on the peak. Thefore within range.

    TSM_ASSERT("MD data in this peak region is not background", !strategy.isBackground(&mockIterator));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockIterator));
  }

  void test_too_far_from_peak_center()
  {
    const V3D hklPeak(1,1,1);
    IPeaksWorkspace_const_sptr peaksWS = make_peaks_workspace(hklPeak);
    const double radius = 1;
    const double threshold = 100;
    PeakBackground strategy(peaksWS, radius, threshold, NoNormalization, Mantid::Kernel::HKL);

    MockIMDIterator mockIterator;
    V3D iteratorCenter(hklPeak[0] + radius, hklPeak[1], hklPeak[2]); // Offset so to be outside of peak radius.
    EXPECT_CALL(mockIterator, getNormalizedSignal()).WillOnce(Return(threshold+1e-4)); // Returns above the threshold.
    EXPECT_CALL(mockIterator, getCenter()).WillOnce(Return(iteratorCenter)); // Return offset iterator center.

    TSM_ASSERT("Data too far from peak. Should be considered background.", strategy.isBackground(&mockIterator));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockIterator));
  }

  void test_just_close_enough_to_peak_center()
  {
    const V3D hklPeak(1,1,1);
    IPeaksWorkspace_const_sptr peaksWS = make_peaks_workspace(hklPeak);
    const double radius = 1;
    const double threshold = 100;
    PeakBackground strategy(peaksWS, radius, threshold, NoNormalization, Mantid::Kernel::HKL);

    MockIMDIterator mockIterator;
    V3D iteratorCenter(hklPeak[0] + radius - 1e-4, hklPeak[1], hklPeak[2]); // Offset so to be outside of peak radius.
    EXPECT_CALL(mockIterator, getNormalizedSignal()).WillOnce(Return(threshold+1e-4)); // Returns above the threshold.
    EXPECT_CALL(mockIterator, getCenter()).WillOnce(Return(iteratorCenter)); // Return offset iterator center.

    TSM_ASSERT("Data is within peak radius. Should NOT be considered background.", !strategy.isBackground(&mockIterator));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockIterator));
  }

  void test_below_threshold()
  {
    const V3D hklPeak(1,1,1);
    IPeaksWorkspace_const_sptr peaksWS = make_peaks_workspace(hklPeak);
    const double radius = 1;
    const double threshold = 100;
    PeakBackground strategy(peaksWS, radius, threshold, NoNormalization, Mantid::Kernel::HKL);

    MockIMDIterator mockIterator;
    EXPECT_CALL(mockIterator, getNormalizedSignal()).WillOnce(Return(threshold)); // Returns equal to the threshold. Exclusive checking.
    
    TSM_ASSERT("MD data signal is below the allowed threshold. Should be background.", strategy.isBackground(&mockIterator));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockIterator));
  }


};


#endif /* MANTID_CRYSTAL_PEAKBACKGROUNDTEST_H_ */
