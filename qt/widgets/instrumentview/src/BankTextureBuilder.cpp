// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/BankTextureBuilder.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/InstrumentView/BankRenderingHelpers.h"

using Mantid::Beamline::ComponentType;

namespace { // anonymous namespace for helper funcitons
void createTexture(const Mantid::Geometry::ComponentInfo &compInfo,
                   const std::vector<size_t> &children,
                   const std::vector<MantidQt::MantidWidgets::GLColor> &colors,
                   std::vector<char> &texture, size_t texSizeX) {
  auto colWidth = children.size() * 3;
  for (size_t x = 0; x < colWidth; x += 3) {
    const auto &dets = compInfo.detectorsInSubtree(children[x / 3]);
    for (size_t y = 0; y < dets.size(); ++y) {
      auto det = dets[y];
      auto ti = (y * texSizeX * 3) + x;
      texture[ti] = static_cast<char>(colors[det].red());
      texture[ti + 1] = static_cast<char>(colors[det].green());
      texture[ti + 2] = static_cast<char>(colors[det].blue());
    }
  }
}

void addColorsToLeftAndRightTextures(
    const Mantid::Geometry::ComponentInfo &compInfo,
    const Mantid::Geometry::ComponentInfo::QuadrilateralComponent &bank,
    size_t nZ, const size_t layerIndex, const std::vector<size_t> &children,
    const std::vector<MantidQt::MantidWidgets::GLColor> &colors,
    std::vector<char> &leftText, std::vector<char> &rightText) {
  const auto &firstColumn = compInfo.children(children[0]);
  const auto &lastColumn = compInfo.children(children[bank.nX - 1]);

  for (size_t i = 0; i < bank.nY; ++i) {
    auto col = colors[firstColumn[i]];
    auto ti = (i * nZ * 3) + (layerIndex * 3); // texture index
    leftText[ti] = static_cast<char>(col.red());
    leftText[ti + 1] = static_cast<char>(col.green());
    leftText[ti + 2] = static_cast<char>(col.blue());
    col = colors[lastColumn[i]];
    rightText[ti] = static_cast<char>(col.red());
    rightText[ti + 1] = static_cast<char>(col.green());
    rightText[ti + 2] = static_cast<char>(col.blue());
  }
}

void addColorsToTopAndBottomTextures(
    const Mantid::Geometry::ComponentInfo &compInfo,
    const Mantid::Geometry::ComponentInfo::QuadrilateralComponent &bank,
    const size_t layerIndex, const std::vector<size_t> &children,
    const std::vector<MantidQt::MantidWidgets::GLColor> &colors,
    std::vector<char> &topText, std::vector<char> &bottomText) {
  for (size_t i = 0; i < bank.nX; ++i) {
    auto ti = (layerIndex * bank.nX * 3) + (i * 3);
    const auto &column = compInfo.children(children[i]);
    auto col = colors[column[0]];
    bottomText[ti] = static_cast<char>(col.red());
    bottomText[ti + 1] = static_cast<char>(col.green());
    bottomText[ti + 2] = static_cast<char>(col.blue());
    col = colors[column[bank.nY - 1]];
    topText[ti] = static_cast<char>(col.red());
    topText[ti + 1] = static_cast<char>(col.green());
    topText[ti + 2] = static_cast<char>(col.blue());
  }
}

void upload2DTexture(const std::pair<size_t, size_t> &textSizes,
                     GLuint &textureID, const std::vector<char> &texture) {
  auto w = textSizes.first;
  auto h = textSizes.second;

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

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<GLsizei>(w),
               static_cast<GLsizei>(h), 0, GL_RGB, GL_UNSIGNED_BYTE,
               texture.data());
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {

namespace detail {

BankTextureBuilder::BankTextureBuilder(
    const Mantid::Geometry::ComponentInfo &compInfo, size_t index)
    : m_compInfo(compInfo), m_bankIndex(index),
      m_bankType(compInfo.componentType(index)) {

  switch (m_bankType) {
  case ComponentType::Grid:
    m_colorTextureIDs.resize(6);
    m_pickTextureIDs.resize(6);
    m_detColorTextures.resize(6);
    m_detPickTextures.resize(6);
    m_textSizes.resize(6);
    break;
  case ComponentType::Rectangular:
  case ComponentType::OutlineComposite:
    m_colorTextureIDs.resize(1);
    m_pickTextureIDs.resize(1);
    m_detColorTextures.resize(1);
    m_detPickTextures.resize(1);
    m_textSizes.resize(1);
    break;
  default:
    break;
  }
}

BankTextureBuilder::~BankTextureBuilder() {
  for (size_t i = 0; i < m_colorTextureIDs.size(); ++i) {
    auto &colTextureID = m_colorTextureIDs[i];
    auto &pickTextureID = m_pickTextureIDs[i];

    if (colTextureID > 0)
      glDeleteTextures(1, &colTextureID);
    if (pickTextureID > 0)
      glDeleteTextures(1, &pickTextureID);
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
other types.This state of this flag is stored for uploading at a later stage.
@param layer grid layer which should be rendered. Ignored for all other types.
This layer value is stored for uploading at a later stage.
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
  default:
    break;
  }
}

/** Uploads openGL textures for rendering. This method results in openGL calls.
@param picking flag which determines whether pick colors are rendered.
@param gridFace flag which determines which grid face to be rendered. Ignored if
using layers or not a grid detector.
*/
void BankTextureBuilder::uploadTextures(bool picking,
                                        GridTextureFace gridFace) {
  switch (m_bankType) {
  case ComponentType::OutlineComposite:
    uploadTubeBankTextures(picking);
    break;
  case ComponentType::Grid:
    uploadGridBankTexture(picking, gridFace);
    break;
  case ComponentType::Rectangular:
    uploadRectangularBankTextures(picking);
    break;
  default:
    break;
  }
}

void BankTextureBuilder::buildTubeBankTextures(
    const std::vector<GLColor> &colors, bool picking) {
  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];
  const auto &children = m_compInfo.children(m_bankIndex);
  texture.resize(children.size() * 3);

  for (size_t i = 0; i < children.size(); ++i) {
    const auto &col = colors[children[i]];
    auto pos = i * 3;
    texture[pos] = static_cast<unsigned char>(col.red());
    texture[pos + 1] = static_cast<unsigned char>(col.green());
    texture[pos + 2] = static_cast<unsigned char>(col.blue());
  }
}

void BankTextureBuilder::buildGridBankFull(const std::vector<GLColor> &colors,
                                           bool picking) {
  const auto &layers = m_compInfo.children(m_bankIndex);
  auto bank = m_compInfo.quadrilateralComponent(layers[0]);

  auto &textures = picking ? m_detPickTextures : m_detColorTextures;

  // Front face
  auto res = BankRenderingHelpers::getCorrectedTextureSize(bank.nX, bank.nY);
  m_textSizes[0] = res;
  m_textSizes[1] = res;
  textures[0].resize(res.first * res.second * 3, 0);
  // Rear face
  textures[1].resize(res.first * res.second * 3, 0);

  // make front and back faces which lie along the z (layer) axis
  // NB. last texture in the layer is the front face
  createTexture(m_compInfo, m_compInfo.children(layers.back()), colors,
                textures[0], res.first);
  createTexture(m_compInfo, m_compInfo.children(layers.front()), colors,
                textures[1], res.first);

  auto nZ = layers.size();
  // Left face
  res = BankRenderingHelpers::getCorrectedTextureSize(nZ, bank.nY);
  m_textSizes[2] = res;
  m_textSizes[3] = res;
  textures[2].resize(res.first * res.second * 3, 0);
  // Right Face
  textures[3].resize(res.first * res.second * 3, 0);

  // Top face
  res = BankRenderingHelpers::getCorrectedTextureSize(bank.nX, nZ);
  m_textSizes[4] = res;
  m_textSizes[5] = res;
  textures[4].resize(res.first * res.second * 3, 0);
  // Bottom Face
  textures[5].resize(res.first * res.second * 3, 0);

  auto li = nZ - 1;
  for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
    auto layerIndex = (*it);
    bank = m_compInfo.quadrilateralComponent(layerIndex);
    const auto &children = m_compInfo.children(layerIndex);
    addColorsToLeftAndRightTextures(m_compInfo, bank, nZ, li, children, colors,
                                    textures[2], textures[3]);
    addColorsToTopAndBottomTextures(m_compInfo, bank, nZ - li - 1, children,
                                    colors, textures[4], textures[5]);
    --li;
  }
}

void BankTextureBuilder::buildGridBankLayer(const std::vector<GLColor> &colors,
                                            bool picking, size_t layer) {
  const auto &layers = m_compInfo.children(m_bankIndex);
  auto bank = m_compInfo.quadrilateralComponent(layers[0]);

  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];

