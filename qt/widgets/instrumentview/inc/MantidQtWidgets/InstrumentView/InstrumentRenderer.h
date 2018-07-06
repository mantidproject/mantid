#ifndef INSTRUMENTRENDERER_H_
#define INSTRUMENTRENDERER_H_

#include "DllOption.h"
#include "GLColor.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "../../../src/qwt/MantidColorMap.h"
#include <QString>

namespace MantidQt {
namespace MantidWidgets {
class InstrumentActor;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentRenderer {
private:
  const InstrumentActor &m_actor;
  std::vector<GLColor> m_colors;
  std::vector<GLColor> m_pickColors;
  mutable GLuint m_displayListId[2];
  mutable bool m_useDisplayList[2];
  mutable std::vector<GLuint> m_textureIDs;
  mutable std::vector<size_t> m_textureIndices;
  mutable std::map<size_t, size_t> m_reverseTextureIndexMap;
  mutable std::vector<std::vector<char>> colorTextures;
  mutable std::vector<std::vector<char>> pickTextures;
  std::vector<double> m_specIntegrs;
  MantidColorMap m_colorMap;

public:
  InstrumentRenderer(const InstrumentActor &actor);
  ~InstrumentRenderer();
  void renderInstrument(const std::vector<bool> &visibleComps, bool showGuides,
                        bool picking = false);
  void reset();

  void changeScaleType(int type);

  void changeNthPower(double nth_power);

  void loadColorMap(const QString &fname);

  const MantidColorMap &getColorMap() const { return m_colorMap; }

  GLColor getColor(size_t index) const;

  static GLColor makePickColor(size_t pickID);

  static size_t decodePickColor(const QRgb &c);

private:
  void resetColors();
  void resetPickColors();
  void draw(const std::vector<bool> &visibleComps, bool showGuides,
            bool picking);
  void drawRectangularBank(size_t bankIndex, bool picking);
  void drawStructuredBank(size_t bankIndex, bool picking);
  void drawTube(size_t bankIndex, bool picking);
  void drawSingleDetector(size_t detIndex, bool picking);
  void generateRectangularTexture(std::vector<char> &texture,
                                  const std::vector<GLColor> &colors,
                                  size_t bankIndex);

  void uploadRectangularTexture(const std::vector<char> &texture,
                                size_t textureIndex) const;

  void generateTubeTexture(std::vector<char> &texture,
                           const std::vector<GLColor> &colors,
                           size_t bankIndex);
  void uploadTubeTexture(const std::vector<char> &texture,
                         size_t textureIndex) const;
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // INSTRUMENTRENDERER_H_
