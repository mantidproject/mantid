// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ColorMap.h"
#include "DllOption.h"
#include "GLColor.h"

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidQtWidgets/InstrumentView/BankTextureBuilder.h"
#include "MantidQtWidgets/InstrumentView/ColorMap.h"
#include <QString>
#include <map>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
class InstrumentActor;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentRenderer {
private:
  const InstrumentActor &m_actor;
  std::vector<GLColor> m_colors;
  std::vector<GLColor> m_pickColors;
  std::vector<detail::BankTextureBuilder> m_textures;
  std::map<size_t, size_t> m_reverseTextureIndexMap;
  ColorMap m_colorMap;
  bool m_isUsingLayers;
  size_t m_layer;
  bool m_HighlightDetsWithZeroCount;

public:
  InstrumentRenderer(const InstrumentActor &actor);

  virtual ~InstrumentRenderer() = default;

  virtual void renderInstrument(const std::vector<bool> &visibleComps, bool showGuides, bool picking = false) = 0;

  void reset();

  void changeScaleType(ColorMap::ScaleType type);

  void changeNthPower(double nth_power);

  void loadColorMap(const std::pair<QString, bool> &cmap);

  const ColorMap &getColorMap() const { return m_colorMap; }

  GLColor getColor(size_t index) const;

  void enableGridBankLayers(bool on, size_t layer);

  static GLColor makePickColor(size_t pickID);

  static size_t decodePickColor(const QRgb &c);

  bool isUsingLayers() const { return m_isUsingLayers; }

  size_t selectedLayer() const { return m_layer; }

protected:
  virtual void draw(const std::vector<bool> &visibleComps, bool showGuides, bool picking) = 0;
  void drawGridBank(size_t bankIndex, bool picking);
  void drawRectangularBank(size_t bankIndex, bool picking);
  void drawStructuredBank(size_t bankIndex, bool picking);
  void drawTube(size_t bankIndex, bool picking);
  void drawSingleDetector(size_t detIndex, bool picking);
  void invalidateAndDeleteDisplayList(std::vector<GLuint> &displayList, bool &useList);
  void updateVisited(const Mantid::Geometry::ComponentInfo &compInfo, const size_t bankIndex,
                     std::vector<bool> &visited);
  virtual void resetDisplayLists() = 0;
  const InstrumentActor &instrActor() const { return m_actor; };
  void drawComponent(const size_t index, const std::vector<bool> &visibleComps, bool showGuides, bool picking,
                     const Mantid::Geometry::ComponentInfo &compInfo, std::vector<bool> &visited);

private:
  void resetColors();
  void resetPickColors();
};
} // namespace MantidWidgets
} // namespace MantidQt
