#ifndef BANKRENDERINGHELPERS_H
#define BANKRENDERINGHELPERS_H
#include "DllOption.h"
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include "MantidQtWidgets/InstrumentView/GridTextureFace.h"
#include <vector>

namespace Mantid {
namespace Geometry {
class ComponentInfo;
}
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {
namespace BankRenderingHelpers {

EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW std::pair<size_t, size_t>
getCorrectedTextureSize(const size_t width, const size_t height);

/** Render GridDetector Bank as 6 bitmaps for all layers or one bitmap for the
selected layer Makes OpenGL calls for drawing the bank in an OpenGL window as
textures. NB glBegin() and glEnd() are called within this function.
*/
EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW void
renderGridBankLayer(const Mantid::Geometry::ComponentInfo &compInfo,
                    size_t index, size_t layer);

EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW void
renderGridBankFull(const Mantid::Geometry::ComponentInfo &compInfo,
                   size_t index, detail::GridTextureFace gridFace);

/** Render RectangularDetector Bank as bitmap texture
Makes OpenGL calls for drawing the bank in an OpenGL window. NB glBegin() and
glEnd() are called within this function.
*/
EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW void
renderRectangularBank(const Mantid::Geometry::ComponentInfo &compInfo,
                      size_t index);

/** Render Structured Detector Bank as quads
Makes OpenGL calls for drawing the bank in an OpenGL window. NB glBegin() and
glEnd() are called within this function.
*/
EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW void
renderStructuredBank(const Mantid::Geometry::ComponentInfo &compInfo,
                     size_t index, const std::vector<GLColor> &color);
} // namespace BankRenderingHelpers
} // namespace MantidWidgets
} // namespace MantidQt

#endif // BANKRENDERINGHELPERS_H