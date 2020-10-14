// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"
#include "MantidVatesAPI/NullPeaksPresenterVsi.h"
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::VATES;

class NullPeaksPresenterVsiTest : public CxxTest::TestSuite {
public:
  void testGettingPeaksWorkspaceThrows() {
    NullPeaksPresenterVsi presenter;
    TSM_ASSERT_THROWS("Should not implement this method",
                      presenter.getPeaksWorkspace(),
                      const std::runtime_error &);
  }

  void testGettingUsablePeaksThrows() {
    NullPeaksPresenterVsi presenter;
    TSM_ASSERT_THROWS("Should not implement this method",
                      presenter.getViewablePeaks(), const std::runtime_error &);
  }

  void testGettingPeaksWorkspaceNameThrows() {
    NullPeaksPresenterVsi presenter;
    TSM_ASSERT_THROWS("Should not implement this method",
                      presenter.getViewablePeaks(), const std::runtime_error &);
  }

  void testGettingPeaksInfoThrows() {
    NullPeaksPresenterVsi presenter;
    int row = 0;
    double radius;
    Mantid::Kernel::V3D position;
    Mantid::Kernel::SpecialCoordinateSystem coord =
        Mantid::Kernel::SpecialCoordinateSystem::None;
    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace;
    TSM_ASSERT_THROWS(
        "Should not implement this method",
        presenter.getPeaksInfo(peaksWorkspace, row, position, radius, coord),
        const std::runtime_error &);
  }
};