#ifndef INSTRUMENTRENDERER_H_
#define INSTRUMENTRENDERER_H_

#include "DllOption.h"
#include "GLColor.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidQtWidgets/LegacyQwt/MantidColorMap.h"
#include <QString>

namespace Mantid {
namespace Geometry {
class ComponentInfo;
class DetectorInfo;
} // namespace Geometry
} // namespace Mantid

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
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // INSTRUMENTRENDERER_H_