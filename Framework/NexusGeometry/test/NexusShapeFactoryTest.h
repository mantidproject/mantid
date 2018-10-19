// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidKernel/V3D.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::NexusGeometry::NexusShapeFactory;
using Mantid::Kernel::V3D;

class NexusShapeFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusShapeFactoryTest *createSuite() {
    return new NexusShapeFactoryTest();
  }
  static void destroySuite(NexusShapeFactoryTest *suite) { delete suite; }

  void test_make_2D_mesh() {

    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(-1, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    std::vector<uint32_t> triangles;
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
    std::vector<uint32_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2, 1, 3, 2, 3, 0, 2});

    auto obj = createMesh(std::move(triangles), std::move(vertices));
    auto mesh = dynamic_cast<const Mantid::Geometry::MeshObject *>(obj.get());
    TS_ASSERT(mesh);                                // Check right dynamic type
    TS_ASSERT_EQUALS(mesh->numberOfTriangles(), 3); // 4 vertices (3 triangles)
  }
};

class NexusShapeFactoryTestPerformance : public CxxTest::TestSuite {
private:
  std::vector<Eigen::Vector3d> m_vertices;
  std::vector<uint32_t> m_facesIndices;
  std::vector<uint32_t> m_windingOrder;

  template <typename T>
  void appendTo(std::vector<T> &destination, unsigned int value) {
    destination.push_back(static_cast<T>(value));
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusShapeFactoryTestPerformance *createSuite() {
    return new NexusShapeFactoryTestPerformance();
  }
  static void destroySuite(NexusShapeFactoryTestPerformance *suite) {
    delete suite;
  }

  NexusShapeFactoryTestPerformance() {
    // Make inputs. Repeated squares
    for (uint32_t i = 0; i < 10000; ++i) {
      m_vertices.emplace_back(Eigen::Vector3d(0 + i, 1, 0));
      m_vertices.emplace_back(Eigen::Vector3d(0 + i, 1, 0));
      /*
       *     x           x     x
       *     |           |     |
       *     |      ->   |     |
       *     x           x     x
       */

      appendTo<uint32_t>(m_facesIndices, (i * 4));
      appendTo<uint32_t>(m_facesIndices, ((i + 1) * 4));
      if (i % 2 != 0) {
        appendTo<uint32_t>(m_windingOrder, (i * 2));
        appendTo<uint32_t>(m_windingOrder, (i * 2 + 1));
        appendTo<uint32_t>(m_windingOrder, (i * 2 + 2));
        appendTo<uint32_t>(m_windingOrder, (i * 2 + 3));
      }
    }
  }

  void test_createFromOFFMesh() {
    auto mesh = createFromOFFMesh(m_facesIndices, m_windingOrder, m_vertices);
    TS_ASSERT(mesh.get() != nullptr)
  }
};
