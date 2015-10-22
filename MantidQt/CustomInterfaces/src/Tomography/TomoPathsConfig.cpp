#include "MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoPathsConfig::TomoPathsConfig()
    : m_pathBase("/work/imat/phase_commissioning/"),      
      m_pathFITS(m_pathBase + "data/sample"),
      m_pathFlat(m_pathBase + "data/ob"),
      m_pathDark(m_pathBase + "data/di"),
      m_pathScriptsTools(m_pathBase + "runs_scripts"){}

bool TomoPathsConfig::validate() const {
  // TODO: too simple for now
  return (!m_pathFITS.empty() && !m_pathFlat.empty() && !m_pathDark.empty());
}

} // namespace CustomInterfaces
} // namespace MantidQt
