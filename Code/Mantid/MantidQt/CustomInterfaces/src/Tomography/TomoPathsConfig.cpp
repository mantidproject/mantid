#include "MantidQtCustomInterfaces/Tomography/TomoPathsConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoPathsConfig::TomoPathsConfig()
    : m_pathSCARFbase("/work/imat/phase_commisioning/"),
      m_pathFITS(m_pathSCARFbase + "data/sample"),
      m_pathFlat(m_pathSCARFbase + "data/ob"),
      m_pathDark(m_pathSCARFbase + "data/di") {}

bool TomoPathsConfig::validate() const {
  // TODO: too simple for now
  return (!m_pathFITS.empty() && !m_pathFlat.empty() && !m_pathDark.empty());
}

} // namespace CustomInterfaces
} // namespace MantidQt