  // Front face
  auto res = BankRenderingHelpers::getCorrectedTextureSize(bank.nX, bank.nY);
  m_textSizes[0] = res;
  texture.resize(res.first * res.second * 3, 0);

  // make front and back faces which lie along the z (layer) axis
  createTexture(m_compInfo, m_compInfo.children(layers[layer]), colors, texture,
                res.first);
}

void BankTextureBuilder::buildGridBankTextures(
    const std::vector<GLColor> &colors, bool picking, bool isUsingLayer,
    size_t layer) {
  m_isUsingLayers = isUsingLayer;
  m_layer = layer;

  if (isUsingLayer)
    buildGridBankLayer(colors, picking, layer);
  else
    buildGridBankFull(colors, picking);
}

void BankTextureBuilder::buildRectangularBankTextures(
    const std::vector<GLColor> &colors, bool picking) {
  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];
  auto bank = m_compInfo.quadrilateralComponent(m_bankIndex);
  // Round size up to nearest power of 2
  auto res = BankRenderingHelpers::getCorrectedTextureSize(bank.nX, bank.nY);
  m_textSizes[0] = res;
  auto texSizeX = res.first;
  auto texSizeY = res.second;

  // Texture width is 3 times wider due to RGB values
  texture.resize(texSizeX * texSizeY * 3, 0); // fill with black

  createTexture(m_compInfo, m_compInfo.children(m_bankIndex), colors, texture,
                texSizeX);
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

