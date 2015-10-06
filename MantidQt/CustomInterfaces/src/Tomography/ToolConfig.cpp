#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"


#include <boost/lexical_cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {

ToolConfigTomoPy::ToolConfigTomoPy(const std::string &runnable,
                                   const std::string &pathOut,
                                   const std::string &pathDark,
                                   const std::string &pathOpen,
                                   const std::string &pathSample,
                                   double centerRot, double angleMin,
                                   double angleMax)
    : TomoRecToolConfig(runnable), m_pathOut(pathOut), m_pathDark(pathDark),
      m_pathOpen(pathOpen), m_pathSample(pathSample), m_centerRot(centerRot),
      m_angleMin(angleMin), m_angleMax(angleMax) {}

std::string ToolConfigTomoPy::makeCmdLineOptions() const {
  return "--input_dir " + m_pathSample + " --dark " + m_pathDark + " --white " +
         m_pathOpen + " --output " + m_pathOut + " --start_angle " +
         boost::lexical_cast<std::string>(m_angleMin) + " --end_angle " +
         boost::lexical_cast<std::string>(m_angleMax) +
         " --center_of_rotation " +
         boost::lexical_cast<std::string>(m_centerRot);
}

ToolConfigAstraToolbox::ToolConfigAstraToolbox(
    const std::string &runnable, double centerRot, double angleMin,
    double angleMax, const std::string &pathOut, const std::string &pathDark,
    const std::string &pathOpen, const std::string &pathSample)
    : TomoRecToolConfig(runnable), m_centerRot(centerRot), m_angleMin(angleMin),
      m_angleMax(angleMax), m_pathOut(pathOut), m_pathDark(pathDark),
      m_pathOpen(pathOpen), m_pathSample(pathSample) {}

std::string ToolConfigAstraToolbox::makeCmdLineOptions() const {
  return "--start_slice " + boost::lexical_cast<std::string>(m_angleMin) +
         " --end_slice " + boost::lexical_cast<std::string>(m_angleMax) +
         " --center_of_rotation " +
         boost::lexical_cast<std::string>(m_centerRot) + " --input_dir " +
         m_pathSample + " --dark " + m_pathDark + " --white " + m_pathOpen +
         " --output " + m_pathOut;
}

} // namespace CustomInterfaces
} // namespace MantidQt
