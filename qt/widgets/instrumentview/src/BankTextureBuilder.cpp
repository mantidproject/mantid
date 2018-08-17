#include "MantidQtWidgets/InstrumentView/BankTextureBuilder.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/InstrumentView/BankRenderingHelpers.h"

using Mantid::Beamline::ComponentType;

namespace MantidQt {
namespace MantidWidgets {

namespace detail {

BankTextureBuilder::BankTextureBuilder(const Mantid::Geometry::ComponentInfo &compInfo,
                           size_t index)
    : m_compInfo(compInfo), m_bankIndex(index),
      m_bankType(compInfo.componentType(index)) {

  switch (m_bankType) {
  case ComponentType::Grid:
    m_colorTextureIDs.resize(6);
    m_pickTextureIDs.resize(6);
    m_detColorTextures.resize(6);
    m_detPickTextures.resize(6);
    break;
  case ComponentType::Rectangular:
  case ComponentType::OutlineComposite:
    m_colorTextureIDs.resize(1);
    m_pickTextureIDs.resize(1);
    m_detColorTextures.resize(1);
    m_detPickTextures.resize(1);
    break;
  }
}

/** Generate and store color textures for bank. This method results in opengl
calls to
* register texture Ids and build textures in memory.
@param colors the color mapping of the detectors.
@param isUsingLayer if layering for grid detectors is enabled. Ignored for all
other types.
@param layer grid layer which should be rendered. Ignored for all other types.
*/
void BankTextureBuilder::buildColorTextures(const std::vector<GLColor> &colors,
                                      bool isUsingLayer, size_t layer) {
  buildOpenGLTextures(false, colors, isUsingLayer, layer);
}

/** Generate and store pick textures for bank. This method results in opengl
calls to
* register texture Ids and build textures in memory.
@param colors the color mapping of the detectors.
@param isUsingLayer if layering for grid detectors is enabled. Ignored for all
other types.
@param layer grid layer which should be rendered. Ignored for all other types.
*/
void BankTextureBuilder::buildPickTextures(const std::vector<GLColor> &colors,
                                     bool isUsingLayer, size_t layer) {
  buildOpenGLTextures(true, colors, isUsingLayer, layer);
}

/** Generate and store color or picking textures for bank. This method results
in opengl calls to
* register texture Ids and build textures in memory.
@param colors the color mapping of the detectors.
@param picking flag which determines whether pick colors are rendered.
@param isUsingLayer if layering for grid detectors is enabled. Ignored for all
other types.
@param layer grid layer which should be rendered. Ignored for all other types.
*/
void BankTextureBuilder::buildOpenGLTextures(bool picking,
                                       const std::vector<GLColor> &colors,
                                       bool isUsingLayer, size_t layer) {
  switch (m_bankType) {
  case ComponentType::OutlineComposite:
    buildTubeBankTextures(colors, picking);
    break;
  case ComponentType::Grid:
    buildGridBankTextures(colors, picking, isUsingLayer, layer);
    break;
  case ComponentType::Rectangular:
    buildRectangularBankTextures(colors, picking);
    break;
  }
}

/** Uploads openGL textures for rendering. This method results in openGL calls.
@param picking flag which determines whether pick colors are rendered.
*/
void BankTextureBuilder::uploadTextures(bool picking) {
  switch (m_bankType) {
  case ComponentType::OutlineComposite:
    uploadTubeBankTextures(picking);
    break;
  case ComponentType::Grid:
    uploadGridBankTextures(picking);
    break;
  case ComponentType::Rectangular:
    uploadRectangularBankTextures(picking);
    break;
  }
}

size_t BankTextureBuilder::numTextures() const { return m_detColorTextures.size(); }

void BankTextureBuilder::buildTubeBankTextures(const std::vector<GLColor> &colors,
                                         bool picking) {
  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];
  const auto &children = m_compInfo.children(m_bankIndex);
  texture.resize(children.size() * 3);

  for (size_t i = 0; i < children.size(); ++i) {
    auto col = colors[children[i]];
    auto pos = i * 3;
    texture[pos] = static_cast<unsigned char>(col.red());
    texture[pos + 1] = static_cast<unsigned char>(col.green());
    texture[pos + 2] = static_cast<unsigned char>(col.blue());
  }
}

void BankTextureBuilder::buildGridBankTextures(const std::vector<GLColor> &colors,
                                         bool picking, bool isUsingLayer,
                                         size_t layer) {
  m_isUsingLayers = isUsingLayer;
  m_layer = layer;
}

void BankTextureBuilder::buildRectangularBankTextures(
    const std::vector<GLColor> &colors, bool picking) {
  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];
  auto bank = m_compInfo.quadrilateralComponent(m_bankIndex);
  // Round size up to nearest power of 2
  auto res = BankRenderingHelpers::getCorrectedTextureSize(bank.nX, bank.nY);
  auto texSizeX = res.first;
  auto texSizeY = res.second;

  // Texture width is 3 times wider due to RGB values
  texture.resize(texSizeX * texSizeY * 3, 0); // fill with black

  const auto &children = m_compInfo.children(m_bankIndex);
  auto colWidth = children.size() * 3;
  for (size_t x = 0; x < colWidth; x += 3) {
    const auto &dets = m_compInfo.detectorsInSubtree(children[x / 3]);
    for (size_t y = 0; y < dets.size(); ++y) {
      auto det = dets[y];
      auto ti = (y * texSizeX * 3) + x;
      texture[ti] = static_cast<char>(colors[det].red());
      texture[ti + 1] = static_cast<char>(colors[det].green());
      texture[ti + 2] = static_cast<char>(colors[det].blue());
    }
  }
}

void BankTextureBuilder::uploadTubeBankTextures(bool picking) {
  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];
  auto &textureID = picking ? m_pickTextureIDs[0] : m_colorTextureIDs[0];

  if (textureID > 0)
    glDeleteTextures(1, &textureID);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  glTexImage2D(GL_TEXTURE_2D, 0, 3, 1, static_cast<GLsizei>(texture.size() / 3),
               0, GL_RGB, GL_UNSIGNED_BYTE, texture.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void BankTextureBuilder::uploadGridBankTextures(bool picking) {}

void BankTextureBuilder::uploadRectangularBankTextures(bool picking) {
  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];

  auto bank = m_compInfo.quadrilateralComponent(m_bankIndex);
  auto res = BankRenderingHelpers::getCorrectedTextureSize(bank.nX, bank.nY);
  auto xsize = res.first;
  auto ysize = res.second;

  auto &textureID = picking ? m_pickTextureIDs[0] : m_colorTextureIDs[0];
  if (textureID != 0)
    glDeleteTextures(1, &textureID);

  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
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

void BankTextureBuilder::bindTextures(bool picking) {
  glEnable(GL_TEXTURE_2D);
  auto &textureIDs = picking ? m_pickTextureIDs : m_colorTextureIDs;

  for (auto &textureID : textureIDs)
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void BankTextureBuilder::unbindTextures() {
  for (size_t i = 0; i < m_pickTextureIDs.size(); ++i)
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace detail
} // namespace MantidWidgets
} // namespace MantidQt