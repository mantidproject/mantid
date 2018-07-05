//----------------
// Includes
//----------------

#include <cxxtest/TestSuite.h>
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/MeshObject.h"

using namespace Mantid::NexusGeometry::NexusShapeFactory;
using Mantid::Kernel::V3D;

class NexusShapeFactoryTest : public CxxTest::TestSuite {
public:
  void test_make_2D_mesh() {

    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(-1, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    std::vector<uint16_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2});

    auto obj = createMesh(std::move(triangles), std::move(vertices));
    auto mesh2d =
        dynamic_cast<const Mantid::Geometry::MeshObject2D *>(obj.get());
    TS_ASSERT(mesh2d); // Check right dynamic type
    TS_ASSERT_EQUALS(mesh2d->numberOfTriangles(), 1); // 3 vertices (1 triangle)
  }

  void test_make_3D_mesh() {

    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(-1, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    vertices.emplace_back(V3D(0, 1, 1));
    std::vector<uint16_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2, 1, 3, 2, 3, 0, 2});

    auto obj = createMesh(std::move(triangles), std::move(vertices));
    auto mesh = dynamic_cast<const Mantid::Geometry::MeshObject *>(obj.get());
    TS_ASSERT(mesh);                                // Check right dynamic type
    TS_ASSERT_EQUALS(mesh->numberOfTriangles(), 3); // 4 vertices (3 triangles)
  }
};
