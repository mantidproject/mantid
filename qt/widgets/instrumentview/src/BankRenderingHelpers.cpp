#include "MantidQtWidgets/InstrumentView/BankRenderingHelpers.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Quat.h"
#include "MantidQtWidgets/InstrumentView/BankTextureBuilder.h"

using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;
using MantidQt::MantidWidgets::detail::GridTextureFace;
namespace {

class Corners {
public:
  Corners() {}
  Corners(V3D bottomLeft, V3D bottomRight, V3D topRight, V3D topLeft)
      : m_bottomLeft(bottomLeft), m_bottomRight(bottomRight),
        m_topRight(topRight), m_topLeft(topLeft) {}

  void translate(const Mantid::Kernel::V3D &position) {
    m_bottomLeft += position;
    m_bottomRight += position;
    m_topRight += position;
    m_topLeft += position;
  }

  void rotate(const Mantid::Kernel::Quat &rotation) {
    rotation.rotate(m_bottomLeft);
    rotation.rotate(m_bottomRight);
    rotation.rotate(m_topLeft);
    rotation.rotate(m_topRight);
  }

  const V3D &bottomLeft() const { return m_bottomLeft; }
  const V3D &bottomRight() const { return m_bottomRight; }
  const V3D &topRight() const { return m_topRight; }
  const V3D &topLeft() const { return m_topLeft; }

private:
  V3D m_bottomLeft;
  V3D m_bottomRight;
  V3D m_topRight;
  V3D m_topLeft;
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
  auto bank = compInfo.quadrilateralComponent(bankIndex);
  Corners c(compInfo.position(bank.bottomLeft),
            compInfo.position(bank.bottomRight),
            compInfo.position(bank.topRight), compInfo.position(bank.topLeft));
  // Apply bank transformation
  auto rotation = compInfo.rotation(bankIndex);
  auto position = compInfo.position(bankIndex);
  rotation.conjugate();
  c.rotate(rotation);
  rotation.rotate(position);
  c.translate(-position);
  return c;
}

Corners findGridCorners(const Mantid::Geometry::ComponentInfo &compInfo,
                        size_t bankIndex, GridTextureFace gridFace) {
  // Find the six faces which make up the bank cube.
  const auto &layers = compInfo.children(bankIndex);
  size_t nZ = layers.size();

  auto frontBank = compInfo.quadrilateralComponent(layers.back());
  auto rearBank = compInfo.quadrilateralComponent(layers.front());
  Corners c;
  switch (gridFace) {
  case GridTextureFace::Front:
    // front face
    c = Corners(compInfo.position(frontBank.bottomLeft),
                compInfo.position(frontBank.bottomRight),
                compInfo.position(frontBank.topRight),
                compInfo.position(frontBank.topLeft));
    break;
  case GridTextureFace::Back:
    // rear face
    c = Corners(compInfo.position(rearBank.bottomLeft),
                compInfo.position(rearBank.bottomRight),
                compInfo.position(rearBank.topRight),
                compInfo.position(rearBank.topLeft));
    break;
  case GridTextureFace::Left:
    // left face
    c = Corners(compInfo.position(rearBank.bottomLeft),
                compInfo.position(frontBank.bottomLeft),
                compInfo.position(frontBank.topLeft),
                compInfo.position(rearBank.topLeft));
    break;
  case GridTextureFace::Right:
    // right face
    c = Corners(compInfo.position(rearBank.bottomRight),
                compInfo.position(frontBank.bottomRight),
                compInfo.position(frontBank.topRight),
                compInfo.position(rearBank.topRight));
    break;
  case GridTextureFace::Top:
    // top face
    c = Corners(compInfo.position(frontBank.topLeft),
                compInfo.position(frontBank.topRight),
                compInfo.position(rearBank.topRight),
                compInfo.position(rearBank.topLeft));
    break;
  case GridTextureFace::Bottom:
    // bottom face
    c = Corners(compInfo.position(frontBank.bottomLeft),
                compInfo.position(frontBank.bottomRight),
                compInfo.position(rearBank.bottomRight),
                compInfo.position(rearBank.bottomLeft));
    break;
  }
  // Apply bank transformation
  auto rotation = compInfo.rotation(bankIndex);
  auto position = compInfo.position(bankIndex);
  rotation.conjugate();
  rotation.rotate(position);
  c.rotate(rotation);
  c.translate(-position);

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

void render2DTexture(const Corners &corners, size_t nX, size_t nY,
                     const Mantid::Kernel::V3D &bottomLeftOffset,
                     const Mantid::Kernel::V3D &bottomRightOffset,
                     const Mantid::Kernel::V3D &topRightOffset,
                     const Mantid::Kernel::V3D &topLeftOffset,
                     const Mantid::Kernel::V3D &basePos) {
  // Because texture colours are combined with the geometry colour
  // make sure the current colour is white
  glColor3f(1.0f, 1.0f, 1.0f);

  // Nearest-neighbor scaling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glEnable(GL_TEXTURE_2D); // enable texture mapping

  size_t texx, texy;
  auto res =
      MantidQt::MantidWidgets::BankRenderingHelpers::getCorrectedTextureSize(
          nX, nY);
  texx = res.first;
  texy = res.second;
  double tex_frac_x = static_cast<double>(nX) / static_cast<double>(texx);
  double tex_frac_y = static_cast<double>(nY) / static_cast<double>(texy);

  glBegin(GL_QUADS);

  // Set the bank normal to facilitate lighting effects
  setBankNormal(corners.bottomRight(), corners.topLeft(), basePos);

  glTexCoord2f(0.0, 0.0);
  addVertex(corners.bottomLeft() - basePos + bottomLeftOffset);

  glTexCoord2f(static_cast<GLfloat>(tex_frac_x), 0.0);
  addVertex(corners.bottomRight() - basePos + bottomRightOffset);

  glTexCoord2f(static_cast<GLfloat>(tex_frac_x),
               static_cast<GLfloat>(tex_frac_y));
  addVertex(corners.topRight() - basePos + topRightOffset);

  glTexCoord2f(0.0, static_cast<GLfloat>(tex_frac_y));
  addVertex(corners.topLeft() - basePos + topLeftOffset);

  glEnd();

  if (glGetError() > 0)
    g_log.error() << "OpenGL error in rendering texture. \n";

  glDisable(
      GL_TEXTURE_2D); // stop texture mapping - not sure if this is necessary.
}

std::tuple<double, double, double> findSteps(const std::vector<V3D> &points) {
  double xstep, ystep, zstep;
  xstep = ystep = zstep = 0;

  for (int i = 0; i < points.size() - 1; i++) {
    if (abs(points[i].X() - points[i + 1].X()) > 0)
      xstep = abs(points[i].X() - points[i + 1].X());
    if (abs(points[i].Y() - points[i + 1].Y()) > 0)
      ystep = abs(points[i].Y() - points[i + 1].Y());
    if (abs(points[i].Z() - points[i + 1].Z()) > 0)
      zstep = abs(points[i].Z() - points[i + 1].Z());
  }

  return std::tuple<double, double, double>(xstep, ystep, zstep);
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {
namespace BankRenderingHelpers {

std::pair<size_t, size_t> getCorrectedTextureSize(const size_t width,
                                                  const size_t height) {
  return {roundToNearestPowerOfTwo(width), roundToNearestPowerOfTwo(height)};
}

void renderGridBankLayer(const Mantid::Geometry::ComponentInfo &compInfo,
                         size_t index, size_t layer) {
  auto layerIndex = compInfo.children(index)[layer];
  auto c = findCorners(compInfo, layerIndex);
  auto bank = compInfo.quadrilateralComponent(layerIndex);
  const auto &detShape = compInfo.shape(bank.bottomLeft);
  const auto &shapeInfo = detShape.getGeometryHandler()->shapeInfo();
  auto steps = findSteps(shapeInfo.points());
  auto xstep = std::get<0>(steps);
  auto ystep = std::get<1>(steps);

  render2DTexture(c, bank.nX, bank.nY, V3D((xstep * -0.5), (ystep * -0.5), 0.0),
                  V3D((xstep * 0.5), (ystep * -0.5), 0.0),
                  V3D((xstep * 0.5), (ystep * 0.5), 0.0),
                  V3D((xstep * -0.5), (ystep * 0.5), 0.0), c.bottomLeft());
}

void renderGridBankFull(const Mantid::Geometry::ComponentInfo &compInfo,
                        size_t index, GridTextureFace gridFace) {
  auto baseCorner = findGridCorners(compInfo, index, GridTextureFace::Front);
  auto corners = findGridCorners(compInfo, index, gridFace);
  auto layers = compInfo.children(index);
  auto firstLayerIndex = layers[0];
  auto bank = compInfo.quadrilateralComponent(firstLayerIndex);
  // find the shape of a detector in the bank.
  const auto &detShape = compInfo.shape(bank.bottomLeft);
  const auto &shapeInfo = detShape.getGeometryHandler()->shapeInfo();
  auto steps = findSteps(shapeInfo.points());
  auto xstep = std::get<0>(steps);
  auto ystep = std::get<1>(steps);
  auto zstep = std::get<2>(steps);

  // All positions referenced from bottomLeft of front face
  auto basePos = baseCorner.bottomLeft();
  auto nZ = layers.size();

  switch (gridFace) {
  case GridTextureFace::Front:
    render2DTexture(corners, bank.nX, bank.nY,
                    V3D((xstep * -0.5), (ystep * -0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * -0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * 0.5), (zstep * 0.5)),
                    V3D((xstep * -0.5), (ystep * 0.5), (zstep * 0.5)), basePos);
    break;
  case GridTextureFace::Back:
    render2DTexture(corners, bank.nX, bank.nY,
                    V3D((xstep * -0.5), (ystep * -0.5), (zstep * -0.5)),
                    V3D((xstep * 0.5), (ystep * -0.5), (zstep * -0.5)),
                    V3D((xstep * 0.5), (ystep * 0.5), (zstep * -0.5)),
                    V3D((xstep * -0.5), (ystep * 0.5), (zstep * -0.5)),
                    basePos);
    break;
  case GridTextureFace::Left:
    render2DTexture(corners, nZ, bank.nY,
                    V3D((xstep * -0.5), (ystep * -0.5), (zstep * -0.5)),
                    V3D((xstep * -0.5), (ystep * -0.5), (zstep * 0.5)),
                    V3D((xstep * -0.5), (ystep * 0.5), (zstep * 0.5)),
                    V3D((xstep * -0.5), (ystep * 0.5), (zstep * -0.5)),
                    basePos);
    break;
  case GridTextureFace::Right:
    render2DTexture(corners, nZ, bank.nY,
                    V3D((xstep * 0.5), (ystep * -0.5), (zstep * -0.5)),
                    V3D((xstep * 0.5), (ystep * -0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * 0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * 0.5), (zstep * -0.5)), basePos);
    break;
  case GridTextureFace::Top:
    render2DTexture(corners, nZ, bank.nY,
                    V3D((xstep * -0.5), (ystep * +0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * +0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * +0.5), (zstep * -0.5)),
                    V3D((xstep * -0.5), (ystep * +0.5), (zstep * -0.5)),
                    basePos);
    break;
  case GridTextureFace::Bottom:
    render2DTexture(corners, nZ, bank.nY,
                    V3D((xstep * -0.5), (ystep * -0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * -0.5), (zstep * 0.5)),
                    V3D((xstep * 0.5), (ystep * -0.5), (zstep * -0.5)),
                    V3D((xstep * -0.5), (ystep * -0.5), (zstep * -0.5)),
                    basePos);
    break;
  }
}

void renderRectangularBank(const Mantid::Geometry::ComponentInfo &compInfo,
                           size_t index) {

  auto c = findCorners(compInfo, index);
  auto bank = compInfo.quadrilateralComponent(index);
  const auto &detShape = compInfo.shape(bank.bottomLeft);
  const auto &shapeInfo = detShape.getGeometryHandler()->shapeInfo();
  auto xstep = shapeInfo.points()[0].X() - shapeInfo.points()[1].X();
  auto ystep = shapeInfo.points()[1].Y() - shapeInfo.points()[2].Y();

  render2DTexture(c, bank.nX, bank.nY, V3D((xstep * -0.5), (ystep * -0.5), 0.0),
                  V3D((xstep * 0.5), (ystep * -0.5), 0.0),
                  V3D((xstep * 0.5), (ystep * 0.5), 0.0),
                  V3D((xstep * -0.5), (ystep * 0.5), 0.0), c.bottomLeft());
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