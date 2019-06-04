// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MAKEUNIQUETEST_H_
#define MANTID_KERNEL_MAKEUNIQUETEST_H_



#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;

struct Vec3 {
  int x, y, z;
  Vec3() : x(0), y(0), z(0) {}
  Vec3(int x, int y, int z) : x(x), y(y), z(z) {}
};

class MakeUniqueTest : public CxxTest::TestSuite {
public:
  void test_default_constructor() {
    // Use the default constructor.
    std::unique_ptr<Vec3> v1 = std::make_unique<Vec3>();
    TS_ASSERT_EQUALS(v1->x, 0);
    TS_ASSERT_EQUALS(v1->y, 0);
    TS_ASSERT_EQUALS(v1->z, 0);
  }

  void test_another_constructor() {
    // Use the constructor that matches these arguments
    std::unique_ptr<Vec3> v2 = std::make_unique<Vec3>(0, 1, 2);
    TS_ASSERT_EQUALS(v2->x, 0);
    TS_ASSERT_EQUALS(v2->y, 1);
    TS_ASSERT_EQUALS(v2->z, 2);
  }

  void test_array_five_elements() {
    // Create a unique_ptr to an array of 5 elements
    std::unique_ptr<Vec3[]> v3 = std::make_unique<Vec3[]>(5);
    for (int i = 0; i < 5; i++) {
      TS_ASSERT_EQUALS(v3[i].x, 0);
      TS_ASSERT_EQUALS(v3[i].y, 0);
      TS_ASSERT_EQUALS(v3[i].z, 0);
    }
  }
};

#endif /* MANTID_KERNEL_MAKEUNIQUETEST_H_ */
