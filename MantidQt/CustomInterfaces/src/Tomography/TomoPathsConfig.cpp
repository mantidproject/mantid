#include "MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoPathsConfig::TomoPathsConfig()
    : m_pathOpenBeamEnabled(true), m_pathDarkEnabled(true),
      m_pathBase("/work/imat/phase_commissioning/"),
      m_pathFITS(m_pathBase + "data"), m_pathFlat(m_pathBase + "flat"),
      m_pathDark(m_pathBase + "dark") {}

bool TomoPathsConfig::validate() const {
  // TODO: too simple for now
  return (!m_pathFITS.empty() && !m_pathFlat.empty() && !m_pathDark.empty());
}

} // namespace CustomInterfaces
} // namespace MantidQt
