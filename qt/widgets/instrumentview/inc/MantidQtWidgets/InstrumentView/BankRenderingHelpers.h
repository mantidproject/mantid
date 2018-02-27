#ifndef BANKRENDERINGHELPERS_H
#define BANKRENDERINGHELPERS_H
#include "DllOption.h"
#include "MantidQtWidgets/InstrumentView/GLColor.h"
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

/// Render RectangularDetector Bank as bitmap texture
EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW void
renderRectangularBank(const Mantid::Geometry::ComponentInfo &compInfo,
                      size_t index);

/// Render Structured Detector Bank as quads
EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW void
renderStructuredBank(const Mantid::Geometry::ComponentInfo &compInfo,
                     size_t index, const std::vector<GLColor> &color);
} // namespace RenderingHelpers
} // namespace MantidWidgets
} // namespace MantidQt

#endif // BANKRENDERINGHELPERS_H