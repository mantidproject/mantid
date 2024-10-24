// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GLColor.h"
#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidQtWidgets/InstrumentView/GridTextureFace.h"
#include <utility>
#include <vector>

namespace Mantid {
namespace Geometry {
class ComponentInfo;
}
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {

namespace detail {
class BankTextureBuilder {
public:
  BankTextureBuilder(const Mantid::Geometry::ComponentInfo &compInfo, size_t index);
  ~BankTextureBuilder();
  void buildColorTextures(const std::vector<GLColor> &colors, bool isUsingLayer = false, size_t layer = 0);
  void buildPickTextures(const std::vector<GLColor> &colors, bool isUsingLayer = false, size_t layer = 0);
  void uploadTextures(bool picking, GridTextureFace gridFace = GridTextureFace::Front);
  void bindTextures(bool picking) const;
  void unbindTextures() const;

private:
  void buildOpenGLTextures(bool picking, const std::vector<GLColor> &colors, bool isUsingLayer, size_t layer);
  void buildTubeBankTextures(const std::vector<GLColor> &colors, bool picking);
  void buildGridBankFull(const std::vector<GLColor> &colors, bool picking);
  void buildGridBankLayer(const std::vector<GLColor> &colors, bool picking, size_t layer);
  void buildGridBankTextures(const std::vector<GLColor> &colors, bool picking, bool isUsingLayer, size_t layer);
  void buildRectangularBankTextures(const std::vector<GLColor> &colors, bool picking);
  void uploadTubeBankTextures(bool picking);
  void uploadGridBankTexture(bool picking, GridTextureFace gridFace);
  void uploadRectangularBankTextures(bool picking);

private:
  const Mantid::Geometry::ComponentInfo &m_compInfo;
  const size_t m_bankIndex;
  const Mantid::Beamline::ComponentType m_bankType;
  bool m_isUsingLayers;
  size_t m_layer;
  std::vector<GLuint> m_colorTextureIDs;
  std::vector<GLuint> m_pickTextureIDs;
  std::vector<std::pair<size_t, size_t>> m_textSizes;
  std::vector<std::vector<char>> m_detColorTextures;
  std::vector<std::vector<char>> m_detPickTextures;
};
} // namespace detail
} // namespace MantidWidgets
} // namespace MantidQt
