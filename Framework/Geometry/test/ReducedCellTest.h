// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Matrix.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/ReducedCell.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::Matrix;

class ReducedCellTest : public CxxTest::TestSuite {
public:
  void test_WeightedDistance() {
    double quartz_err[45] = {0.00000, 3.58705, 1.55740, 1.55740, 1.55740, 0.70989, 0.50000, 1.19527, 1.31295,
                             3.58705, 3.58705, 1.55740, 0.00000, 0.00000, 0.00000, 1.55740, 1.31295, 1.31295,
                             3.58705, 3.58705, 1.55740, 1.55740, 1.55740, 1.55740, 0.70989, 1.55740, 3.58705,
                             3.58705, 3.58705, 3.58705, 3.58705, 3.58705, 1.55740, 1.55740, 0.00000, 1.55740,
                             1.55740, 1.55740, 0.00000, 0.00000, 1.55740, 1.55740, 1.55740, 0.78133, 0.00000};

    double oxalic_err[45] = {0.00000,  8.46773,  5.94102,  5.94102, 5.94102, 5.80000, 5.80000, 5.80000, 5.80000,
                             2.66773,  2.50000,  2.50000,  3.48243, 2.50000, 2.50000, 2.50000, 5.11692, 3.66129,
                             10.96773, 10.96773, 10.47244, 8.30000, 8.30000, 8.30000, 8.30000, 8.30000, 3.48308,
                             3.48308,  2.66773,  3.48308,  1.19537, 2.50865, 1.19537, 0.00000, 1.19537, 1.19537,
                             0.13023,  0.13023,  2.26465,  2.26465, 1.19537, 0.51072, 0.51072, 0.51072, 0.00000};

    double silicon_err[45] = {0.00000, 0.00000, 0.00000, 1.59472, 2.81840, 2.43702, 2.81840, 2.81840, 2.81840,
                              0.00000, 0.00000, 1.59472, 2.81840, 2.81840, 2.81840, 2.81840, 2.81840, 2.81840,
                              0.86527, 0.00000, 0.00000, 1.59472, 2.81840, 2.81840, 2.43702, 2.81840, 0.86527,
                              0.00000, 0.86527, 0.86527, 0.86527, 0.00000, 1.59472, 2.81840, 2.81840, 2.81840,
                              2.81840, 2.81840, 2.81840, 2.81840, 2.81840, 2.81840, 2.81840, 2.81840, 2.81840};
    double distance;
    ReducedCell form_0(0, 4.9, 4.9, 5.4, 90, 90, 120);

    for (size_t i = 1; i <= 44; i++) {
      ReducedCell form_i(i, 4.9, 4.9, 5.4, 90, 90, 120);
      distance = form_0.WeightedDistance(form_i);
      TS_ASSERT_DELTA(distance, quartz_err[i], 1e-5);
    }

    form_0 = ReducedCell(0, 6.1, 3.6, 11.9, 90, 103.3, 90);
    for (size_t i = 1; i <= 44; i++) {
      ReducedCell form_i(i, 6.1, 3.6, 11.9, 90, 103.3, 90);
      distance = form_0.WeightedDistance(form_i);
      TS_ASSERT_DELTA(distance, oxalic_err[i], 1e-5);
    }

    form_0 = ReducedCell(0, 3.85, 3.85, 3.85, 60, 60, 60);
    for (size_t i = 1; i <= 44; i++) {
      ReducedCell form_i(i, 3.85, 3.85, 3.85, 60, 60, 60);
      distance = form_0.WeightedDistance(form_i);
      TS_ASSERT_DELTA(distance, silicon_err[i], 1e-5);
    }
  }
};
