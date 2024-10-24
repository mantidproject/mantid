// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidDataHandling/LoadStl.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>
#include <functional>
using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class LoadStlTest : public CxxTest::TestSuite {
public:
  static LoadStlTest *createSuite() { return new LoadStlTest(); }
  static void destroySuite(LoadStlTest *suite) { delete suite; }

  void test_same_V3D_hash_equal() {
    V3D vertex1 = V3D(0, 0, 0);
    auto pair1 = std::pair<V3D, uint32_t>(vertex1, 0);
    auto pair2 = std::pair<V3D, uint32_t>(vertex1, 1);
    TS_ASSERT_EQUALS(HashV3DPair{}(pair1), HashV3DPair{}(pair2));
  }

  void test_V3D_same_val_hash_equal() {
    V3D vertex1 = V3D(0, 0, 0);
    V3D vertex2 = V3D(0, 0, 0);
    auto pair1 = std::pair<V3D, uint32_t>(vertex1, 0);
    auto pair2 = std::pair<V3D, uint32_t>(vertex2, 1);
    TS_ASSERT_EQUALS(HashV3DPair{}(pair1), HashV3DPair{}(pair2));
  }

  void test_dif_first_val() {
    V3D vertex1 = V3D(0, 0, 0);
    V3D vertex2 = V3D(1, 0, 0);
    auto pair1 = std::pair<V3D, uint32_t>(vertex1, 0);
    auto pair2 = std::pair<V3D, uint32_t>(vertex2, 1);
    TS_ASSERT_DIFFERS(HashV3DPair{}(pair1), HashV3DPair{}(pair2));
  }

  void test_dif_second_val() {
    V3D vertex1 = V3D(0, 0, 0);
    V3D vertex2 = V3D(0, 1, 0);
    auto pair1 = std::pair<V3D, uint32_t>(vertex1, 0);
    auto pair2 = std::pair<V3D, uint32_t>(vertex2, 1);
    TS_ASSERT_DIFFERS(HashV3DPair{}(pair1), HashV3DPair{}(pair2));
  }

  void test_dif_third_val() {
    V3D vertex1 = V3D(0, 0, 0);
    V3D vertex2 = V3D(0, 0, 1);
    auto pair1 = std::pair<V3D, uint32_t>(vertex1, 0);
    auto pair2 = std::pair<V3D, uint32_t>(vertex2, 1);
    TS_ASSERT_DIFFERS(HashV3DPair{}(pair1), HashV3DPair{}(pair2));
  }

  void test_order_matters() {
    V3D vertex1 = V3D(1, 0, 0);
    V3D vertex2 = V3D(0, 1, 0);
    V3D vertex3 = V3D(0, 0, 1);
    auto pair1 = std::pair<V3D, uint32_t>(vertex1, 0);
    auto pair2 = std::pair<V3D, uint32_t>(vertex2, 1);
    auto pair3 = std::pair<V3D, uint32_t>(vertex3, 2);
    TS_ASSERT_DIFFERS(HashV3DPair{}(pair1), HashV3DPair{}(pair2));
    TS_ASSERT_DIFFERS(HashV3DPair{}(pair2), HashV3DPair{}(pair3));
    TS_ASSERT_DIFFERS(HashV3DPair{}(pair1), HashV3DPair{}(pair3));
  }
};
