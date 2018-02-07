#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
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

void InstrumentRenderer::draw(const std::vector<bool> &visibleComps,
                              bool showGuides, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  for (size_t i = 0; i < compInfo.size(); ++i) {
    if (compInfo.componentType(compInfo.parent(i)) ==
        Mantid::Beamline::ComponentType::Rectangular)
      continue;

    if (compInfo.componentType(compInfo.parent(i)) ==
        Mantid::Beamline::ComponentType::OutlineComposite)
      continue;

    if ((!compInfo.isDetector(i) && !showGuides))
      continue;

    if (compInfo.hasValidShape(i)) {
      if (visibleComps[i]) {
        if (picking)
          m_pickColors[i].paint();
        else
          m_colors[i].paint();
        glPushMatrix();
        // Translate
        auto pos = compInfo.position(i);
        if (!pos.nullVector())
          glTranslated(pos[0], pos[1], pos[2]);

        // Rotate
        auto rot = compInfo.rotation(i);
        if (!rot.isNull()) {
          double deg, ax0, ax1, ax2;
          rot.getAngleAxis(deg, ax0, ax1, ax2);
          glRotated(deg, ax0, ax1, ax2);
        }

        // Scale
        auto scale = compInfo.scaleFactor(i);
        if (scale != Mantid::Kernel::V3D(1, 1, 1))
          glScaled(scale[0], scale[1], scale[2]);

        compInfo.shape(i).draw();
        glPopMatrix();
      }
    }
  }
}

void InstrumentRenderer::reset() {
  resetColors();
  resetPickColors();

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

  Mantid::API::IMaskWorkspace_sptr mask = m_actor.getMaskWorkspace();
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

} // namespace MantidWidgets
} // namespace MantidQt