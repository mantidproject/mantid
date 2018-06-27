#include "MantidQtWidgets/InstrumentView/BankRenderingHelpers.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Quat.h"

using Mantid::Kernel::V3D;
using Mantid::Kernel::Quat;
namespace {
struct Corners {
  V3D bottomLeft;
  V3D bottomRight;
  V3D topRight;
  V3D topLeft;
};

Mantid::Kernel::Logger g_log("BankRenderingHelpers");

// Round a number up to the nearest power  of 2
size_t roundToNearestPowerOfTwo(size_t val) {
  size_t rounded = 2;
  while (val > rounded)
    rounded *= 2;
  return rounded;
}

Corners findCorners(const Mantid::Geometry::ComponentInfo &compInfo,
                    size_t bankIndex) {
  auto rotation = compInfo.rotation(bankIndex);
  auto position = compInfo.position(bankIndex);
  auto bank = compInfo.quadrilateralComponent(bankIndex);
  Corners c;
  c.bottomLeft = compInfo.position(bank.bottomLeft);
  c.bottomRight = compInfo.position(bank.bottomRight);
  c.topRight = compInfo.position(bank.topRight);
  c.topLeft = compInfo.position(bank.topLeft);
  rotation.conjugate();
  rotation.rotate(c.bottomLeft);
  rotation.rotate(c.bottomRight);
  rotation.rotate(c.topLeft);
  rotation.rotate(c.topRight);
  rotation.rotate(position);
  c.bottomLeft -= position;
  c.bottomRight -= position;
  c.topRight -= position;
  c.topLeft -= position;
  return c;
}

void addVertex(const V3D &pos) {
  glVertex3f(static_cast<GLfloat>(pos.X()), static_cast<GLfloat>(pos.Y()),
             static_cast<GLfloat>(pos.Z()));
}

void setBankNormal(const V3D &pos1, const V3D &pos2, const V3D &basePos) {
  // Set the bank normal to facilitate lighting effects
  auto vec1 = pos1 - basePos;
  auto vec2 = pos2 - basePos;
  auto normal = vec1.cross_prod(vec2);
  normal.normalize();
  glNormal3f(static_cast<GLfloat>(normal.X()), static_cast<GLfloat>(normal.Y()),
             static_cast<GLfloat>(normal.Z()));
}

void extractHexahedron(const Mantid::Geometry::IObject &shape,
                       std::vector<V3D> &hex) {
  const auto &shapeInfo = shape.getGeometryHandler()->shapeInfo();
  const auto &points = shapeInfo.points();
  hex.assign(points.begin(), points.begin() + 4);
}

void rotateHexahedron(std::vector<V3D> &hex, const Quat &rotation) {
  for (auto &pos : hex)
    rotation.rotate(pos);
}

void offsetHexahedronPosition(std::vector<V3D> &hex, const V3D &offset) {
  for (auto &pos : hex)
    pos += offset;
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {
namespace BankRenderingHelpers {

std::pair<size_t, size_t> getCorrectedTextureSize(const size_t width,
                                                  const size_t height) {
  return {roundToNearestPowerOfTwo(width), roundToNearestPowerOfTwo(height)};
}

void renderRectangularBank(const Mantid::Geometry::ComponentInfo &compInfo,
                           size_t index) {

  auto c = findCorners(compInfo, index);
  auto bank = compInfo.quadrilateralComponent(index);
  const auto &detShape = compInfo.shape(bank.bottomLeft);
  const auto &shapeInfo = detShape.getGeometryHandler()->shapeInfo();
  auto xstep = shapeInfo.points()[0].X() - shapeInfo.points()[1].X();
  auto ystep = shapeInfo.points()[1].Y() - shapeInfo.points()[2].Y();
  auto name = compInfo.name(index);
  // Because texture colours are combined with the geometry colour
  // make sure the current colour is white
  glColor3f(1.0f, 1.0f, 1.0f);

  // Nearest-neighbor scaling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glEnable(GL_TEXTURE_2D); // enable texture mapping

  size_t texx, texy;
  auto res = getCorrectedTextureSize(bank.nX, bank.nY);
  texx = res.first;
  texy = res.second;
  double tex_frac_x = static_cast<double>(bank.nX) / static_cast<double>(texx);
  double tex_frac_y = static_cast<double>(bank.nY) / static_cast<double>(texy);

  glBegin(GL_QUADS);

  auto basePos = c.bottomLeft;

  // Set the bank normal to facilitate lighting effects
  setBankNormal(c.bottomRight, c.topLeft, basePos);

  glTexCoord2f(0.0, 0.0);
  addVertex(c.bottomLeft - basePos + V3D((xstep * -0.5), (ystep * -0.5), 0.0));

  glTexCoord2f(static_cast<GLfloat>(tex_frac_x), 0.0);
  addVertex(c.bottomRight - basePos + V3D((xstep * 0.5), (ystep * -0.5), 0.0));

  glTexCoord2f(static_cast<GLfloat>(tex_frac_x),
               static_cast<GLfloat>(tex_frac_y));
  addVertex(c.topRight - basePos + V3D((xstep * 0.5), (ystep * 0.5), 0.0));

  glTexCoord2f(0.0, static_cast<GLfloat>(tex_frac_y));
  addVertex(c.topLeft - basePos + V3D((xstep * -0.5), (ystep * 0.5), 0.0));

  glEnd();

  if (glGetError() > 0)
    g_log.error() << "OpenGL error in renderRectangularBank() \n";

  glDisable(
      GL_TEXTURE_2D); // stop texture mapping - not sure if this is necessary.
}

void renderStructuredBank(const Mantid::Geometry::ComponentInfo &compInfo,
                          size_t index, const std::vector<GLColor> &color) {
  glBegin(GL_QUADS);

  const auto &columns = compInfo.children(index);
  auto colWidth = (columns.size()) * 3;
  auto baseIndex = compInfo.children(columns[0])[0];
  const auto &baseShapeInfo =
      compInfo.shape(baseIndex).getGeometryHandler()->shapeInfo();
  auto basePos = baseShapeInfo.points()[0];
  std::vector<V3D> hex(4);

  setBankNormal(baseShapeInfo.points()[1], baseShapeInfo.points()[3], basePos);

  for (size_t x = 0; x < colWidth; x += 3) {
    auto index = x / 3;
    const auto &column = compInfo.children(columns[index]);
    for (size_t y = 0; y < column.size(); ++y) {
      extractHexahedron(compInfo.shape(column[y]), hex);
      auto rot = compInfo.rotation(column[y]);
      rotateHexahedron(hex, rot);
      offsetHexahedronPosition(hex, -basePos);
      offsetHexahedronPosition(hex, compInfo.position(column[y]));

      glColor3ub((GLubyte)color[column[y]].red(),
                 (GLubyte)color[column[y]].green(),
                 (GLubyte)color[column[y]].blue());
      glVertex3f(static_cast<GLfloat>(hex[0].X()),
                 static_cast<GLfloat>(hex[0].Y()), 0);
      glVertex3f(static_cast<GLfloat>(hex[1].X()),
                 static_cast<GLfloat>(hex[1].Y()), 0);
      glVertex3f(static_cast<GLfloat>(hex[2].X()),
                 static_cast<GLfloat>(hex[2].Y()), 0);
      glVertex3f(static_cast<GLfloat>(hex[3].X()),
                 static_cast<GLfloat>(hex[3].Y()), 0);
    }
  }

  glEnd();

  if (glGetError() > 0)
    g_log.error() << "OpenGL error in renderStructuredBank() \n";
}
} // namespace BankRenderingHelpers
} // namespace MantidWidgets
} // namespace MantidQt
