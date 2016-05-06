#include "MantidGeometry/Instrument/Can.h"

namespace Mantid {
namespace Geometry {

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
/**
 * Construct a Can providing an XML definition of the default sample shape
 * @param sampleTemplateXML XML definition of the sample shape
 */
Can::Can(std::string sampleTemplateXML)
    : m_sampleShapeTemplate(sampleTemplateXML) {}

const std::string &Can::sampleShapeTemplate() const {
  return m_sampleShapeTemplate;
}

} // namespace Geometry
} // namespace Mantid