void BankTextureBuilder::uploadGridBankTexture(bool picking,
                                               GridTextureFace gridBankFace) {
  if (m_isUsingLayers) {
    auto &textures = picking ? m_detPickTextures : m_detColorTextures;
    auto &textureIDs = picking ? m_pickTextureIDs : m_colorTextureIDs;

    upload2DTexture(m_textSizes[0], textureIDs[0], textures[0]);
    return;
  }

  auto textureIndex = static_cast<size_t>(gridBankFace);
  auto &texture = picking ? m_detPickTextures[textureIndex]
                          : m_detColorTextures[textureIndex];
  auto &textureID = picking ? m_pickTextureIDs[textureIndex]
                            : m_colorTextureIDs[textureIndex];

  upload2DTexture(m_textSizes[textureIndex], textureID, texture);
}

void BankTextureBuilder::uploadRectangularBankTextures(bool picking) {
  auto &texture = picking ? m_detPickTextures[0] : m_detColorTextures[0];
  auto &textureID = picking ? m_pickTextureIDs[0] : m_colorTextureIDs[0];

  upload2DTexture(m_textSizes[0], textureID, texture);
}

void BankTextureBuilder::bindTextures(bool picking) const {
  glEnable(GL_TEXTURE_2D);
  auto &textureIDs = picking ? m_pickTextureIDs : m_colorTextureIDs;

  for (auto &textureID : textureIDs)
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void BankTextureBuilder::unbindTextures() const {
  for (size_t i = 0; i < m_pickTextureIDs.size(); ++i)
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace detail
} // namespace MantidWidgets
} // namespace MantidQt
