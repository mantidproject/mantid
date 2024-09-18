// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Quat.h"
#include "MantidQtWidgets/InstrumentView/BankRenderingHelpers.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"

using namespace MantidQt::MantidWidgets;
using Mantid::Beamline::ComponentType;

namespace MantidQt::MantidWidgets {

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
    : m_actor(actor), m_isUsingLayers(false), m_HighlightDetsWithZeroCount(false) {
  const auto &componentInfo = actor.componentInfo();

  size_t textureIndex = 0;
  if (componentInfo.size() <= 1) {
    return;
  }
  for (size_t i = componentInfo.root(); !componentInfo.isDetector(i); --i) {
    auto type = componentInfo.componentType(i);
    if (type == ComponentType::Rectangular || type == ComponentType::OutlineComposite || type == ComponentType::Grid) {
      m_textures.emplace_back(componentInfo, i);
      m_reverseTextureIndexMap[i] = textureIndex;
      textureIndex++;
    }
  }
}

void InstrumentRenderer::updateVisited(const Mantid::Geometry::ComponentInfo &compInfo, const size_t bankIndex,
                                       std::vector<bool> &visited) {
  visited[bankIndex] = true;
  const auto &children = compInfo.children(bankIndex);
  for (auto child : children) {
    const auto &subchildren = compInfo.children(child);
    if (subchildren.size() > 0)
      updateVisited(compInfo, child, visited);
    else
      visited[child] = true;
  }
}

void InstrumentRenderer::drawComponent(const size_t i, const std::vector<bool> &visibleComps, bool showGuides,
                                       bool picking, const Mantid::Geometry::ComponentInfo &compInfo,
                                       std::vector<bool> &visited) {
  auto type = compInfo.componentType(i);
  if (type == ComponentType::Infinite)
    return;

  if (type == ComponentType::Grid) {
    if (visibleComps[i]) {
      drawGridBank(i, picking);
      updateVisited(compInfo, i, visited);
    }
    return;
  }
  if (type == ComponentType::Rectangular) {
    if (visibleComps[i]) {
      drawRectangularBank(i, picking);
      updateVisited(compInfo, i, visited);
    }
    return;
  }

  if (type == ComponentType::OutlineComposite) {
    if (visibleComps[i]) {
      drawTube(i, picking);
      updateVisited(compInfo, i, visited);
    }
    return;
  }

  if (type == ComponentType::Structured) {
    if (visibleComps[i]) {
      drawStructuredBank(i, picking);
      updateVisited(compInfo, i, visited);
    }
    return;
  }

  if (!compInfo.isDetector(i) && !showGuides) {
    visited[i] = true;
    return;
  }

  if (compInfo.hasValidShape(i) && visibleComps[i] && !visited[i]) {
    visited[i] = true;
    drawSingleDetector(i, picking);
  }
}

void InstrumentRenderer::drawGridBank(size_t bankIndex, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  glPushMatrix();

  auto firstLayer = compInfo.children(bankIndex)[0];
  auto bank = compInfo.quadrilateralComponent(firstLayer);
  auto pos = compInfo.position(bank.bottomLeft);

  auto scale = compInfo.scaleFactor(bankIndex);
  glTranslated(pos.X(), pos.Y(), pos.Z());
  glScaled(scale[0], scale[1], scale[2]);

  auto rot = compInfo.rotation(bankIndex);
  if (!(rot.isNull())) {
    double deg, ax0, ax1, ax2;
    rot.getAngleAxis(deg, ax0, ax1, ax2);
    glRotated(deg, ax0, ax1, ax2);
  }

  auto ti = m_reverseTextureIndexMap[bankIndex];
  auto &tex = m_textures[ti];
  tex.bindTextures(picking);
  if (m_isUsingLayers) { // Render single layer
    tex.uploadTextures(picking);
    BankRenderingHelpers::renderGridBankLayer(compInfo, bankIndex, m_layer);
    BankRenderingHelpers::renderGridBankOutline(compInfo, bankIndex);
  } else { // Render 6 faces representing grid box
    tex.uploadTextures(picking, detail::GridTextureFace::Front);
    BankRenderingHelpers::renderGridBankFull(compInfo, bankIndex, detail::GridTextureFace::Front);
    tex.uploadTextures(picking, detail::GridTextureFace::Back);
    BankRenderingHelpers::renderGridBankFull(compInfo, bankIndex, detail::GridTextureFace::Back);
    tex.uploadTextures(picking, detail::GridTextureFace::Left);
    BankRenderingHelpers::renderGridBankFull(compInfo, bankIndex, detail::GridTextureFace::Left);
    tex.uploadTextures(picking, detail::GridTextureFace::Right);
    BankRenderingHelpers::renderGridBankFull(compInfo, bankIndex, detail::GridTextureFace::Right);
    tex.uploadTextures(picking, detail::GridTextureFace::Top);
    BankRenderingHelpers::renderGridBankFull(compInfo, bankIndex, detail::GridTextureFace::Top);
    tex.uploadTextures(picking, detail::GridTextureFace::Bottom);
    BankRenderingHelpers::renderGridBankFull(compInfo, bankIndex, detail::GridTextureFace::Bottom);
  }
  tex.unbindTextures();
  glPopMatrix();
}

void InstrumentRenderer::drawRectangularBank(size_t bankIndex, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  glPushMatrix();

  auto bank = compInfo.quadrilateralComponent(bankIndex);
  auto pos = compInfo.position(bank.bottomLeft);

  auto scale = compInfo.scaleFactor(bankIndex);
  glTranslated(pos.X(), pos.Y(), pos.Z());
  glScaled(scale[0], scale[1], scale[2]);

  auto rot = compInfo.rotation(bankIndex);
  if (!(rot.isNull())) {
    double deg, ax0, ax1, ax2;
    rot.getAngleAxis(deg, ax0, ax1, ax2);
    glRotated(deg, ax0, ax1, ax2);
  }

  auto ti = m_reverseTextureIndexMap[bankIndex];
  auto &tex = m_textures[ti];
  tex.bindTextures(picking);
  tex.uploadTextures(picking);

  BankRenderingHelpers::renderRectangularBank(compInfo, bankIndex);

  tex.unbindTextures();
  glPopMatrix();
}

void InstrumentRenderer::drawStructuredBank(size_t bankIndex, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  glPushMatrix();

  auto bank = compInfo.quadrilateralComponent(bankIndex);
  const auto &shapeInfo = compInfo.shape(bank.bottomLeft).getGeometryHandler()->shapeInfo();
  auto pos = shapeInfo.points()[0];
  pos.setZ(compInfo.position(bank.bottomLeft).Z());
  auto scale = compInfo.scaleFactor(bankIndex);
  glTranslated(pos.X(), pos.Y(), pos.Z());
  glScaled(scale[0], scale[1], scale[2]);

  BankRenderingHelpers::renderStructuredBank(compInfo, bankIndex, picking ? m_pickColors : m_colors);

  glPopMatrix();
}

void InstrumentRenderer::drawTube(size_t bankIndex, bool picking) {
  const auto &compInfo = m_actor.componentInfo();
  glPushMatrix();

  auto pos = compInfo.position(bankIndex);
  auto rot = compInfo.rotation(bankIndex);
  auto scale = compInfo.scaleFactor(bankIndex);
  glTranslated(pos.X(), pos.Y(), pos.Z());
  glScaled(scale[0], scale[1], scale[2]);
  double deg, ax0, ax1, ax2;
  rot.getAngleAxis(deg, ax0, ax1, ax2);
  glRotated(deg, ax0, ax1, ax2);

  auto ti = m_reverseTextureIndexMap[bankIndex];
  auto &tex = m_textures[ti];
  glColor3f(1.0f, 1.0f, 1.0f);
  tex.bindTextures(picking);
  tex.uploadTextures(picking);

  compInfo.shape(bankIndex).draw();

  tex.unbindTextures();
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

  for (auto &texture : m_textures) {
    texture.buildColorTextures(m_colors, m_isUsingLayers, m_layer);
    texture.buildPickTextures(m_pickColors, m_isUsingLayers, m_layer);
  }

  /// Invalidate the OpenGL display lists to force full re-drawing of the
  /// instrument and creation of new lists.
  resetDisplayLists();
}

void InstrumentRenderer::invalidateAndDeleteDisplayList(std::vector<GLuint> &displayList, bool &useList) {
  useList = false;
  for (size_t i = 0; i < m_actor.componentInfo().size(); ++i) {
    if (displayList[i] != 0) {
      glDeleteLists(displayList[i], 1);
      displayList[i] = 0;
    }
  }
}

void InstrumentRenderer::resetColors() {
  const double vmin(m_actor.minValue()), vmax(m_actor.maxValue());
  // Reset all colors to 0 and resize m_colors to the appropriate size
  const auto zero = m_colorMap.rgb(vmin, vmax, 0);
  const auto &compInfo = m_actor.componentInfo();
  m_colors.assign(compInfo.size(), GLColor(qRed(zero), qGreen(zero), qBlue(zero), 1));
  // No data/masked colors
  static const auto invalidColor = GLColor(80, 80, 80, 1);
  static const auto maskedColor = GLColor(100, 100, 100, 1);

  // Compute required colors for the detectors in a single shot to avoid
  // repeated calls to python and back in the matplotlib-based implementation
  const auto &detInfo = m_actor.detectorInfo();
  std::vector<double> counts(detInfo.size());
  for (size_t det = 0; det < detInfo.size(); ++det) {
    counts[det] = m_actor.getIntegratedCounts(det);
  }
  auto rgba = m_colorMap.rgb(vmin, vmax, counts);

  // Now apply colors taking into account detectors with bad counts and detector
  // masking
  Mantid::API::IMaskWorkspace_sptr maskWS = m_actor.getMaskWorkspaceIfExists();
  const auto &detectorIDs = detInfo.detectorIDs();
  // Defines a mask checker lambda dependent on if we have a mask workspace or
  // not. Done once outside the loop to avoid repeated if branches in the loop
  std::function<bool(size_t)> isMasked;
  if (maskWS) {
    isMasked = [&detInfo, &detectorIDs, &maskWS](size_t index) {
      return maskWS->isMasked(detectorIDs[index]) || detInfo.isMasked(index);
    };
  } else {
    isMasked = [&detInfo](size_t index) { return detInfo.isMasked(index); };
  }
  for (size_t det = 0; det < counts.size(); ++det) {
    if (!isMasked(det)) {
      const double integratedValue(counts[det]);
      if (integratedValue == InstrumentActor::INVALID_VALUE ||
          (m_HighlightDetsWithZeroCount && integratedValue == 0.)) {
        m_colors[det] = invalidColor;
      } else {
        const auto &color = rgba[det];
        m_colors[det] =
            GLColor(qRed(color), qGreen(color), qBlue(color), static_cast<int>(255 * (integratedValue / vmax)));
      }
    } else {
      m_colors[det] = maskedColor;
    }
  }
  // finish off rest of the components with the mask color
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

void InstrumentRenderer::changeScaleType(ColorMap::ScaleType type) { m_colorMap.changeScaleType(type); }

void InstrumentRenderer::changeNthPower(double nth_power) { m_colorMap.setNthPower(nth_power); }

GLColor InstrumentRenderer::getColor(size_t index) const {
  if (index <= m_colors.size() - 1)
    return m_colors.at(index);

  return m_colors.front();
}

void InstrumentRenderer::enableGridBankLayers(bool on, size_t layer) {
  m_isUsingLayers = on;
  m_layer = layer;
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
  return decodePickColorRGB(static_cast<unsigned char>(qRed(c)), static_cast<unsigned char>(qGreen(c)),
                            static_cast<unsigned char>(qBlue(c)));
}

void InstrumentRenderer::loadColorMap(const std::pair<QString, bool> &cmap) {
  m_colorMap.loadMap(cmap.first);
  m_HighlightDetsWithZeroCount = cmap.second;
}
} // namespace MantidQt::MantidWidgets
