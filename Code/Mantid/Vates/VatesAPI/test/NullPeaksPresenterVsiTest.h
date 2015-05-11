#ifndef NULL_PEAKS_PRESENTER_VSI_TEST_H_
#define NULL_PEAKS_PRESENTER_VSI_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/NullPeaksPresenterVsi.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <stdexcept>

using namespace Mantid::VATES;

class NullPeaksPresenterVsiTest : public CxxTest::TestSuite
{
public:
  void testGettingPeaksWorkspaceThrows() {
     NullPeaksPresenterVsi presenter;
     TSM_ASSERT_THROWS("Should not implement this method", presenter.getPeaksWorkspace(), std::runtime_error);
  }

  void testGettingUsablePeaksThrows() {
    NullPeaksPresenterVsi presenter;
    TSM_ASSERT_THROWS("Should not implement this method", presenter.getViewablePeaks(), std::runtime_error);
  }

  void testGettingPeaksWorkspaceNameThrows() {
    NullPeaksPresenterVsi presenter;
    TSM_ASSERT_THROWS("Should not implement this method", presenter.getViewablePeaks(), std::runtime_error);
  }

  void testGettingPeaksInfoThrows() {
    NullPeaksPresenterVsi presenter;
    int row = 0;
    double radius;
    Mantid::Kernel::V3D position;
    Mantid::Kernel::SpecialCoordinateSystem coord = Mantid::Kernel::SpecialCoordinateSystem::None;
    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace;
    TSM_ASSERT_THROWS("Should not implement this method", presenter.getPeaksInfo(peaksWorkspace,row,position,radius, coord), std::runtime_error);
  }
};
#endif
