#ifndef MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_
#define MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class MeshObjectCommonTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeshObjectCommonTest *createSuite() {
    return new MeshObjectCommonTest();
  }
  static void destroySuite(MeshObjectCommonTest *suite) { delete suite; }

  void test_v3d_to_array() {

    std::vector<V3D> input{{1, 2, 3}, {4, 5, 6}};
    auto output = MeshObjectCommon::getVertices(input);
    TS_ASSERT_EQUALS(output, (std::vector<double>{1, 2, 3, 4, 5, 6}));
  }
};

#endif /* MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_ */
