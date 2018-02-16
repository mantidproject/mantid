#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Rendering/Renderer.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Quat.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"

using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {

namespace {
size_t decodePickColorRGB(unsigned char r, unsigned char g, unsigned char b) {
  unsigned int index = r;
  index *= 256;
  index += g;
  index *= 256;
  index += b - 1;
  return index;
}
} // namespace

InstrumentRenderer::InstrumentRenderer(const InstrumentActor &actor)
    : m_actor(actor) {

  m_displayListId[0] = 0;
  m_displayListId[1] = 0;
  m_useDisplayList[0] = false;
  m_useDisplayList[1] = false;

  const auto &componentInfo = actor.componentInfo();
  size_t numTextures = 0;
  for (size_t i = componentInfo.root(); !componentInfo.isDetector(i); --i) {
    if (componentInfo.componentType(i) ==
        Mantid::Beamline::ComponentType::Rectangular) {
      m_textureIndices.push_back(i);
      m_reverseTextureIndexMap[i] = numTextures;
      numTextures++;
    }
  }
  m_textureIDs.resize(numTextures, 0);
  colorTextures.resize(numTextures);
  pickTextures.resize(numTextures);
}

InstrumentRenderer::~InstrumentRenderer() {
  for (size_t i = 0; i < 2; ++i) {
    if (m_displayListId[i] != 0) {
      glDeleteLists(m_displayListId[i], 1);
    }
  }
}

void InstrumentRenderer::renderInstrument(const std::vector<bool> &visibleComps,
                                          bool showGuides, bool picking) {
  if (std::none_of(visibleComps.cbegin(), visibleComps.cend(),
                   [](bool visible) { return visible; }))
    return;

  OpenGLError::check("InstrumentActor::draw()");
  size_t i = picking ? 1 : 0;
  if (m_useDisplayList[i]) {
    glCallList(m_displayListId[i]);
  } else {
    m_displayListId[i] = glGenLists(1);
    m_useDisplayList[i] = true;
    glNewList(m_displayListId[i],
              GL_COMPILE); // Construct display list for object representation
    draw(visibleComps, showGuides, picking);
    glEndList();
    if (glGetError() == GL_OUT_OF_MEMORY) // Throw an exception
      throw Mantid::Kernel::Exception::OpenGLError(
          "OpenGL: Out of video memory");
    glCallList(m_displayListId[i]);
  }
  OpenGLError::check("InstrumentActor::draw()");
}

namespace {
void updateVisited(std::vector<bool> &visited,
                   const std::vector<size_t> &components) {
  for (auto component : components)
    visited[component] = true;
}
} // namespace

void InstrumentRenderer::draw(const std::vector<bool> &visibleComps,
                              bool showGuides, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  const auto &detInfo = m_actor.detectorInfo();
  std::vector<bool> visited(compInfo.size(), false);

  for (size_t i = compInfo.root(); i != std::numeric_limits<size_t>::max();
       --i) {
    if (compInfo.componentType(i) ==
        Mantid::Beamline::ComponentType::Rectangular) {
      updateVisited(visited, compInfo.componentsInSubtree(i));
      if (visibleComps[i])
        drawRectangularBank(i, picking);
      continue;
    }

    if (compInfo.componentType(i) ==
        Mantid::Beamline::ComponentType::OutlineComposite) {
      updateVisited(visited, compInfo.componentsInSubtree(i));
      continue;
    }

    if (!compInfo.isDetector(i) && !showGuides) {
      visited[i] = true;
      continue;
    }

    if (compInfo.hasValidShape(i) && visibleComps[i] && !visited[i]) {
      visited[i] = true;
      drawSingleDetector(i, picking);
    }
  }
}

void InstrumentRenderer::drawRectangularBank(size_t bankIndex, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  glPushMatrix();

  auto bank = compInfo.quadrilateralComponent(bankIndex);
  auto pos = compInfo.position(bank.bottomLeft);
  auto rot = compInfo.rotation(bank.bottomLeft);
  auto scale = compInfo.scaleFactor(bankIndex);
  glTranslated(pos.X(), pos.Y(), pos.Z());
  glScaled(scale[0], scale[1], scale[2]);
  double deg, ax0, ax1, ax2;
  rot.getAngleAxis(deg, ax0, ax1, ax2);
  glRotated(deg, ax0, ax1, ax2);

  auto ti = m_reverseTextureIndexMap[bankIndex];
  glBindTexture(GL_TEXTURE_2D, m_textureIDs[ti]);
  uploadRectangularTexture(picking ? pickTextures[ti] : colorTextures[ti],
                           bankIndex);

  Mantid::Geometry::detail::Renderer::renderRectangularBank(compInfo,
                                                            bankIndex);
  glBindTexture(GL_TEXTURE_2D, 0);
  glPopMatrix();
}

void InstrumentRenderer::drawSingleDetector(size_t detIndex, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  if (picking)
    m_pickColors[detIndex].paint();
  else
    m_colors[detIndex].paint();
  glPushMatrix();
  // Translate
  auto pos = compInfo.position(detIndex);
  if (!pos.nullVector())
    glTranslated(pos[0], pos[1], pos[2]);

  // Rotate
  auto rot = compInfo.rotation(detIndex);
  if (!rot.isNull()) {
    double deg, ax0, ax1, ax2;
    rot.getAngleAxis(deg, ax0, ax1, ax2);
    glRotated(deg, ax0, ax1, ax2);
  }

  // Scale
  auto scale = compInfo.scaleFactor(detIndex);
  if (scale != Mantid::Kernel::V3D(1, 1, 1))
    glScaled(scale[0], scale[1], scale[2]);

  compInfo.shape(detIndex).draw();
  glPopMatrix();
}

void InstrumentRenderer::reset() {
  resetColors();
  resetPickColors();

  if (m_textureIDs.size() > 0) {
    for (size_t i = 0; i < m_textureIDs.size(); i++) {
      generateRectangularTexture(colorTextures[i], m_colors,
                                 m_textureIndices[i]);
      generateRectangularTexture(pickTextures[i], m_pickColors,
                                 m_textureIndices[i]);
    }
  }

  /// Invalidate the OpenGL display lists to force full re-drawing of the
  /// instrument and creation of new lists.
  for (size_t i = 0; i < 2; ++i) {
    if (m_displayListId[i] != 0) {
      glDeleteLists(m_displayListId[i], 1);
      m_displayListId[i] = 0;
      m_useDisplayList[i] = false;
    }
  }
}

void InstrumentRenderer::resetColors() {
  QwtDoubleInterval qwtInterval(m_actor.minValue(), m_actor.maxValue());
  auto sharedWorkspace = m_actor.getWorkspace();
  const auto &compInfo = m_actor.componentInfo();
  const auto &detInfo = m_actor.detectorInfo();
  auto color = m_colorMap.rgb(qwtInterval, 0);
  m_colors.assign(compInfo.size(),
                  GLColor(qRed(color), qGreen(color), qBlue(color), 1));
  auto invalidColor = GLColor(80, 80, 80, 1);
  auto maskedColor = GLColor(100, 100, 100, 1);

  Mantid::API::IMaskWorkspace_sptr mask = m_actor.getMaskWorkspaceIfExists();
  const auto &detectorIDs = detInfo.detectorIDs();
  for (size_t det = 0; det < detInfo.size(); ++det) {
    auto masked = false;

    if (mask)
      masked = mask->isMasked(detectorIDs[det]);
    if (detInfo.isMasked(det) || masked)
      m_colors[det] = maskedColor;
    else {
      auto integratedValue = m_actor.getIntegratedCounts(det);
      if (integratedValue > -1) {
        auto color = m_colorMap.rgb(qwtInterval, integratedValue);
        m_colors[det] = GLColor(
            qRed(color), qGreen(color), qBlue(color),
            static_cast<int>(255 * (integratedValue / m_actor.maxValue())));
      } else
        m_colors[det] = invalidColor;
    }
  }

  for (const auto comp : m_actor.components())
    m_colors[comp] = maskedColor;
}

void InstrumentRenderer::resetPickColors() {
  const auto &compInfo = m_actor.componentInfo();
  m_pickColors.resize(compInfo.size());

  for (size_t i = 0; i < compInfo.size(); ++i) {
    m_pickColors[i] = makePickColor(i);
  }
}

void InstrumentRenderer::changeScaleType(int type) {
  m_colorMap.changeScaleType(static_cast<GraphOptions::ScaleType>(type));
}

void InstrumentRenderer::changeNthPower(double nth_power) {
  m_colorMap.setNthPower(nth_power);
}

GLColor InstrumentRenderer::getColor(size_t index) const {
  if (index <= m_colors.size() - 1)
    return m_colors.at(index);

  return m_colors.front();
}

GLColor InstrumentRenderer::makePickColor(size_t pickID) {
  pickID += 1;
  unsigned char r, g, b;
  r = static_cast<unsigned char>(pickID / 65536);
  g = static_cast<unsigned char>((pickID % 65536) / 256);
  b = static_cast<unsigned char>((pickID % 65536) % 256);
  return GLColor(r, g, b);
}

size_t InstrumentRenderer::decodePickColor(const QRgb &c) {
  return decodePickColorRGB(static_cast<unsigned char>(qRed(c)),
                            static_cast<unsigned char>(qGreen(c)),
                            static_cast<unsigned char>(qBlue(c)));
}

void InstrumentRenderer::loadColorMap(const QString &fname) {
  m_colorMap.loadMap(fname);
}

void InstrumentRenderer::generateRectangularTexture(
    std::vector<char> &texture, const std::vector<GLColor> &colors,
    size_t bankIndex) {
  const auto &compInfo = m_actor.componentInfo();
  auto bank = compInfo.quadrilateralComponent(bankIndex);
  auto xstep = (compInfo.position(bank.bottomRight).X() -
                compInfo.position(bank.bottomLeft).X()) /
               bank.nX;
  auto ystep = (compInfo.position(bank.topRight).Y() -
                compInfo.position(bank.bottomLeft).Y()) /
               bank.nY;

  auto res = Mantid::Geometry::detail::Renderer::getCorrectedTextureSize(
      bank.nX, bank.nY);
  auto texSizeX = res.first;
  auto texSizeY = res.second;

  texture.resize(texSizeX * texSizeY * 3, 0); // fill with black
  auto basePos = compInfo.position(bank.bottomLeft);
  auto dets = compInfo.detectorsInSubtree(bankIndex);

  for (auto det : dets) {
    if (!compInfo.isDetector(det))
      continue;

    auto x = static_cast<size_t>(abs(compInfo.position(det).X() - basePos.X()) /
                                 xstep);
    auto y = static_cast<size_t>(abs(compInfo.position(det).Y() - basePos.Y()) /
                                 ystep);
    x *= 3;
    auto ti = (y * texSizeX * 3) + x;
    texture[ti] = static_cast<char>(colors[det].red());
    texture[ti + 1] = static_cast<char>(colors[det].green());
    texture[ti + 2] = static_cast<char>(colors[det].blue());
  }
}

void InstrumentRenderer::uploadRectangularTexture(
    const std::vector<char> &texture, size_t textureIndex) {
  auto bank = m_actor.componentInfo().quadrilateralComponent(textureIndex);
  auto res = Mantid::Geometry::detail::Renderer::getCorrectedTextureSize(
      bank.nX, bank.nY);
  auto xsize = res.first;
  auto ysize = res.second;

  auto ti = m_reverseTextureIndexMap[textureIndex];
  if (m_textureIDs[ti] != 0)
    glDeleteTextures(1, &m_textureIDs[ti]);

  glGenTextures(1, &m_textureIDs[ti]);
  glBindTexture(GL_TEXTURE_2D, m_textureIDs[ti]);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // Allow lighting effects
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<GLsizei>(xsize),
               static_cast<GLsizei>(ysize), 0, GL_RGB, GL_UNSIGNED_BYTE,
               texture.data());
}

} // namespace MantidWidgets
} // namespace MantidQt